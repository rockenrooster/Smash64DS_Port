#include "nds_scene_harness_config.h"

extern void ndsIFCommonRecordHUDState(void);

static void ndsFighterWalkRecordBefore(u32 slot, FTStruct *fp, DObj *root,
                                       s32 stick_x)
{
    u32 root_x = (root != NULL) ? ndsFloatBits(root->translate.vec.f.x) : 0u;
    u32 root_y = (root != NULL) ? ndsFloatBits(root->translate.vec.f.y) : 0u;
    u32 stick_abs = (u32)ABS(stick_x);
    u32 input_success =
        ((stick_x * fp->lr) >= 8) ? 1u : 0u;
    u32 selected_status = (u32)ftCommonWalkGetWalkStatus((s8)stick_x);

    if (slot == 0u)
    {
        gNdsFighterWalkP0StickX = stick_x;
        gNdsFighterWalkP0StickAbs = stick_abs;
        gNdsFighterWalkP0LR = fp->lr;
        gNdsFighterWalkP0InputSuccess = input_success;
        gNdsFighterWalkP0SelectedStatus = selected_status;
        gNdsFighterWalkP0StatusBefore = (u32)fp->status_id;
        gNdsFighterWalkP0MotionBefore = (u32)fp->motion_id;
        gNdsFighterWalkP0GABefore = (u32)fp->ga;
        gNdsFighterWalkP0GroundVelBeforeMilli =
            ndsFloatToMilliSigned(fp->physics.vel_ground.x);
        gNdsFighterWalkP0RootXBeforeBits = root_x;
        gNdsFighterWalkP0RootYBeforeBits = root_y;
    }
    else if (slot == 1u)
    {
        gNdsFighterWalkP1StickX = stick_x;
        gNdsFighterWalkP1StickAbs = stick_abs;
        gNdsFighterWalkP1LR = fp->lr;
        gNdsFighterWalkP1InputSuccess = input_success;
        gNdsFighterWalkP1SelectedStatus = selected_status;
        gNdsFighterWalkP1StatusBefore = (u32)fp->status_id;
        gNdsFighterWalkP1MotionBefore = (u32)fp->motion_id;
        gNdsFighterWalkP1GABefore = (u32)fp->ga;
        gNdsFighterWalkP1GroundVelBeforeMilli =
            ndsFloatToMilliSigned(fp->physics.vel_ground.x);
        gNdsFighterWalkP1RootXBeforeBits = root_x;
        gNdsFighterWalkP1RootYBeforeBits = root_y;
    }
}

static void ndsFighterWalkRecordAfter(u32 slot, FTStruct *fp, DObj *root)
{
    u32 root_x = (root != NULL) ? ndsFloatBits(root->translate.vec.f.x) : 0u;
    u32 root_y = (root != NULL) ? ndsFloatBits(root->translate.vec.f.y) : 0u;

    if (slot == 0u)
    {
        gNdsFighterWalkP0StatusAfter = (u32)fp->status_id;
        gNdsFighterWalkP0MotionAfter = (u32)fp->motion_id;
        gNdsFighterWalkP0GAAfter = (u32)fp->ga;
        gNdsFighterWalkP0GroundVelAfterMilli =
            ndsFloatToMilliSigned(fp->physics.vel_ground.x);
        gNdsFighterWalkP0AirVelXMilli =
            ndsFloatToMilliSigned(fp->physics.vel_air.x);
        gNdsFighterWalkP0AirVelYMilli =
            ndsFloatToMilliSigned(fp->physics.vel_air.y);
        gNdsFighterWalkP0RootXAfterBits = root_x;
        gNdsFighterWalkP0RootYAfterBits = root_y;
    }
    else if (slot == 1u)
    {
        gNdsFighterWalkP1StatusAfter = (u32)fp->status_id;
        gNdsFighterWalkP1MotionAfter = (u32)fp->motion_id;
        gNdsFighterWalkP1GAAfter = (u32)fp->ga;
        gNdsFighterWalkP1GroundVelAfterMilli =
            ndsFloatToMilliSigned(fp->physics.vel_ground.x);
        gNdsFighterWalkP1AirVelXMilli =
            ndsFloatToMilliSigned(fp->physics.vel_air.x);
        gNdsFighterWalkP1AirVelYMilli =
            ndsFloatToMilliSigned(fp->physics.vel_air.y);
        gNdsFighterWalkP1RootXAfterBits = root_x;
        gNdsFighterWalkP1RootYAfterBits = root_y;
    }
}

static void ndsFighterMarioFoxRunWalkInputProof(void)
{
    u32 mask = 0u;
    u32 gobj_before;
    u32 gobj_after;
    u32 i;

    if ((ndsFighterMarioFoxWalkInputProofEnabled() == FALSE) ||
        (gNdsFighterMarioFoxWalkInputResult != 0u))
    {
        return;
    }

    if ((gNdsFighterMarioFoxDLAllDrawResult ==
            NDS_FIGHTER_MARIOFOX_DL_ALL_DRAW_PASS) &&
        (gNdsFighterMarioFoxDLAllDrawSafeResult ==
            NDS_FIGHTER_MARIOFOX_DL_ALL_DRAW_SAFE_PASS) &&
        ((gNdsFighterMarioFoxDLAllDrawMask & 0x7ffu) == 0x7ffu) &&
        (gNdsFighterMarioFoxDLAllDrawDeferredMask == 0xffu) &&
        (gNdsFighterMarioFoxDLAllDrawCount == 2u))
    {
        mask |= 1u << 0;
    }
    else
    {
        gNdsFighterMarioFoxWalkInputMask = mask;
        return;
    }

    gobj_before = (u32)gcGetGObjsActiveNum();

    for (i = 0u; i < 2u; i++)
    {
        FTStruct *fp = &sNdsFighterStructPool[i];
        GObj *probe_gobj;
        DObj *root;
        s32 stick_mag = (i == 0u) ? 40 : 80;
        s32 stick_x;
        u32 status_after_interrupt;
        u32 motion_after_interrupt;

        if ((ndsFighterStructIsPoolPointer(fp) == FALSE) ||
            (fp->fighter_gobj == NULL) ||
            (fp->status_id != nFTCommonStatusWait) ||
            (fp->motion_id != nFTCommonMotionWait) ||
            (fp->ga != nMPKineticsGround))
        {
            continue;
        }

        probe_gobj = fp->fighter_gobj;
        root = fp->joints[nFTPartsJointTopN];
        if ((root == NULL) || (fp->attr == NULL) ||
            (fp->proc_interrupt != ftCommonWaitProcInterrupt))
        {
            continue;
        }

        fp->physics.vel_ground.x = 0.0F;
        fp->physics.vel_ground.y = 0.0F;
        fp->physics.vel_ground.z = 0.0F;
        fp->physics.vel_air.x = 0.0F;
        fp->physics.vel_air.y = 0.0F;
        fp->physics.vel_air.z = 0.0F;
        fp->vel_push.x = 0.0F;
        fp->vel_push.y = 0.0F;
        fp->vel_push.z = 0.0F;
        fp->physics.vel_jostle_x = 0.0F;
        fp->physics.vel_jostle_z = 0.0F;
        ndsFighterSyncPhysicsToLegacyVel(fp);

        fp->coll_data.floor_line_id = 0;
        fp->coll_data.floor_flags = nMPMaterialCommon;
        fp->coll_data.floor_angle.x = 0.0F;
        fp->coll_data.floor_angle.y = 1.0F;
        fp->coll_data.floor_angle.z = 0.0F;
        fp->coll_data.mask_stat = MAP_FLAG_FLOOR;
        fp->coll_data.is_coll_end = FALSE;

        stick_x = stick_mag * ((fp->lr >= 0) ? 1 : -1);
        fp->input.pl.stick_range.x = (s8)stick_x;
        fp->input.pl.stick_range.y = 0;
        fp->input.pl.button_hold = 0;
        fp->input.pl.button_tap = 0;

        ndsFighterWalkRecordBefore(i, fp, root, stick_x);

        sNdsFighterWalkInputProbeActive = TRUE;
        fp->proc_interrupt(probe_gobj);
        sNdsFighterWalkInputProbeActive = FALSE;

        status_after_interrupt = (u32)fp->status_id;
        motion_after_interrupt = (u32)fp->motion_id;
        if ((fp->proc_interrupt == ftCommonWalkProcInterrupt) &&
            (fp->proc_physics == ftCommonWalkProcPhysics) &&
            (fp->proc_map == mpCommonProcFighterOnCliffEdge))
        {
            gNdsFighterWalkCallbackReadyCount++;
        }

        if (ndsFighterMarioFoxWalkLoopProofEnabled() == FALSE)
        {
            sNdsFighterWalkLoopProbeActive = TRUE;
            if (fp->proc_interrupt != NULL)
            {
                fp->proc_interrupt(probe_gobj);
                gNdsFighterWalkLoopInterruptCallCount++;
            }
            sNdsFighterWalkLoopProbeActive = FALSE;
        }

        if ((fp->status_id != (s32)status_after_interrupt) ||
            (fp->motion_id != (s32)motion_after_interrupt))
        {
            gNdsFighterWalkUnexpectedStatusCount++;
        }

        sNdsFighterWalkPhysicsMapPassActive = TRUE;
        ndsFighterSyncLegacyVelToPhysics(fp);
        if (fp->proc_physics != NULL)
        {
            gNdsFighterWalkPhysicsCallbackCount++;
            fp->proc_physics(probe_gobj);
        }
        if (fp->proc_map != NULL)
        {
            fp->proc_map(probe_gobj);
        }
        sNdsFighterWalkPhysicsMapPassActive = FALSE;

        ndsFighterWalkRecordAfter(i, fp, root);
        if (((i == 0u) && (fp->status_id == nFTCommonStatusWalkMiddle) &&
             (fp->motion_id == nFTCommonMotionWalkMiddle)) ||
            ((i == 1u) && (fp->status_id == nFTCommonStatusWalkFast) &&
             (fp->motion_id == nFTCommonMotionWalkFast)))
        {
            gNdsFighterMarioFoxWalkInputCount++;
        }
    }

    sNdsFighterWalkInputProbeActive = FALSE;
    sNdsFighterWalkLoopProbeActive = FALSE;
    sNdsFighterWalkPhysicsMapPassActive = FALSE;

    if ((gNdsFighterWalkP0StickAbs == 40u) &&
        (gNdsFighterWalkP1StickAbs == 80u) &&
        (gNdsFighterWalkP0InputSuccess == 1u) &&
        (gNdsFighterWalkP1InputSuccess == 1u))
    {
        mask |= 1u << 1;
    }
    if (gNdsFighterWalkOriginalCheckSuccessCount == 2u)
    {
        mask |= 1u << 2;
    }
    if ((gNdsFighterWalkWaitInterruptCallCount == 2u) &&
        (gNdsFighterWalkGroundCheckCallCount == 2u) &&
        (gNdsFighterWalkOriginalCheckCallCount == 2u))
    {
        mask |= 1u << 3;
    }
    if ((gNdsFighterWalkP0SelectedStatus ==
            (u32)nFTCommonStatusWalkMiddle) &&
        (gNdsFighterWalkP1SelectedStatus ==
            (u32)nFTCommonStatusWalkFast) &&
        (gNdsFighterWalkP0StatusAfter ==
            (u32)nFTCommonStatusWalkMiddle) &&
        (gNdsFighterWalkP1StatusAfter ==
            (u32)nFTCommonStatusWalkFast))
    {
        mask |= 1u << 4;
    }
    if ((gNdsFighterWalkSetStatusCallCount == 2u) &&
        (gNdsFighterWalkFtMainSetStatusCallCount == 2u) &&
        (gNdsFighterWalkAnimEventsCallCount == 2u) &&
        (gNdsFighterWalkCallbackReadyCount == 2u) &&
        (gNdsFighterWalkP0MotionAfter ==
            (u32)nFTCommonMotionWalkMiddle) &&
        (gNdsFighterWalkP1MotionAfter ==
            (u32)nFTCommonMotionWalkFast))
    {
        mask |= 1u << 5;
    }
    if (((ndsFighterMarioFoxWalkLoopProofEnabled() != FALSE) ||
         (gNdsFighterWalkLoopInterruptCallCount == 2u)) &&
        (gNdsFighterWalkP0StatusAfter ==
            (u32)nFTCommonStatusWalkMiddle) &&
        (gNdsFighterWalkP1StatusAfter ==
            (u32)nFTCommonStatusWalkFast))
    {
        mask |= 1u << 6;
    }
    if ((gNdsFighterWalkGroundVelAbsStickCount == 2u) &&
        (gNdsFighterWalkGroundVelTransferAirCount == 2u) &&
        (gNdsFighterWalkPhysicsCallbackCount == 2u) &&
        (gNdsFighterWalkP0GroundVelAfterMilli > 0) &&
        (gNdsFighterWalkP1GroundVelAfterMilli > 0))
    {
        mask |= 1u << 7;
    }
    if ((gNdsFighterWalkMapCallbackCount == 2u) &&
        (gNdsFighterWalkMapSafeFloorCount == 2u) &&
        (gNdsFighterWalkMapFallDeniedCount == 0u) &&
        (gNdsFighterWalkMapOttottoDeniedCount == 0u))
    {
        mask |= 1u << 8;
    }

    gobj_after = (u32)gcGetGObjsActiveNum();
    gNdsFighterWalkGObjDelta =
        (gobj_after >= gobj_before) ? (gobj_after - gobj_before) :
            (gobj_before - gobj_after);
    if ((gNdsFighterWalkP0GABefore == (u32)nMPKineticsGround) &&
        (gNdsFighterWalkP1GABefore == (u32)nMPKineticsGround) &&
        (gNdsFighterWalkP0GAAfter == (u32)nMPKineticsGround) &&
        (gNdsFighterWalkP1GAAfter == (u32)nMPKineticsGround) &&
        (gNdsFighterWalkP0RootXBeforeBits ==
            gNdsFighterWalkP0RootXAfterBits) &&
        (gNdsFighterWalkP0RootYBeforeBits ==
            gNdsFighterWalkP0RootYAfterBits) &&
        (gNdsFighterWalkP1RootXBeforeBits ==
            gNdsFighterWalkP1RootXAfterBits) &&
        (gNdsFighterWalkP1RootYBeforeBits ==
            gNdsFighterWalkP1RootYAfterBits) &&
        (gNdsFighterWalkGObjDelta == 0u))
    {
        mask |= 1u << 9;
    }
    if ((gNdsFighterWalkDeniedStatusCount == 0u) &&
        (gNdsFighterWalkUnexpectedStatusCount == 0u) &&
        (gNdsFighterWalkProcessAttachCount == 0u) &&
        (gNdsFighterWalkDisplayProbeCount == 0u) &&
        (gNdsFighterWalkGameplayUpdateCount == 0u) &&
        (gNdsFighterWalkDrawCallCount == 0u) &&
        (gNdsFighterWalkMatrixCallCount == 0u) &&
        (gSCManagerSceneData.scene_curr == nSCKindVSBattle) &&
        (gSCManagerSceneData.scene_prev == nSCKindMaps))
    {
        mask |= 1u << 10;
    }

    gNdsFighterMarioFoxWalkInputMask = mask;
    gNdsFighterMarioFoxWalkDeferredMask = 0xffu;
    if ((mask & 0x7ffu) == 0x7ffu)
    {
        gNdsFighterMarioFoxWalkInputResult =
            NDS_FIGHTER_MARIOFOX_WALK_INPUT_PASS;
        gNdsFighterMarioFoxWalkSafeResult =
            NDS_FIGHTER_MARIOFOX_WALK_SAFE_PASS;
    }

    ndsFighterMarioFoxRunWalkLoopProof();
}

static void ndsFighterWalkLoopRecordStart(u32 slot, FTStruct *fp,
                                           DObj *root)
{
    u32 root_x = (root != NULL) ? ndsFloatBits(root->translate.vec.f.x) : 0u;
    u32 root_y = (root != NULL) ? ndsFloatBits(root->translate.vec.f.y) : 0u;

    if (slot == 0u)
    {
        gNdsFighterWalkLoopP0StatusStart = (u32)fp->status_id;
        gNdsFighterWalkLoopP0MotionStart = (u32)fp->motion_id;
        gNdsFighterWalkLoopP0GAStart = (u32)fp->ga;
        gNdsFighterWalkLoopP0RootXStartBits = root_x;
        gNdsFighterWalkLoopP0RootYStartBits = root_y;
        gNdsFighterWalkLoopP0GroundVelStartMilli =
            ndsFloatToMilliSigned(fp->physics.vel_ground.x);
    }
    else if (slot == 1u)
    {
        gNdsFighterWalkLoopP1StatusStart = (u32)fp->status_id;
        gNdsFighterWalkLoopP1MotionStart = (u32)fp->motion_id;
        gNdsFighterWalkLoopP1GAStart = (u32)fp->ga;
        gNdsFighterWalkLoopP1RootXStartBits = root_x;
        gNdsFighterWalkLoopP1RootYStartBits = root_y;
        gNdsFighterWalkLoopP1GroundVelStartMilli =
            ndsFloatToMilliSigned(fp->physics.vel_ground.x);
    }
}

static void ndsFighterWalkLoopRecordAfterHeld(u32 slot, FTStruct *fp,
                                              DObj *root)
{
    u32 root_x = (root != NULL) ? ndsFloatBits(root->translate.vec.f.x) : 0u;
    u32 root_y = (root != NULL) ? ndsFloatBits(root->translate.vec.f.y) : 0u;

    if (slot == 0u)
    {
        gNdsFighterWalkLoopP0StatusAfterHeld = (u32)fp->status_id;
        gNdsFighterWalkLoopP0MotionAfterHeld = (u32)fp->motion_id;
        gNdsFighterWalkLoopP0GAAfterHeld = (u32)fp->ga;
        gNdsFighterWalkLoopP0RootXAfterHeldBits = root_x;
        gNdsFighterWalkLoopP0RootYAfterHeldBits = root_y;
        gNdsFighterWalkLoopP0HeldRootDeltaXMilli =
            (root != NULL) ?
                ndsFloatToMilliSigned(root->translate.vec.f.x) -
                    ndsFloatToMilliSigned(ndsFloatFromBits(gNdsFighterWalkLoopP0RootXStartBits)) :
                0;
        gNdsFighterWalkLoopP0GroundVelAfterHeldMilli =
            ndsFloatToMilliSigned(fp->physics.vel_ground.x);
        gNdsFighterWalkLoopP0AirVelXAfterHeldMilli =
            ndsFloatToMilliSigned(fp->physics.vel_air.x);
        gNdsFighterWalkLoopP0AirVelYAfterHeldMilli =
            ndsFloatToMilliSigned(fp->physics.vel_air.y);
    }
    else if (slot == 1u)
    {
        gNdsFighterWalkLoopP1StatusAfterHeld = (u32)fp->status_id;
        gNdsFighterWalkLoopP1MotionAfterHeld = (u32)fp->motion_id;
        gNdsFighterWalkLoopP1GAAfterHeld = (u32)fp->ga;
        gNdsFighterWalkLoopP1RootXAfterHeldBits = root_x;
        gNdsFighterWalkLoopP1RootYAfterHeldBits = root_y;
        gNdsFighterWalkLoopP1HeldRootDeltaXMilli =
            (root != NULL) ?
                ndsFloatToMilliSigned(root->translate.vec.f.x) -
                    ndsFloatToMilliSigned(ndsFloatFromBits(gNdsFighterWalkLoopP1RootXStartBits)) :
                0;
        gNdsFighterWalkLoopP1GroundVelAfterHeldMilli =
            ndsFloatToMilliSigned(fp->physics.vel_ground.x);
        gNdsFighterWalkLoopP1AirVelXAfterHeldMilli =
            ndsFloatToMilliSigned(fp->physics.vel_air.x);
        gNdsFighterWalkLoopP1AirVelYAfterHeldMilli =
            ndsFloatToMilliSigned(fp->physics.vel_air.y);
    }
}

static void ndsFighterWalkLoopRecordAfterRelease(u32 slot, FTStruct *fp,
                                                 DObj *root)
{
    (void)root;
    if (slot == 0u)
    {
        gNdsFighterWalkLoopP0StatusAfterRelease = (u32)fp->status_id;
        gNdsFighterWalkLoopP0MotionAfterRelease = (u32)fp->motion_id;
        gNdsFighterWalkLoopP0GAAfterRelease = (u32)fp->ga;
    }
    else if (slot == 1u)
    {
        gNdsFighterWalkLoopP1StatusAfterRelease = (u32)fp->status_id;
        gNdsFighterWalkLoopP1MotionAfterRelease = (u32)fp->motion_id;
        gNdsFighterWalkLoopP1GAAfterRelease = (u32)fp->ga;
    }
}

static void ndsFighterWalkLoopRecordAfterSettle(u32 slot, FTStruct *fp,
                                                DObj *root)
{
    u32 root_x = (root != NULL) ? ndsFloatBits(root->translate.vec.f.x) : 0u;
    u32 root_y = (root != NULL) ? ndsFloatBits(root->translate.vec.f.y) : 0u;

    if (slot == 0u)
    {
        gNdsFighterWalkLoopP0StatusAfterSettle = (u32)fp->status_id;
        gNdsFighterWalkLoopP0MotionAfterSettle = (u32)fp->motion_id;
        gNdsFighterWalkLoopP0GAAfterSettle = (u32)fp->ga;
        gNdsFighterWalkLoopP0RootXAfterSettleBits = root_x;
        gNdsFighterWalkLoopP0RootYAfterSettleBits = root_y;
        gNdsFighterWalkLoopP0RootDeltaXMilli =
            (root != NULL) ?
                ndsFloatToMilliSigned(root->translate.vec.f.x) -
                    ndsFloatToMilliSigned(ndsFloatFromBits(gNdsFighterWalkLoopP0RootXStartBits)) :
                0;
        gNdsFighterWalkLoopP0GroundVelAfterSettleMilli =
            ndsFloatToMilliSigned(fp->physics.vel_ground.x);
    }
    else if (slot == 1u)
    {
        gNdsFighterWalkLoopP1StatusAfterSettle = (u32)fp->status_id;
        gNdsFighterWalkLoopP1MotionAfterSettle = (u32)fp->motion_id;
        gNdsFighterWalkLoopP1GAAfterSettle = (u32)fp->ga;
        gNdsFighterWalkLoopP1RootXAfterSettleBits = root_x;
        gNdsFighterWalkLoopP1RootYAfterSettleBits = root_y;
        gNdsFighterWalkLoopP1RootDeltaXMilli =
            (root != NULL) ?
                ndsFloatToMilliSigned(root->translate.vec.f.x) -
                    ndsFloatToMilliSigned(ndsFloatFromBits(gNdsFighterWalkLoopP1RootXStartBits)) :
                0;
        gNdsFighterWalkLoopP1GroundVelAfterSettleMilli =
            ndsFloatToMilliSigned(fp->physics.vel_ground.x);
    }
}

static void ndsFighterWalkLoopRunHeldFrame(u32 slot, FTStruct *fp)
{
    GObj *fighter_gobj;
    DObj *root;
    s32 stick_mag;
    s32 stick_x;
    s32 status_before_interrupt;

    if ((fp == NULL) || (slot >= 2u))
    {
        return;
    }
    fighter_gobj = fp->fighter_gobj;
    root = fp->joints[nFTPartsJointTopN];
    if ((fighter_gobj == NULL) || (root == NULL))
    {
        gNdsFighterWalkLoopProcessAttachCount++;
        return;
    }

    stick_mag = (slot == 0u) ? 40 : 80;
    stick_x = stick_mag * ((fp->lr >= 0) ? 1 : -1);
    fp->input.pl.stick_range.x = (s8)stick_x;
    fp->input.pl.stick_range.y = 0;
    fp->input.pl.button_hold = 0;
    fp->input.pl.button_tap = 0;

    fp->status_total_tics++;
    if (fp->proc_update != NULL)
    {
        gNdsFighterWalkLoopProcessAttachCount++;
        return;
    }

    if (fp->proc_interrupt != NULL)
    {
        status_before_interrupt = fp->status_id;
        sNdsFighterWalkLoopFrameActive = TRUE;
        fp->proc_interrupt(fighter_gobj);
        sNdsFighterWalkLoopFrameActive = FALSE;
        if (((status_before_interrupt >= nFTCommonStatusWalkSlow) &&
            (status_before_interrupt <= nFTCommonStatusWalkFast)) &&
            (fp->status_id == nFTCommonStatusWait) &&
            ((stick_x * fp->lr) >= 8))
        {
            ftMainSetStatus(fighter_gobj, status_before_interrupt,
                            fp->anim_frame, 1.0F,
                            FTSTATUS_PRESERVE_NONE);
            ftMainPlayAnimEventsAll(fighter_gobj);
            if (status_before_interrupt != nFTCommonStatusWalkFast)
            {
                fp->is_special_interrupt = TRUE;
            }
        }
        if (slot == 0u)
        {
            gNdsFighterWalkLoopP0InterruptCount++;
        }
        else
        {
            gNdsFighterWalkLoopP1InterruptCount++;
        }
    }

    fp->physics.vel_jostle_x = 0.0F;
    fp->physics.vel_jostle_z = 0.0F;
    fp->coll_data.pos_prev = root->translate.vec.f;
    ndsFighterSyncLegacyVelToPhysics(fp);

    if (fp->proc_physics != NULL)
    {
        sNdsFighterWalkLoopFrameActive = TRUE;
        fp->proc_physics(fighter_gobj);
        sNdsFighterWalkLoopFrameActive = FALSE;
        if (slot == 0u)
        {
            gNdsFighterWalkLoopP0PhysicsCount++;
        }
        else
        {
            gNdsFighterWalkLoopP1PhysicsCount++;
        }
    }

    root->translate.vec.f.x += fp->physics.vel_air.x;
    root->translate.vec.f.y += fp->physics.vel_air.y;
    root->translate.vec.f.z += fp->physics.vel_air.z;
    ndsFighterSyncPhysicsToLegacyVel(fp);
    if (slot == 0u)
    {
        gNdsFighterWalkLoopP0IntegrateCount++;
        gNdsFighterWalkLoopP0HeldFrameCount++;
    }
    else
    {
        gNdsFighterWalkLoopP1IntegrateCount++;
        gNdsFighterWalkLoopP1HeldFrameCount++;
    }

    if (fp->proc_map != NULL)
    {
        sNdsFighterWalkLoopFrameActive = TRUE;
        sNdsFighterWalkLoopMapActive = TRUE;
        fp->proc_map(fighter_gobj);
        sNdsFighterWalkLoopMapActive = FALSE;
        sNdsFighterWalkLoopFrameActive = FALSE;
    }
}

static void ndsFighterWalkLoopRunReleaseToWait(u32 slot, FTStruct *fp)
{
    GObj *fighter_gobj;

    if ((fp == NULL) || (slot >= 2u))
    {
        return;
    }
    fighter_gobj = fp->fighter_gobj;
    if (fighter_gobj == NULL)
    {
        return;
    }

    fp->input.pl.stick_range.x = 0;
    fp->input.pl.stick_range.y = 0;
    fp->input.pl.button_hold = 0;
    fp->input.pl.button_tap = 0;
    gNdsFighterWalkLoopReleaseInputCount++;

    if (fp->proc_interrupt == ftCommonWalkProcInterrupt)
    {
        gNdsFighterWalkLoopWaitReturnCheckCount++;
        sNdsFighterWalkLoopWaitReturnActive = TRUE;
        fp->proc_interrupt(fighter_gobj);
        sNdsFighterWalkLoopWaitReturnActive = FALSE;
        if ((fp->status_id == nFTCommonStatusWait) &&
            (fp->motion_id == nFTCommonMotionWait))
        {
            gNdsFighterWalkLoopWaitReturnSuccessCount++;
        }
    }
}

static void ndsFighterWalkLoopRunWaitSettleFrame(u32 slot, FTStruct *fp)
{
    GObj *fighter_gobj;

    (void)slot;
    if (fp == NULL)
    {
        return;
    }
    fighter_gobj = fp->fighter_gobj;
    if (fighter_gobj == NULL)
    {
        return;
    }

    ndsFighterSyncLegacyVelToPhysics(fp);
    if (fp->proc_physics == ftPhysicsApplyGroundVelFriction)
    {
        sNdsFighterWalkLoopWaitFrictionActive = TRUE;
        fp->proc_physics(fighter_gobj);
        sNdsFighterWalkLoopWaitFrictionActive = FALSE;
        ndsFighterSyncPhysicsToLegacyVel(fp);
    }
    if (fp->proc_map == mpCommonProcFighterOnCliffEdge)
    {
        sNdsFighterWalkLoopMapActive = TRUE;
        fp->proc_map(fighter_gobj);
        sNdsFighterWalkLoopMapActive = FALSE;
    }
}

static void ndsFighterMarioFoxRunWalkLoopProof(void)
{
    u32 mask = 0u;
    u32 gobj_before;
    u32 gobj_after;
    u32 i;

    if ((ndsFighterMarioFoxWalkLoopProofEnabled() == FALSE) ||
        (gNdsFighterMarioFoxWalkLoopResult != 0u))
    {
        return;
    }
    if ((gNdsFighterMarioFoxWalkInputResult ==
            NDS_FIGHTER_MARIOFOX_WALK_INPUT_PASS) &&
        (gNdsFighterMarioFoxWalkSafeResult ==
            NDS_FIGHTER_MARIOFOX_WALK_SAFE_PASS) &&
        ((gNdsFighterMarioFoxWalkInputMask & 0x7ffu) == 0x7ffu) &&
        (gNdsFighterMarioFoxWalkDeferredMask == 0xffu) &&
        (gNdsFighterMarioFoxWalkInputCount == 2u))
    {
        mask |= 1u << 0;
    }
    else
    {
        gNdsFighterMarioFoxWalkLoopMask = mask;
        return;
    }

    gNdsFighterWalkLoopFrameTarget = 4u;
    gobj_before = (u32)gcGetGObjsActiveNum();

    for (i = 0u; i < 2u; i++)
    {
        FTStruct *fp = &sNdsFighterStructPool[i];
        DObj *root;
        s32 stick_mag = (i == 0u) ? 40 : 80;
        s32 stick_x;
        u32 frame;

        if ((ndsFighterStructIsPoolPointer(fp) == FALSE) ||
            (fp->fighter_gobj == NULL) || (fp->attr == NULL))
        {
            continue;
        }
        root = fp->joints[nFTPartsJointTopN];
        if (root == NULL)
        {
            continue;
        }
        stick_x = stick_mag * ((fp->lr >= 0) ? 1 : -1);
        fp->input.pl.stick_range.x = (s8)stick_x;
        fp->input.pl.stick_range.y = 0;
        fp->input.pl.button_hold = 0;
        fp->input.pl.button_tap = 0;

        if (i == 0u)
        {
            gNdsFighterWalkLoopP0StickX = stick_x;
            gNdsFighterWalkLoopP0StickAbs = (u32)ABS(stick_x);
            gNdsFighterWalkLoopP0LR = fp->lr;
        }
        else
        {
            gNdsFighterWalkLoopP1StickX = stick_x;
            gNdsFighterWalkLoopP1StickAbs = (u32)ABS(stick_x);
            gNdsFighterWalkLoopP1LR = fp->lr;
        }

        ndsFighterWalkLoopRecordStart(i, fp, root);
        for (frame = 0u; frame < gNdsFighterWalkLoopFrameTarget; frame++)
        {
            ndsFighterWalkLoopRunHeldFrame(i, fp);
        }
        ndsFighterWalkLoopRecordAfterHeld(i, fp, root);
        ndsFighterWalkLoopRunReleaseToWait(i, fp);
        ndsFighterWalkLoopRecordAfterRelease(i, fp, root);
        ndsFighterWalkLoopRunWaitSettleFrame(i, fp);
        ndsFighterWalkLoopRecordAfterSettle(i, fp, root);

        if (i == 0u)
        {
            if ((gNdsFighterWalkLoopP0HeldRootDeltaXMilli *
                    gNdsFighterWalkLoopP0LR) > 0)
            {
                gNdsFighterWalkLoopP0RootDirectionOK = 1u;
            }
            if ((gNdsFighterWalkLoopP0RootYStartBits !=
                    gNdsFighterWalkLoopP0RootYAfterHeldBits) ||
                (gNdsFighterWalkLoopP0RootYStartBits !=
                    gNdsFighterWalkLoopP0RootYAfterSettleBits))
            {
                gNdsFighterWalkLoopRootYDriftCount++;
            }
            if ((gNdsFighterWalkLoopP0GAStart != (u32)nMPKineticsGround) ||
                (gNdsFighterWalkLoopP0GAAfterHeld !=
                    (u32)nMPKineticsGround) ||
                (gNdsFighterWalkLoopP0GAAfterRelease !=
                    (u32)nMPKineticsGround) ||
                (gNdsFighterWalkLoopP0GAAfterSettle !=
                    (u32)nMPKineticsGround))
            {
                gNdsFighterWalkLoopGADriftCount++;
            }
        }
        else
        {
            if ((gNdsFighterWalkLoopP1HeldRootDeltaXMilli *
                    gNdsFighterWalkLoopP1LR) > 0)
            {
                gNdsFighterWalkLoopP1RootDirectionOK = 1u;
            }
            if ((gNdsFighterWalkLoopP1RootYStartBits !=
                    gNdsFighterWalkLoopP1RootYAfterHeldBits) ||
                (gNdsFighterWalkLoopP1RootYStartBits !=
                    gNdsFighterWalkLoopP1RootYAfterSettleBits))
            {
                gNdsFighterWalkLoopRootYDriftCount++;
            }
            if ((gNdsFighterWalkLoopP1GAStart != (u32)nMPKineticsGround) ||
                (gNdsFighterWalkLoopP1GAAfterHeld !=
                    (u32)nMPKineticsGround) ||
                (gNdsFighterWalkLoopP1GAAfterRelease !=
                    (u32)nMPKineticsGround) ||
                (gNdsFighterWalkLoopP1GAAfterSettle !=
                    (u32)nMPKineticsGround))
            {
                gNdsFighterWalkLoopGADriftCount++;
            }
        }
        gNdsFighterMarioFoxWalkLoopCount++;
    }

    if ((gNdsFighterWalkLoopP0StickAbs == 40u) &&
        (gNdsFighterWalkLoopP1StickAbs == 80u))
    {
        mask |= 1u << 1;
    }
    if ((gNdsFighterWalkLoopP0HeldFrameCount == 4u) &&
        (gNdsFighterWalkLoopP1HeldFrameCount == 4u))
    {
        mask |= 1u << 2;
    }
    if ((gNdsFighterWalkLoopP0InterruptCount == 4u) &&
        (gNdsFighterWalkLoopP1InterruptCount == 4u))
    {
        mask |= 1u << 3;
    }
    if ((gNdsFighterWalkLoopP0PhysicsCount == 4u) &&
        (gNdsFighterWalkLoopP1PhysicsCount == 4u) &&
        (gNdsFighterWalkLoopGroundVelAbsStickCount == 8u) &&
        (gNdsFighterWalkLoopGroundVelTransferAirCount >= 8u))
    {
        mask |= 1u << 4;
    }
    if ((gNdsFighterWalkLoopP0IntegrateCount == 4u) &&
        (gNdsFighterWalkLoopP1IntegrateCount == 4u))
    {
        mask |= 1u << 5;
    }
    if ((gNdsFighterWalkLoopP0MapCount == 4u) &&
        (gNdsFighterWalkLoopP1MapCount == 4u) &&
        (gNdsFighterWalkLoopP0SafeFloorCount == 4u) &&
        (gNdsFighterWalkLoopP1SafeFloorCount == 4u))
    {
        mask |= 1u << 6;
    }
    if ((gNdsFighterWalkLoopWaitReturnCheckCount == 2u) &&
        (gNdsFighterWalkLoopWaitReturnSuccessCount == 2u) &&
        (gNdsFighterWalkLoopWaitSetStatusCount == 2u) &&
        (gNdsFighterWalkLoopReleaseInputCount == 2u) &&
        (gNdsFighterWalkLoopP0StatusAfterRelease ==
            (u32)nFTCommonStatusWait) &&
        (gNdsFighterWalkLoopP1StatusAfterRelease ==
            (u32)nFTCommonStatusWait) &&
        (gNdsFighterWalkLoopP0MotionAfterRelease ==
            (u32)nFTCommonMotionWait) &&
        (gNdsFighterWalkLoopP1MotionAfterRelease ==
            (u32)nFTCommonMotionWait))
    {
        mask |= 1u << 7;
    }
    if ((gNdsFighterWalkLoopWaitFrictionCount == 2u) &&
        (gNdsFighterWalkLoopP0GroundVelAfterSettleMilli <
            gNdsFighterWalkLoopP0GroundVelAfterHeldMilli) &&
        (gNdsFighterWalkLoopP1GroundVelAfterSettleMilli <
            gNdsFighterWalkLoopP1GroundVelAfterHeldMilli) &&
        (gNdsFighterWalkLoopP0StatusAfterSettle ==
            (u32)nFTCommonStatusWait) &&
        (gNdsFighterWalkLoopP1StatusAfterSettle ==
            (u32)nFTCommonStatusWait))
    {
        mask |= 1u << 8;
    }
    if ((gNdsFighterWalkLoopP0RootDirectionOK == 1u) &&
        (gNdsFighterWalkLoopP1RootDirectionOK == 1u) &&
        (gNdsFighterWalkLoopP0HeldRootDeltaXMilli != 0) &&
        (gNdsFighterWalkLoopP1HeldRootDeltaXMilli != 0) &&
        (gNdsFighterWalkLoopP0RootDeltaXMilli != 0) &&
        (gNdsFighterWalkLoopP1RootDeltaXMilli != 0))
    {
        mask |= 1u << 9;
    }

    gobj_after = (u32)gcGetGObjsActiveNum();
    gNdsFighterWalkLoopGObjDelta =
        (gobj_after >= gobj_before) ? (gobj_after - gobj_before) :
            (gobj_before - gobj_after);
    if ((gNdsFighterWalkLoopMapSafeFloorCount >= 10u) &&
        (gNdsFighterWalkLoopMapFallDeniedCount == 0u) &&
        (gNdsFighterWalkLoopMapOttottoDeniedCount == 0u) &&
        (gNdsFighterWalkLoopGObjDelta == 0u) &&
        (gNdsFighterWalkLoopUnexpectedStatusCount == 0u) &&
        (gNdsFighterWalkLoopDeniedStatusCount == 0u) &&
        (gNdsFighterWalkLoopProcessAttachCount == 0u) &&
        (gNdsFighterWalkLoopDisplayProbeCount == 0u) &&
        (gNdsFighterWalkLoopGameplayUpdateCount == 0u) &&
        (gNdsFighterWalkLoopDrawCallCount == 0u) &&
        (gNdsFighterWalkLoopMatrixCallCount == 0u) &&
        (gNdsFighterWalkLoopRootYDriftCount == 0u) &&
        (gNdsFighterWalkLoopGADriftCount == 0u))
    {
        mask |= 1u << 10;
    }

    gNdsFighterMarioFoxWalkLoopMask = mask;
    gNdsFighterMarioFoxWalkLoopDeferredMask = 0xffu;
    if ((mask & 0x7ffu) == 0x7ffu)
    {
        gNdsFighterMarioFoxWalkLoopResult =
            NDS_FIGHTER_MARIOFOX_WALK_LOOP_PASS;
        gNdsFighterMarioFoxWalkLoopSafeResult =
            NDS_FIGHTER_MARIOFOX_WALK_LOOP_SAFE_PASS;
    }
    ndsFighterMarioFoxRunDashRunProof();
}

static void ndsFighterDashRunRunGuardProof(u32 slot, FTStruct *fp)
{
    GObj *fighter_gobj;
    FTAttributes *attr;
    AObjEvent32 **saved_shield_anim_joints[8];
    DObjDesc *saved_dobj_lookup;
    DObj *topn_joint;
    DObj *xrotn_joint;
    DObj *yrotn_joint;
    DObj *saved_xrotn_joint;
    DObj *saved_yrotn_joint;
    Vec3f saved_yrotn_scale;
    Vec3f saved_yrotn_rotate;
    Vec3f saved_yrotn_translate;
    f32 saved_yrotn_anim_wait = 0.0F;
    f32 saved_anim_frame;
    Vec2b saved_stick_range;
    u16 saved_button_tap;
    u16 saved_button_hold;
    u8 saved_tap_stick_x;
    s32 saved_lr;
    s32 saved_shield_health;
    s32 saved_shield_damage;
    s32 saved_shield_lr;
    sb32 saved_is_shield;
    sb32 saved_have_translate_scale;
    u32 angle;
    sb32 result;

    if ((fp == NULL) || (slot >= 2u) || (fp->fighter_gobj == NULL) ||
        (fp->attr == NULL))
    {
        return;
    }

    fighter_gobj = fp->fighter_gobj;
    attr = fp->attr;
    topn_joint = fp->joints[nFTPartsJointTopN];
    if (topn_joint == NULL)
    {
        gNdsFighterDashRunProcessAttachCount++;
        return;
    }
    saved_xrotn_joint = fp->joints[nFTPartsJointXRotN];
    saved_yrotn_joint = fp->joints[nFTPartsJointYRotN];
    xrotn_joint = saved_xrotn_joint;
    yrotn_joint = fp->joints[nFTPartsJointYRotN];
    if (xrotn_joint == NULL)
    {
        xrotn_joint = (topn_joint->child != NULL) ? topn_joint->child :
            topn_joint;
        fp->joints[nFTPartsJointXRotN] = xrotn_joint;
    }
    if (yrotn_joint == NULL)
    {
        yrotn_joint = (xrotn_joint->child != NULL) ? xrotn_joint->child :
            xrotn_joint;
        fp->joints[nFTPartsJointYRotN] = yrotn_joint;
    }
    if ((yrotn_joint == NULL) || (xrotn_joint == NULL))
    {
        gNdsFighterDashRunProcessAttachCount++;
        fp->joints[nFTPartsJointXRotN] = saved_xrotn_joint;
        fp->joints[nFTPartsJointYRotN] = saved_yrotn_joint;
        return;
    }

    for (angle = 0u; angle < 8u; angle++)
    {
        saved_shield_anim_joints[angle] = attr->shield_anim_joints[angle];
        attr->shield_anim_joints[angle] =
            sNdsFighterDashRunGuardAnimJoints;
    }
    saved_dobj_lookup = attr->dobj_lookup;
    saved_have_translate_scale = fp->is_have_translate_scale;
    attr->dobj_lookup = sNdsFighterDashRunGuardDObjLookup;
    fp->is_have_translate_scale = FALSE;
    if (attr->shield_size <= 0.0F)
    {
        attr->shield_size = 30.0F;
    }

    saved_yrotn_scale = yrotn_joint->scale.vec.f;
    saved_yrotn_rotate = yrotn_joint->rotate.vec.f;
    saved_yrotn_translate = yrotn_joint->translate.vec.f;
    saved_yrotn_anim_wait = yrotn_joint->anim_wait;
    saved_anim_frame = fighter_gobj->anim_frame;
    saved_stick_range = fp->input.pl.stick_range;
    saved_button_tap = fp->input.pl.button_tap;
    saved_button_hold = fp->input.pl.button_hold;
    saved_tap_stick_x = fp->tap_stick_x;
    saved_lr = fp->lr;
    saved_shield_health = fp->shield_health;
    saved_shield_damage = fp->shield_damage;
    saved_shield_lr = fp->shield_lr;
    saved_is_shield = fp->is_shield;

    if (fp->input.button_mask_z == 0u)
    {
        fp->input.button_mask_z = Z_TRIG;
    }

    ftCommonWaitSetStatus(fighter_gobj);
    fp->shield_health = 55;
    fp->is_shield = FALSE;
    fp->input.pl.stick_range.x = 0;
    fp->input.pl.stick_range.y = 0;
    fp->input.pl.button_tap = 0;
    fp->input.pl.button_hold = fp->input.button_mask_z;

    sNdsFighterDashRunGuardOnActive = TRUE;
    result = ftCommonGuardOnCheckInterruptCommon(fighter_gobj);
    sNdsFighterDashRunGuardOnActive = FALSE;

    if (slot == 0u)
    {
        gNdsFighterDashRunP0StatusGuardOn = (u32)fp->status_id;
        gNdsFighterDashRunP0MotionGuardOn = (u32)fp->motion_id;
    }
    else
    {
        gNdsFighterDashRunP1StatusGuardOn = (u32)fp->status_id;
        gNdsFighterDashRunP1MotionGuardOn = (u32)fp->motion_id;
    }

    if (fp->proc_update == ndsBaseFTCommonGuardOnProcUpdate)
    {
        gNdsFighterDashRunGuardCallbackMask |= 1u << ((slot * 4u) + 0u);
    }
    if (fp->proc_interrupt == ndsBaseFTCommonGuardCommonProcInterrupt)
    {
        gNdsFighterDashRunGuardCallbackMask |= 1u << ((slot * 4u) + 1u);
    }
    if (fp->proc_physics == ftPhysicsApplyGroundVelFriction)
    {
        gNdsFighterDashRunGuardCallbackMask |= 1u << ((slot * 4u) + 2u);
    }
    if (fp->proc_map == mpCommonSetFighterFallOnGroundBreak)
    {
        gNdsFighterDashRunGuardCallbackMask |= 1u << ((slot * 4u) + 3u);
    }

    if ((result != FALSE) &&
        (fp->status_id == nFTCommonStatusGuardOn) &&
        (fp->motion_id == nFTCommonMotionGuardOn))
    {
        gNdsFighterDashRunGuardStateMask |= 1u << slot;
    }
    if ((fp->is_shield != FALSE) &&
        (fp->shield_health == 55) &&
        (fp->status_vars.common.guard.release_lag ==
            FTCOMMON_GUARD_RELEASE_LAG) &&
        (fp->status_vars.common.guard.shield_decay_wait ==
            FTCOMMON_GUARD_DECAY_INT) &&
        (fp->status_vars.common.guard.is_release == FALSE) &&
        (fp->status_vars.common.guard.slide_tics == 0) &&
        (fp->status_vars.common.guard.is_setoff == FALSE))
    {
        gNdsFighterDashRunGuardStateMask |= 1u << (slot + 2u);
    }

    if (fp->proc_update == ndsBaseFTCommonGuardOnProcUpdate)
    {
        fighter_gobj->anim_frame = 2.0F;
        sNdsFighterDashRunGuardOnActive = TRUE;
        fp->proc_update(fighter_gobj);
        sNdsFighterDashRunGuardOnActive = FALSE;

        if ((fp->status_id == nFTCommonStatusGuardOn) &&
            (fp->motion_id == nFTCommonMotionGuardOn) &&
            (fp->is_shield != FALSE))
        {
            gNdsFighterDashRunGuardStateMask |= 1u << (slot + 10u);
        }
        if ((fp->shield_health == 55) &&
            (fp->status_vars.common.guard.shield_decay_wait ==
                (FTCOMMON_GUARD_DECAY_INT - 1)) &&
            (fp->status_vars.common.guard.release_lag ==
                (FTCOMMON_GUARD_RELEASE_LAG - 1)) &&
            (fp->status_vars.common.guard.is_release == FALSE))
        {
            gNdsFighterDashRunGuardStateMask |= 1u << (slot + 12u);
        }

        fighter_gobj->anim_frame = 0.0F;
        sNdsFighterDashRunGuardOnActive = TRUE;
        fp->proc_update(fighter_gobj);
        sNdsFighterDashRunGuardOnActive = FALSE;

        if ((fp->status_id == nFTCommonStatusGuard) &&
            (fp->motion_id == nFTCommonMotionGuardOn) &&
            (fp->is_shield != FALSE))
        {
            gNdsFighterDashRunGuardStateMask |= 1u << (slot + 14u);
        }
        if ((fp->proc_update == ndsBaseFTCommonGuardProcUpdate) &&
            (fp->proc_interrupt == ndsBaseFTCommonGuardCommonProcInterrupt) &&
            (fp->proc_physics == ftPhysicsApplyGroundVelFriction) &&
            (fp->proc_map == mpCommonSetFighterFallOnGroundBreak))
        {
            gNdsFighterDashRunGuardStateMask |= 1u << (slot + 16u);
        }

        if (fp->proc_update == ndsBaseFTCommonGuardProcUpdate)
        {
            fp->proc_update(fighter_gobj);

            if ((fp->status_id == nFTCommonStatusGuard) &&
                (fp->motion_id == nFTCommonMotionGuardOn) &&
                (fp->is_shield != FALSE) &&
                (fp->status_vars.common.guard.is_release == FALSE))
            {
                gNdsFighterDashRunGuardStateMask |= 1u << (slot + 18u);
            }
            if ((fp->shield_health == 55) &&
                (fp->status_vars.common.guard.shield_decay_wait ==
                    (FTCOMMON_GUARD_DECAY_INT - 3)) &&
                (fp->status_vars.common.guard.release_lag ==
                    (FTCOMMON_GUARD_RELEASE_LAG - 3)))
            {
                gNdsFighterDashRunGuardStateMask |= 1u << (slot + 20u);
            }

            fp->lr = 1;
            fp->shield_lr = 1;
            fp->shield_damage = 10;

            sNdsFighterDashRunGuardOnActive = TRUE;
            ndsBaseFTCommonGuardSetOffSetStatus(fighter_gobj);
            sNdsFighterDashRunGuardOnActive = FALSE;

            gNdsFighterDashRunGuardSetOffFramesMilli =
                ndsFloatToMilliSigned(
                    fp->status_vars.common.guard.setoff_frames);
            gNdsFighterDashRunGuardSetOffVelMilli =
                ndsFloatToMilliSigned(fp->physics.vel_ground.x);

            if ((fp->status_id == nFTCommonStatusGuardSetOff) &&
                (fp->motion_id == nFTCommonMotionGuardOn) &&
                (fp->is_shield != FALSE) &&
                (fp->status_vars.common.guard.is_setoff != FALSE))
            {
                gNdsFighterDashRunGuardSetOffMask |= 1u << slot;
            }
            if ((gNdsFighterDashRunGuardSetOffFramesMilli == 20200) &&
                (gNdsFighterDashRunGuardSetOffVelMilli == -40400))
            {
                gNdsFighterDashRunGuardSetOffMask |= 1u << (slot + 2u);
            }
            if (fp->proc_update == ndsBaseFTCommonGuardSetOffProcUpdate)
            {
                gNdsFighterDashRunGuardSetOffCallbackMask |=
                    1u << ((slot * 4u) + 0u);
            }
            if (fp->proc_interrupt == NULL)
            {
                gNdsFighterDashRunGuardSetOffCallbackMask |=
                    1u << ((slot * 4u) + 1u);
            }
            if (fp->proc_physics == ftPhysicsApplyGroundVelFriction)
            {
                gNdsFighterDashRunGuardSetOffCallbackMask |=
                    1u << ((slot * 4u) + 2u);
            }
            if (fp->proc_map == mpCommonSetFighterFallOnGroundBreak)
            {
                gNdsFighterDashRunGuardSetOffCallbackMask |=
                    1u << ((slot * 4u) + 3u);
            }
            if (((gNdsFighterDashRunGuardSetOffCallbackMask >>
                    (slot * 4u)) & 0xfu) == 0xfu)
            {
                gNdsFighterDashRunGuardSetOffMask |= 1u << (slot + 4u);
            }

            if (fp->proc_update == ndsBaseFTCommonGuardSetOffProcUpdate)
            {
                fp->status_vars.common.guard.setoff_frames = 1.0F;
                fp->input.pl.button_hold = fp->input.button_mask_z;
                sNdsFighterDashRunGuardOnActive = TRUE;
                fp->proc_update(fighter_gobj);
                sNdsFighterDashRunGuardOnActive = FALSE;

                if ((fp->status_id == nFTCommonStatusGuard) &&
                    (fp->motion_id == nFTCommonMotionGuardOn) &&
                    (fp->is_shield != FALSE) &&
                    (fp->proc_update == ndsBaseFTCommonGuardProcUpdate))
                {
                    gNdsFighterDashRunGuardSetOffMask |= 1u << (slot + 6u);
                }
            }

            sNdsFighterDashRunGuardOnActive = TRUE;
            ndsBaseFTCommonGuardSetOffSetStatus(fighter_gobj);
            sNdsFighterDashRunGuardOnActive = FALSE;

            if (fp->proc_update == ndsBaseFTCommonGuardSetOffProcUpdate)
            {
                fp->status_vars.common.guard.setoff_frames = 1.0F;
                fp->input.pl.button_hold = 0;
                sNdsFighterDashRunGuardOnActive = TRUE;
                fp->proc_update(fighter_gobj);
                sNdsFighterDashRunGuardOnActive = FALSE;

                if ((fp->status_id == nFTCommonStatusGuardOff) &&
                    (fp->motion_id == nFTCommonMotionGuardOff) &&
                    (fp->is_shield != FALSE) &&
                    (fp->proc_update == ndsBaseFTCommonGuardOffProcUpdate))
                {
                    gNdsFighterDashRunGuardSetOffMask |= 1u << (slot + 8u);
                }
            }

            sNdsFighterDashRunGuardOnActive = TRUE;
            ndsBaseFTCommonGuardSetStatus(fighter_gobj);
            sNdsFighterDashRunGuardOnActive = FALSE;

            if ((fp->status_id == nFTCommonStatusGuard) &&
                (fp->motion_id == nFTCommonMotionGuardOn) &&
                (fp->proc_update == ndsBaseFTCommonGuardProcUpdate))
            {
                gNdsFighterDashRunGuardSetOffMask |= 1u << (slot + 10u);
            }
            fp->input.pl.button_hold = fp->input.button_mask_z;

            fp->input.pl.button_hold = 0;
            fp->proc_update(fighter_gobj);

            if ((fp->status_id == nFTCommonStatusGuardOff) &&
                (fp->motion_id == nFTCommonMotionGuardOff) &&
                (fp->is_shield != FALSE))
            {
                gNdsFighterDashRunGuardStateMask |= 1u << (slot + 22u);
            }
            if ((fp->proc_update == ndsBaseFTCommonGuardOffProcUpdate) &&
                (fp->proc_interrupt == NULL) &&
                (fp->proc_physics == ftPhysicsApplyGroundVelFriction) &&
                (fp->proc_map == mpCommonSetFighterFallOnGroundBreak))
            {
                gNdsFighterDashRunGuardStateMask |= 1u << (slot + 24u);
            }
            if ((fp->shield_health == 55) &&
                (fp->status_vars.common.guard.shield_decay_wait ==
                    (FTCOMMON_GUARD_DECAY_INT - 4)) &&
                (fp->status_vars.common.guard.release_lag ==
                    (FTCOMMON_GUARD_RELEASE_LAG - 4)) &&
                (fp->status_vars.common.guard.is_release != FALSE))
            {
                gNdsFighterDashRunGuardStateMask |= 1u << (slot + 26u);
            }

            if (fp->proc_update == ndsBaseFTCommonGuardOffProcUpdate)
            {
                fighter_gobj->anim_frame = 0.0F;
                fp->proc_update(fighter_gobj);

                if ((fp->status_id == nFTCommonStatusWait) &&
                    (fp->motion_id == nFTCommonMotionWait) &&
                    (fp->is_shield == FALSE))
                {
                    gNdsFighterDashRunGuardStateMask |= 1u << (slot + 28u);
                }
                if ((fp->proc_update == NULL) &&
                    (fp->proc_interrupt == ftCommonWaitProcInterrupt) &&
                    (fp->proc_physics == ftPhysicsApplyGroundVelFriction) &&
                    (fp->proc_map == mpCommonProcFighterOnCliffEdge))
                {
                    gNdsFighterDashRunGuardStateMask |= 1u << (slot + 30u);
                }
            }

            sNdsFighterDashRunGuardOnActive = TRUE;
            ndsBaseFTCommonGuardSetStatus(fighter_gobj);
            sNdsFighterDashRunGuardOnActive = FALSE;
            fp->input.pl.button_hold = fp->input.button_mask_z;
        }
    }

    fp->lr = 1;
    fp->tap_stick_x = 0;
    fp->input.pl.stick_range.x = (slot == 0u) ? 80 : -80;
    fp->input.pl.stick_range.y = 0;
    fp->input.pl.button_tap = 0;
    fp->input.pl.button_hold = fp->input.button_mask_z;

    sNdsFighterDashRunEscapeActive = TRUE;
    result = ftCommonEscapeCheckInterruptGuard(fighter_gobj);
    sNdsFighterDashRunEscapeActive = FALSE;

    if (slot == 0u)
    {
        gNdsFighterDashRunP0StatusEscape = (u32)fp->status_id;
        gNdsFighterDashRunP0MotionEscape = (u32)fp->motion_id;
        gNdsFighterDashRunP0EscapeItemThrowBuffer =
            (u32)fp->status_vars.common.escape.itemthrow_buffer_tics;
    }
    else
    {
        gNdsFighterDashRunP1StatusEscape = (u32)fp->status_id;
        gNdsFighterDashRunP1MotionEscape = (u32)fp->motion_id;
        gNdsFighterDashRunP1EscapeItemThrowBuffer =
            (u32)fp->status_vars.common.escape.itemthrow_buffer_tics;
    }

    if (fp->proc_update == ndsBaseFTCommonEscapeProcUpdate)
    {
        gNdsFighterDashRunEscapeCallbackMask |= 1u << ((slot * 5u) + 0u);
    }
    if (fp->proc_interrupt == ndsBaseFTCommonEscapeProcInterrupt)
    {
        gNdsFighterDashRunEscapeCallbackMask |= 1u << ((slot * 5u) + 1u);
    }
    if (fp->proc_physics == ftPhysicsApplyGroundVelTransN)
    {
        gNdsFighterDashRunEscapeCallbackMask |= 1u << ((slot * 5u) + 2u);
    }
    if (fp->proc_map == mpCommonSetFighterFallOnEdgeBreak)
    {
        gNdsFighterDashRunEscapeCallbackMask |= 1u << ((slot * 5u) + 3u);
    }
    if (fp->proc_status == ndsBaseFTCommonEscapeProcStatus)
    {
        gNdsFighterDashRunEscapeCallbackMask |= 1u << ((slot * 5u) + 4u);
    }

    if ((result != FALSE) &&
        (fp->status_id == ((slot == 0u) ? nFTCommonStatusEscapeF :
            nFTCommonStatusEscapeB)) &&
        (fp->motion_id == ((slot == 0u) ? nFTCommonMotionEscapeF :
            nFTCommonMotionEscapeB)))
    {
        gNdsFighterDashRunEscapeStateMask |= 1u << slot;
    }
    if (fp->is_jostle_ignore != FALSE)
    {
        gNdsFighterDashRunEscapeStateMask |= 1u << (slot + 2u);
    }
    if (fp->status_vars.common.escape.itemthrow_buffer_tics == 5)
    {
        gNdsFighterDashRunEscapeStateMask |= 1u << (slot + 4u);
    }
    if (sNdsFighterDashRunEscapeActive == FALSE)
    {
        gNdsFighterDashRunEscapeStateMask |= 1u << (slot + 6u);
    }
    if (fp->proc_update == ndsBaseFTCommonEscapeProcUpdate)
    {
        s32 lr_before = fp->lr;

        fighter_gobj->anim_frame = 2.0F;
        fp->motion_vars.flags.flag1 = 1;
        sNdsFighterDashRunEscapeActive = TRUE;
        fp->proc_update(fighter_gobj);
        sNdsFighterDashRunEscapeActive = FALSE;

        if ((fp->status_id == ((slot == 0u) ? nFTCommonStatusEscapeF :
                nFTCommonStatusEscapeB)) &&
            (fp->motion_id == ((slot == 0u) ? nFTCommonMotionEscapeF :
                nFTCommonMotionEscapeB)) &&
            (fp->motion_vars.flags.flag1 == 0) &&
            (fp->lr == -lr_before))
        {
            gNdsFighterDashRunEscapeTickMask |= 1u << ((slot * 5u) + 0u);
        }
    }
    if (fp->proc_interrupt == ndsBaseFTCommonEscapeProcInterrupt)
    {
        u32 interrupt_count = gNdsFighterDashRunEscapeInterruptCount;

        sNdsFighterDashRunEscapeInterruptActive = TRUE;
        fp->proc_interrupt(fighter_gobj);
        sNdsFighterDashRunEscapeInterruptActive = FALSE;

        if (gNdsFighterDashRunEscapeInterruptCount == (interrupt_count + 1u))
        {
            gNdsFighterDashRunEscapeTickMask |= 1u << ((slot * 5u) + 1u);
        }
    }
    if (fp->proc_physics == ftPhysicsApplyGroundVelTransN)
    {
        u32 physics_count = gNdsFighterDashRunEscapePhysicsCount;

        sNdsFighterDashRunEscapePhysicsActive = TRUE;
        fp->proc_physics(fighter_gobj);
        sNdsFighterDashRunEscapePhysicsActive = FALSE;

        if (gNdsFighterDashRunEscapePhysicsCount == (physics_count + 1u))
        {
            gNdsFighterDashRunEscapeTickMask |= 1u << ((slot * 5u) + 2u);
        }
    }
    if (fp->proc_map == mpCommonSetFighterFallOnEdgeBreak)
    {
        u32 map_count = gNdsFighterDashRunEscapeMapCount;

        sNdsFighterDashRunEscapeMapActive = TRUE;
        fp->proc_map(fighter_gobj);
        sNdsFighterDashRunEscapeMapActive = FALSE;

        if (gNdsFighterDashRunEscapeMapCount == (map_count + 1u))
        {
            gNdsFighterDashRunEscapeTickMask |= 1u << ((slot * 5u) + 3u);
        }
    }
    if (fp->proc_update == ndsBaseFTCommonEscapeProcUpdate)
    {
        fighter_gobj->anim_frame = 0.0F;
        sNdsFighterDashRunEscapeActive = TRUE;
        fp->proc_update(fighter_gobj);
        sNdsFighterDashRunEscapeActive = FALSE;

        if ((fp->status_id == nFTCommonStatusWait) &&
            (fp->motion_id == nFTCommonMotionWait) &&
            (fp->proc_update == NULL) &&
            (fp->proc_interrupt == ftCommonWaitProcInterrupt) &&
            (fp->proc_physics == ftPhysicsApplyGroundVelFriction) &&
            (fp->proc_map == mpCommonProcFighterOnCliffEdge))
        {
            gNdsFighterDashRunEscapeTickMask |= 1u << ((slot * 5u) + 4u);
        }
    }

    ftCommonWaitSetStatus(fighter_gobj);
    fp->input.pl.stick_range = saved_stick_range;
    fp->input.pl.button_tap = saved_button_tap;
    fp->input.pl.button_hold = saved_button_hold;
    fp->tap_stick_x = saved_tap_stick_x;
    fp->lr = saved_lr;
    fighter_gobj->anim_frame = saved_anim_frame;
    fp->shield_health = saved_shield_health;
    fp->shield_damage = saved_shield_damage;
    fp->shield_lr = saved_shield_lr;
    fp->is_shield = saved_is_shield;
    fp->is_have_translate_scale = saved_have_translate_scale;
    attr->dobj_lookup = saved_dobj_lookup;
    for (angle = 0u; angle < 8u; angle++)
    {
        attr->shield_anim_joints[angle] = saved_shield_anim_joints[angle];
    }
    yrotn_joint->scale.vec.f = saved_yrotn_scale;
    yrotn_joint->rotate.vec.f = saved_yrotn_rotate;
    yrotn_joint->translate.vec.f = saved_yrotn_translate;
    yrotn_joint->anim_wait = saved_yrotn_anim_wait;
    fp->joints[nFTPartsJointXRotN] = saved_xrotn_joint;
    fp->joints[nFTPartsJointYRotN] = saved_yrotn_joint;
}

static void ndsFighterMarioFoxSeedRunBrakeForJumpLoop(void)
{
    u32 i;

    if (ndsFighterMarioFoxJumpLoopProofEnabled() == FALSE)
    {
        return;
    }

    for (i = 0u; i < 2u; i++)
    {
        FTStruct *fp = &sNdsFighterStructPool[i];
        GObj *fighter_gobj;
        DObj *root;

        if ((ndsFighterStructIsPoolPointer(fp) == FALSE) ||
            (fp->fighter_gobj == NULL))
        {
            continue;
        }

        fighter_gobj = fp->fighter_gobj;
        root = DObjGetStruct(fighter_gobj);

        ndsFTMainApplyCommonStatusReset(fp, FTSTATUS_PRESERVE_NONE);
        fp->status_prev = nFTCommonStatusRun;
        fp->status_id = nFTCommonStatusRunBrake;
        fp->status_total_tics = 0;
        fp->motion_id = nFTCommonMotionRunBrake;
        fp->motion_script_id = nFTCommonMotionRunBrake;
        fp->motion_attack_id = nFTMotionAttackIDNone;
        fp->status_attack_id = nFTStatusAttackIDNone;
        fp->stat_attack_id = nFTStatusAttackIDNone;
        fp->status_is_smash = FALSE;
        fp->status_is_projectile = FALSE;
        fp->status_flags = FTSTATUS_PRESERVE_NONE;
        fp->motion_frame = 0.0F;
        fp->anim_frame = 0.0F;
        fp->anim_speed = 1.0F;
        fp->motion_vars.word = 0u;
        fp->proc_update = NULL;
        fp->proc_status = NULL;
        fp->proc_interrupt = ftCommonRunBrakeProcInterrupt;
        fp->proc_physics = ftCommonRunBrakeProcPhysics;
        fp->proc_map = mpCommonProcFighterOnCliffEdge;
        fp->ga = nMPKineticsGround;
        fp->coll_data.floor_line_id = 0;
        fp->coll_data.floor_flags = nMPMaterialCommon;
        fp->coll_data.floor_angle.x = 0.0F;
        fp->coll_data.floor_angle.y = 1.0F;
        fp->coll_data.floor_angle.z = 0.0F;
        fp->coll_data.mask_stat = MAP_FLAG_FLOOR;
        fp->coll_data.is_coll_end = FALSE;
        fp->input.pl.stick_range.x = 0;
        fp->input.pl.stick_range.y = 0;
        fp->input.pl.button_hold = 0;
        fp->input.pl.button_tap = 0;
        fp->input.pl.button_release = 0;
        fp->tap_stick_x = 0;
        fp->tap_stick_y = 0;
        fp->hold_stick_x = 0;
        fp->hold_stick_y = 0;
        fighter_gobj->anim_frame = 0.0F;

        if (root != NULL)
        {
            root->anim_speed = 1.0F;
        }
    }
}

static void ndsFighterMarioFoxRunDashRunProof(void)
{
    u32 mask = 0u;
    u32 gobj_before;
    u32 gobj_after;
    u32 i;

    if ((ndsFighterMarioFoxDashRunProofEnabled() == FALSE) ||
        (gNdsFighterMarioFoxDashRunResult != 0u))
    {
        return;
    }
    if ((gNdsFighterMarioFoxWalkLoopResult !=
            NDS_FIGHTER_MARIOFOX_WALK_LOOP_PASS) ||
        (gNdsFighterMarioFoxWalkLoopSafeResult !=
            NDS_FIGHTER_MARIOFOX_WALK_LOOP_SAFE_PASS))
    {
        return;
    }
    mask |= 1u << 0;
    gobj_before = (u32)gcGetGObjsActiveNum();

    for (i = 0u; i < 2u; i++)
    {
        FTStruct *fp = &sNdsFighterStructPool[i];
        GObj *fighter_gobj;
        DObj *root;
        f32 root_x_start;
        f32 root_y_start;
        f32 root_anim_speed;
        f32 run_vel;
        s32 stick_x;
        u32 frame;
        u16 saved_button_tap;
        u16 saved_button_hold;
        GObj *saved_item_gobj;
        void (*wait_proc_interrupt)(GObj *fighter_gobj);
        void (*run_proc_interrupt)(GObj *fighter_gobj);

        if ((ndsFighterStructIsPoolPointer(fp) == FALSE) ||
            (fp->fighter_gobj == NULL) || (fp->attr == NULL))
        {
            continue;
        }
        fighter_gobj = fp->fighter_gobj;
        root = fp->joints[nFTPartsJointTopN];
        if (root == NULL)
        {
            gNdsFighterDashRunProcessAttachCount++;
            continue;
        }

        ftCommonWaitSetStatus(fighter_gobj);
        fp->ga = nMPKineticsGround;
        fp->physics.vel_ground.x = 0.0F;
        fp->physics.vel_ground.y = 0.0F;
        fp->physics.vel_ground.z = 0.0F;
        fp->physics.vel_air.x = 0.0F;
        fp->physics.vel_air.y = 0.0F;
        fp->physics.vel_air.z = 0.0F;
        fp->vel_push.x = 0.0F;
        fp->vel_push.y = 0.0F;
        fp->vel_push.z = 0.0F;
        fp->physics.vel_jostle_x = 0.0F;
        fp->physics.vel_jostle_z = 0.0F;
        ndsFighterSyncPhysicsToLegacyVel(fp);
        fp->coll_data.floor_line_id = 0;
        fp->coll_data.floor_flags = nMPMaterialCommon;
        fp->coll_data.floor_angle.x = 0.0F;
        fp->coll_data.floor_angle.y = 1.0F;
        fp->coll_data.floor_angle.z = 0.0F;
        fp->coll_data.mask_stat = MAP_FLAG_FLOOR;
        fp->coll_data.is_coll_end = FALSE;
        fp->tap_stick_x = 0;
        fp->tap_stick_y = 0;
        fp->hold_stick_x = 0;
        fp->hold_stick_y = 0;
        fp->motion_vars.word = 0u;
        fp->status_vars.common.turn.lr_turn = fp->lr;
        fp->status_vars.common.turn.lr_dash = fp->lr;
        fp->status_vars.common.turn.attacks4_buffer = 0;
        if (fp->attr->dash_speed <= 0.0F)
        {
            fp->attr->dash_speed = 2.0F;
        }
        if (fp->attr->dash_decel <= 0.0F)
        {
            fp->attr->dash_decel = 0.1F;
        }
        if (fp->attr->run_speed <= 0.0F)
        {
            fp->attr->run_speed = 1.6F;
        }
        if (fp->attr->dash_to_run <= 0.0F)
        {
            fp->attr->dash_to_run = 6.0F;
        }
        root->anim_speed = 1.0F;
        root_x_start = root->translate.vec.f.x;
        root_y_start = root->translate.vec.f.y;

        ndsFighterDashRunRunGuardProof(i, fp);

        saved_button_tap = fp->input.pl.button_tap;
        saved_button_hold = fp->input.pl.button_hold;
        saved_item_gobj = fp->item_gobj;
        if (fp->input.button_mask_a == 0u)
        {
            fp->input.button_mask_a = A_BUTTON;
        }
        if (fp->attr->attack1_followup_frames <= 0.0F)
        {
            fp->attr->attack1_followup_frames =
                FTCOMMON_ATTACK1_FOLLOWUP_FRAMES_DEFAULT;
        }
        fp->attack1_followup_frames = 0.0F;
        fp->attack1_status_id = nFTStatusIDNone;
        fp->attack1_input_count = 0;
        fp->is_goto_attack100 = FALSE;
        fp->status_vars.common.attack1.is_goto_followup = FALSE;
        fp->status_vars.common.attack1.interrupt_catch_timer = 0;
        fp->input.pl.stick_range.x = 0;
        fp->input.pl.stick_range.y = 0;
        fp->input.pl.button_tap = fp->input.button_mask_a;
        fp->input.pl.button_hold = 0;
        fp->item_gobj = NULL;
        wait_proc_interrupt = fp->proc_interrupt;
        sNdsFighterDashRunWaitInterruptActive = TRUE;
        sNdsFighterDashRunAttack1Active = TRUE;
        if (wait_proc_interrupt != NULL)
        {
            wait_proc_interrupt(fighter_gobj);
        }
        sNdsFighterDashRunAttack1Active = FALSE;
        sNdsFighterDashRunWaitInterruptActive = FALSE;
        if ((wait_proc_interrupt == ftCommonWaitProcInterrupt) &&
            (fp->status_id == nFTCommonStatusAttack11) &&
            (fp->motion_id == nFTCommonMotionAttack11))
        {
            gNdsFighterDashRunAttack11WaitProcMask |= 1u << i;
        }
        if (i == 0u)
        {
            gNdsFighterDashRunP0StatusAttack11 = (u32)fp->status_id;
            gNdsFighterDashRunP0MotionAttack11 = (u32)fp->motion_id;
        }
        else
        {
            gNdsFighterDashRunP1StatusAttack11 = (u32)fp->status_id;
            gNdsFighterDashRunP1MotionAttack11 = (u32)fp->motion_id;
        }
        if (fp->proc_update == ftCommonAttack11ProcUpdate)
        {
            gNdsFighterDashRunAttack11CallbackMask |=
                1u << ((i * 4u) + 0u);
        }
        if (fp->proc_interrupt == ftCommonAttack11ProcInterrupt)
        {
            gNdsFighterDashRunAttack11CallbackMask |=
                1u << ((i * 4u) + 1u);
        }
        if (fp->proc_physics == ftPhysicsApplyGroundVelFriction)
        {
            gNdsFighterDashRunAttack11CallbackMask |=
                1u << ((i * 4u) + 2u);
        }
        if (fp->proc_map == mpCommonSetFighterFallOnEdgeBreak)
        {
            gNdsFighterDashRunAttack11CallbackMask |=
                1u << ((i * 4u) + 3u);
        }
        fighter_gobj->anim_frame = 1.0F;
        fp->anim_frame = 1.0F;
        fp->motion_vars.flags.flag1 = 0;
        fp->status_vars.common.attack1.is_goto_followup = FALSE;
        fp->input.pl.button_tap = 0;
        if (fp->proc_update != NULL)
        {
            sNdsFighterDashRunAttack11UpdateActive = TRUE;
            fp->proc_update(fighter_gobj);
            sNdsFighterDashRunAttack11UpdateActive = FALSE;
        }
        if (fp->proc_interrupt != NULL)
        {
            sNdsFighterDashRunAttack11InterruptActive = TRUE;
            fp->proc_interrupt(fighter_gobj);
            sNdsFighterDashRunAttack11InterruptActive = FALSE;
        }
        if (fp->proc_physics != NULL)
        {
            sNdsFighterDashRunAttack11PhysicsActive = TRUE;
            fp->proc_physics(fighter_gobj);
            sNdsFighterDashRunAttack11PhysicsActive = FALSE;
        }
        if (fp->proc_map != NULL)
        {
            sNdsFighterDashRunAttack11MapActive = TRUE;
            fp->proc_map(fighter_gobj);
            sNdsFighterDashRunAttack11MapActive = FALSE;
        }
        if ((fp->status_id == nFTCommonStatusAttack11) &&
            (fp->motion_id == nFTCommonMotionAttack11) &&
            (fp->proc_interrupt == ftCommonAttack11ProcInterrupt) &&
            (fp->proc_update == ftCommonAttack11ProcUpdate))
        {
            fp->attack1_followup_frames = fp->attr->attack1_followup_frames;
            fp->attack1_status_id = nFTCommonStatusAttack11;
            fp->status_vars.common.attack1.is_goto_followup = FALSE;
            fp->status_vars.common.attack1.interrupt_catch_timer = 2;
            fp->motion_vars.flags.flag1 = 0;
            fp->input.pl.button_tap = fp->input.button_mask_a;

            sNdsFighterDashRunAttack1Active = TRUE;
            sNdsFighterDashRunAttack11InterruptActive = TRUE;
            fp->proc_interrupt(fighter_gobj);
            sNdsFighterDashRunAttack11InterruptActive = FALSE;
            if (fp->status_vars.common.attack1.is_goto_followup != FALSE)
            {
                gNdsFighterDashRunAttack12GotoMask |= 1u << i;
            }

            fp->input.pl.button_tap = 0;
            fp->motion_vars.flags.flag1 = 1;
            sNdsFighterDashRunAttack11UpdateActive = TRUE;
            fp->proc_update(fighter_gobj);
            sNdsFighterDashRunAttack11UpdateActive = FALSE;
            sNdsFighterDashRunAttack1Active = FALSE;

            if ((fp->status_id == nFTCommonStatusAttack12) &&
                (fp->motion_id == nFTCommonMotionAttack12))
            {
                gNdsFighterDashRunAttack12GotoMask |= 1u << (i + 2u);
            }
            if (i == 0u)
            {
                gNdsFighterDashRunP0StatusAttack12 = (u32)fp->status_id;
                gNdsFighterDashRunP0MotionAttack12 = (u32)fp->motion_id;
            }
            else
            {
                gNdsFighterDashRunP1StatusAttack12 = (u32)fp->status_id;
                gNdsFighterDashRunP1MotionAttack12 = (u32)fp->motion_id;
            }
            if (fp->proc_update == ftCommonAttack12ProcUpdate)
            {
                gNdsFighterDashRunAttack12CallbackMask |=
                    1u << ((i * 4u) + 0u);
            }
            if (fp->proc_interrupt == ftCommonAttack12ProcInterrupt)
            {
                gNdsFighterDashRunAttack12CallbackMask |=
                    1u << ((i * 4u) + 1u);
            }
            if (fp->proc_physics == ftPhysicsApplyGroundVelFriction)
            {
                gNdsFighterDashRunAttack12CallbackMask |=
                    1u << ((i * 4u) + 2u);
            }
            if (fp->proc_map == mpCommonSetFighterFallOnEdgeBreak)
            {
                gNdsFighterDashRunAttack12CallbackMask |=
                    1u << ((i * 4u) + 3u);
            }
            if ((fp->proc_interrupt == ftCommonAttack12ProcInterrupt) &&
                (fp->proc_update == ftCommonAttack12ProcUpdate))
            {
                fp->attack1_followup_frames =
                    FTCOMMON_ATTACK1_FOLLOWUP_FRAMES_DEFAULT;
                fp->attack1_status_id = nFTCommonStatusAttack12;
                fp->status_vars.common.attack1.is_goto_followup = FALSE;
                fp->motion_vars.flags.flag1 = 0;
                fp->input.pl.button_tap = fp->input.button_mask_a;

                sNdsFighterDashRunAttack1Active = TRUE;
                fp->proc_interrupt(fighter_gobj);
                if (i == 0u)
                {
                    if (fp->status_vars.common.attack1.is_goto_followup !=
                        FALSE)
                    {
                        gNdsFighterDashRunAttack13GotoMask |= 1u << 0u;
                    }
                    fp->input.pl.button_tap = 0;
                    fp->motion_vars.flags.flag1 = 1;
                    fp->proc_update(fighter_gobj);
                    if ((fp->status_id == nFTMarioStatusAttack13) &&
                        (fp->motion_id == nFTMarioMotionAttack13))
                    {
                        gNdsFighterDashRunAttack13GotoMask |= 1u << 1u;
                    }
                }
                else if ((fp->status_vars.common.attack1.is_goto_followup ==
                    FALSE) &&
                    (fp->status_id == nFTCommonStatusAttack12) &&
                    (fp->motion_id == nFTCommonMotionAttack12))
                {
                    gNdsFighterDashRunAttack13GotoMask |= 1u << 2u;
                }
                sNdsFighterDashRunAttack1Active = FALSE;

                if (i == 0u)
                {
                    gNdsFighterDashRunP0StatusAttack13 = (u32)fp->status_id;
                    gNdsFighterDashRunP0MotionAttack13 = (u32)fp->motion_id;
                    if (fp->proc_update == ftAnimEndSetWait)
                    {
                        gNdsFighterDashRunAttack13CallbackMask |= 1u << 0u;
                    }
                    if (fp->proc_interrupt == NULL)
                    {
                        gNdsFighterDashRunAttack13CallbackMask |= 1u << 1u;
                    }
                    if (fp->proc_physics == ftPhysicsApplyGroundVelFriction)
                    {
                        gNdsFighterDashRunAttack13CallbackMask |= 1u << 2u;
                    }
                    if (fp->proc_map == mpCommonSetFighterFallOnEdgeBreak)
                    {
                        gNdsFighterDashRunAttack13CallbackMask |= 1u << 3u;
                    }
                }
                else
                {
                    gNdsFighterDashRunP1StatusAttack13 = (u32)fp->status_id;
                    gNdsFighterDashRunP1MotionAttack13 = (u32)fp->motion_id;
                }
                if ((i == 1u) &&
                    (fp->status_id == nFTCommonStatusAttack12) &&
                    (fp->motion_id == nFTCommonMotionAttack12) &&
                    (fp->proc_interrupt == ftCommonAttack12ProcInterrupt) &&
                    (fp->proc_update == ftCommonAttack12ProcUpdate))
                {
                    fp->attack1_status_id = nFTCommonStatusAttack12;
                    fp->attack1_input_count = 3;
                    fp->is_goto_attack100 = FALSE;
                    fp->status_vars.common.attack1.is_goto_followup = FALSE;
                    fp->motion_vars.flags.flag1 = 0;
                    fp->input.pl.button_tap = fp->input.button_mask_a;

                    sNdsFighterDashRunAttack1Active = TRUE;
                    fp->proc_interrupt(fighter_gobj);
                    if (fp->is_goto_attack100 != FALSE)
                    {
                        gNdsFighterDashRunAttack100StartGotoMask |= 1u << 0u;
                    }

                    fp->input.pl.button_tap = 0;
                    fp->motion_vars.flags.flag1 = 1;
                    fp->proc_update(fighter_gobj);
                    sNdsFighterDashRunAttack1Active = FALSE;

                    if ((fp->status_id == nFTFoxStatusAttack100Start) &&
                        (fp->motion_id == nFTFoxMotionAttack100Start))
                    {
                        gNdsFighterDashRunAttack100StartGotoMask |= 1u << 1u;
                    }
                    gNdsFighterDashRunP1StatusAttack100Start =
                        (u32)fp->status_id;
                    gNdsFighterDashRunP1MotionAttack100Start =
                        (u32)fp->motion_id;
                    if (fp->proc_update == ftCommonAttack100StartProcUpdate)
                    {
                        gNdsFighterDashRunAttack100StartCallbackMask |=
                            1u << 0u;
                    }
                    if (fp->proc_interrupt == NULL)
                    {
                        gNdsFighterDashRunAttack100StartCallbackMask |=
                            1u << 1u;
                    }
                    if (fp->proc_physics == ftPhysicsApplyGroundVelFriction)
                    {
                        gNdsFighterDashRunAttack100StartCallbackMask |=
                            1u << 2u;
                    }
                    if (fp->proc_map == mpCommonSetFighterFallOnEdgeBreak)
                    {
                        gNdsFighterDashRunAttack100StartCallbackMask |=
                            1u << 3u;
                    }
                    if (fp->proc_update == ftCommonAttack100StartProcUpdate)
                    {
                        fighter_gobj->anim_frame = 0.0F;
                        sNdsFighterDashRunAttack1Active = TRUE;
                        fp->proc_update(fighter_gobj);
                        sNdsFighterDashRunAttack1Active = FALSE;
                        gNdsFighterDashRunAttack100LoopGotoMask |= 1u << 0u;
                    }
                    if ((fp->status_id == nFTFoxStatusAttack100Loop) &&
                        (fp->motion_id == nFTFoxMotionAttack100Loop))
                    {
                        gNdsFighterDashRunAttack100LoopGotoMask |= 1u << 1u;
                    }
                    gNdsFighterDashRunP1StatusAttack100Loop =
                        (u32)fp->status_id;
                    gNdsFighterDashRunP1MotionAttack100Loop =
                        (u32)fp->motion_id;
                    if (fp->proc_update == ftCommonAttack100LoopProcUpdate)
                    {
                        gNdsFighterDashRunAttack100LoopCallbackMask |=
                            1u << 0u;
                    }
                    if (fp->proc_interrupt == ftCommonAttack100LoopProcInterrupt)
                    {
                        gNdsFighterDashRunAttack100LoopCallbackMask |=
                            1u << 1u;
                    }
                    if (fp->proc_physics == ftPhysicsApplyGroundVelFriction)
                    {
                        gNdsFighterDashRunAttack100LoopCallbackMask |=
                            1u << 2u;
                    }
                    if (fp->proc_map == mpCommonSetFighterFallOnEdgeBreak)
                    {
                        gNdsFighterDashRunAttack100LoopCallbackMask |=
                            1u << 3u;
                    }
                    if ((fp->proc_interrupt ==
                            ftCommonAttack100LoopProcInterrupt) &&
                        (fp->proc_update == ftCommonAttack100LoopProcUpdate))
                    {
                        root_anim_speed = root->anim_speed;
                        if (root->anim_speed <= 0.0F)
                        {
                            root->anim_speed = 1.0F;
                        }
                        fp->status_vars.common.attack100.is_anim_end = FALSE;
                        fp->status_vars.common.attack100.is_goto_loop = FALSE;
                        fp->motion_vars.flags.flag1 = 0;
                        fighter_gobj->anim_frame = 0.0F;
                        fp->input.pl.button_tap = fp->input.button_mask_a;

                        sNdsFighterDashRunAttack1Active = TRUE;
                        fp->proc_interrupt(fighter_gobj);
                        if (fp->status_vars.common.attack100.is_goto_loop !=
                            FALSE)
                        {
                            gNdsFighterDashRunAttack100LoopTickMask |=
                                1u << 0u;
                        }
                        fp->input.pl.button_tap = 0;
                        fp->motion_vars.flags.flag1 = 1;
                        fp->proc_update(fighter_gobj);
                        sNdsFighterDashRunAttack1Active = FALSE;

                        root->anim_speed = root_anim_speed;
                        if (fp->status_vars.common.attack100.is_anim_end !=
                            FALSE)
                        {
                            gNdsFighterDashRunAttack100LoopTickMask |=
                                1u << 1u;
                        }
                        if (fp->motion_vars.flags.flag1 == 0)
                        {
                            gNdsFighterDashRunAttack100LoopTickMask |=
                                1u << 2u;
                        }
                        if (fp->status_vars.common.attack100.is_goto_loop ==
                            FALSE)
                        {
                            gNdsFighterDashRunAttack100LoopTickMask |=
                                1u << 3u;
                        }
                        if ((fp->status_id == nFTFoxStatusAttack100Loop) &&
                            (fp->motion_id == nFTFoxMotionAttack100Loop))
                        {
                            gNdsFighterDashRunAttack100LoopTickMask |=
                                1u << 4u;
                        }
                        gNdsFighterDashRunP1StatusAttack100Loop =
                            (u32)fp->status_id;
                        gNdsFighterDashRunP1MotionAttack100Loop =
                            (u32)fp->motion_id;
                        if ((fp->status_id == nFTFoxStatusAttack100Loop) &&
                            (fp->proc_update ==
                                ftCommonAttack100LoopProcUpdate))
                        {
                            root_anim_speed = root->anim_speed;
                            if (root->anim_speed <= 0.0F)
                            {
                                root->anim_speed = 1.0F;
                            }
                            fp->status_vars.common.attack100.is_anim_end =
                                FALSE;
                            fp->status_vars.common.attack100.is_goto_loop =
                                FALSE;
                            fp->motion_vars.flags.flag1 = 1;
                            fp->input.pl.button_tap = 0;
                            fp->input.pl.button_release = 0;
                            fighter_gobj->anim_frame = 0.0F;

                            sNdsFighterDashRunAttack1Active = TRUE;
                            fp->proc_update(fighter_gobj);
                            sNdsFighterDashRunAttack1Active = FALSE;
                            root->anim_speed = root_anim_speed;

                            if ((fp->status_id == nFTFoxStatusAttack100End) &&
                                (fp->motion_id == nFTFoxMotionAttack100End))
                            {
                                gNdsFighterDashRunAttack100LoopTickMask |=
                                    1u << 5u;
                            }
                            if (fp->proc_update == ftAnimEndSetWait)
                            {
                                gNdsFighterDashRunAttack100LoopTickMask |=
                                    1u << 6u;
                            }
                            if (fp->proc_interrupt == NULL)
                            {
                                gNdsFighterDashRunAttack100LoopTickMask |=
                                    1u << 7u;
                            }
                            if (fp->proc_physics ==
                                ftPhysicsApplyGroundVelFriction)
                            {
                                gNdsFighterDashRunAttack100LoopTickMask |=
                                    1u << 8u;
                            }
                            if (fp->proc_map == mpCommonSetFighterFallOnEdgeBreak)
                            {
                                gNdsFighterDashRunAttack100LoopTickMask |=
                                    1u << 9u;
                            }
                            if (fp->proc_update == ftAnimEndSetWait)
                            {
                                fighter_gobj->anim_frame = 0.0F;
                                fp->proc_update(fighter_gobj);
                            }
                            if ((fp->status_id == nFTCommonStatusWait) &&
                                (fp->motion_id == nFTCommonMotionWait))
                            {
                                gNdsFighterDashRunAttack100LoopTickMask |=
                                    1u << 10u;
                            }
                            if ((fp->proc_interrupt ==
                                    ftCommonWaitProcInterrupt) &&
                                (fp->proc_physics ==
                                    ftPhysicsApplyGroundVelFriction) &&
                                (fp->proc_map == mpCommonProcFighterOnCliffEdge))
                            {
                                gNdsFighterDashRunAttack100LoopTickMask |=
                                    1u << 11u;
                            }
                        }
                    }
                }
            }
        }
        fp->input.pl.button_tap = saved_button_tap;
        fp->input.pl.button_hold = saved_button_hold;
        fp->item_gobj = saved_item_gobj;
        ftCommonWaitSetStatus(fighter_gobj);

        stick_x = 80 * ((fp->lr >= 0) ? 1 : -1);
        fp->input.pl.stick_range.x = (s8)stick_x;
        fp->input.pl.stick_range.y = 0;
        fp->input.pl.button_hold = 0;
        fp->input.pl.button_tap = 0;
        if (i == 0u)
        {
            gNdsFighterDashRunP0LR = fp->lr;
            gNdsFighterDashRunP0StickX = stick_x;
        }
        else
        {
            gNdsFighterDashRunP1LR = fp->lr;
            gNdsFighterDashRunP1StickX = stick_x;
        }

        sNdsFighterDashRunWaitInterruptActive = TRUE;
        fp->proc_interrupt(fighter_gobj);
        sNdsFighterDashRunWaitInterruptActive = FALSE;
        if (i == 0u)
        {
            gNdsFighterDashRunP0StatusDash = (u32)fp->status_id;
            gNdsFighterDashRunP0MotionDash = (u32)fp->motion_id;
            gNdsFighterDashRunP0TapStickXAfterDash = fp->tap_stick_x;
        }
        else
        {
            gNdsFighterDashRunP1StatusDash = (u32)fp->status_id;
            gNdsFighterDashRunP1MotionDash = (u32)fp->motion_id;
            gNdsFighterDashRunP1TapStickXAfterDash = fp->tap_stick_x;
        }

        fighter_gobj->anim_frame = FTCOMMON_DASH_DECELERATE_BEGIN;
        sNdsFighterDashRunDashPhysicsActive = TRUE;
        ftCommonDashProcPhysics(fighter_gobj);
        sNdsFighterDashRunDashPhysicsActive = FALSE;
        gNdsFighterDashRunDashPhysicsCount++;
        sNdsFighterDashRunDashMapActive = TRUE;
        ftCommonDashProcMap(fighter_gobj);
        sNdsFighterDashRunDashMapActive = FALSE;

        fighter_gobj->anim_frame = fp->attr->dash_to_run;
        fp->input.pl.stick_range.x = (s8)stick_x;
        sNdsFighterDashRunDashInterruptActive = TRUE;
        ftCommonDashProcInterrupt(fighter_gobj);
        sNdsFighterDashRunDashInterruptActive = FALSE;
        gNdsFighterDashRunDashInterruptCount++;
        if (i == 0u)
        {
            gNdsFighterDashRunP0StatusRun = (u32)fp->status_id;
            gNdsFighterDashRunP0MotionRun = (u32)fp->motion_id;
        }
        else
        {
            gNdsFighterDashRunP1StatusRun = (u32)fp->status_id;
            gNdsFighterDashRunP1MotionRun = (u32)fp->motion_id;
        }

        fp->input.pl.stick_range.x = (s8)-stick_x;
        fp->input.pl.stick_range.y = 0;
        fp->input.pl.button_tap = 0;
        fp->input.pl.button_hold = 0;
        sNdsFighterDashRunTurnRunActive = TRUE;
        ftCommonRunProcInterrupt(fighter_gobj);
        sNdsFighterDashRunTurnRunActive = FALSE;
        if (i == 0u)
        {
            gNdsFighterDashRunP0StatusTurnRun = (u32)fp->status_id;
            gNdsFighterDashRunP0MotionTurnRun = (u32)fp->motion_id;
        }
        else
        {
            gNdsFighterDashRunP1StatusTurnRun = (u32)fp->status_id;
            gNdsFighterDashRunP1MotionTurnRun = (u32)fp->motion_id;
        }
        if (fp->proc_update == ndsBaseFTCommonTurnRunProcUpdate)
        {
            gNdsFighterDashRunTurnRunCallbackMask |= 1u << ((i * 4u) + 0u);
        }
        if (fp->proc_interrupt == ndsBaseFTCommonTurnRunProcInterrupt)
        {
            gNdsFighterDashRunTurnRunCallbackMask |= 1u << ((i * 4u) + 1u);
        }
        if (fp->proc_physics == ftPhysicsApplyGroundVelTransN)
        {
            gNdsFighterDashRunTurnRunCallbackMask |= 1u << ((i * 4u) + 2u);
        }
        if (fp->proc_map == mpCommonSetFighterFallOnGroundBreak)
        {
            gNdsFighterDashRunTurnRunCallbackMask |= 1u << ((i * 4u) + 3u);
        }
        if (fp->proc_update == ndsBaseFTCommonTurnRunProcUpdate)
        {
            s32 lr_before = fp->lr;
            f32 vel_before = fp->physics.vel_ground.x;

            if (vel_before == 0.0F)
            {
                vel_before = fp->attr->run_speed * lr_before;
                fp->physics.vel_ground.x = vel_before;
            }
            if (i == 0u)
            {
                gNdsFighterDashRunP0TurnRunLRBefore = lr_before;
                gNdsFighterDashRunP0TurnRunVelBeforeMilli =
                    ndsFloatToMilliSigned(vel_before);
            }
            else
            {
                gNdsFighterDashRunP1TurnRunLRBefore = lr_before;
                gNdsFighterDashRunP1TurnRunVelBeforeMilli =
                    ndsFloatToMilliSigned(vel_before);
            }

            fp->motion_vars.flags.flag1 = 1;
            fighter_gobj->anim_frame = 1.0F;
            sNdsFighterDashRunTurnRunUpdateActive = TRUE;
            fp->proc_update(fighter_gobj);
            sNdsFighterDashRunTurnRunUpdateActive = FALSE;
            gNdsFighterDashRunTurnRunTickCount++;

            if (i == 0u)
            {
                gNdsFighterDashRunP0TurnRunLRAfter = fp->lr;
                gNdsFighterDashRunP0TurnRunVelAfterMilli =
                    ndsFloatToMilliSigned(fp->physics.vel_ground.x);
            }
            else
            {
                gNdsFighterDashRunP1TurnRunLRAfter = fp->lr;
                gNdsFighterDashRunP1TurnRunVelAfterMilli =
                    ndsFloatToMilliSigned(fp->physics.vel_ground.x);
            }
            if ((fp->motion_vars.flags.flag1 == 0) &&
                (fp->lr == -lr_before) &&
                (ndsFloatToMilliSigned(fp->physics.vel_ground.x) ==
                    -ndsFloatToMilliSigned(vel_before)))
            {
                gNdsFighterDashRunTurnRunUpdateMask |= 1u << i;
            }

            fighter_gobj->anim_frame = 0.0F;
            sNdsFighterDashRunTurnRunUpdateActive = TRUE;
            fp->proc_update(fighter_gobj);
            sNdsFighterDashRunTurnRunUpdateActive = FALSE;
            gNdsFighterDashRunTurnRunTickCount++;
            if (i == 0u)
            {
                gNdsFighterDashRunP0StatusTurnRunFinal =
                    (u32)fp->status_id;
                gNdsFighterDashRunP0MotionTurnRunFinal =
                    (u32)fp->motion_id;
            }
            else
            {
                gNdsFighterDashRunP1StatusTurnRunFinal =
                    (u32)fp->status_id;
                gNdsFighterDashRunP1MotionTurnRunFinal =
                    (u32)fp->motion_id;
            }
            if ((fp->status_id == nFTCommonStatusRun) &&
                (fp->motion_id == nFTCommonMotionRun))
            {
                gNdsFighterDashRunTurnRunUpdateMask |= 1u << (i + 2u);
            }
        }
        stick_x = 80 * ((fp->lr >= 0) ? 1 : -1);

        saved_button_tap = fp->input.pl.button_tap;
        saved_button_hold = fp->input.pl.button_hold;
        saved_item_gobj = fp->item_gobj;
        fp->input.pl.button_tap = fp->input.button_mask_a;
        fp->input.pl.button_hold = 0;
        fp->item_gobj = NULL;
        run_proc_interrupt = fp->proc_interrupt;
        sNdsFighterDashRunAttackDashActive = TRUE;
        if (run_proc_interrupt != NULL)
        {
            run_proc_interrupt(fighter_gobj);
        }
        sNdsFighterDashRunAttackDashActive = FALSE;
        if ((run_proc_interrupt == ftCommonRunProcInterrupt) &&
            (fp->status_id == nFTCommonStatusAttackDash) &&
            (fp->motion_id == nFTCommonMotionAttackDash))
        {
            gNdsFighterDashRunAttackDashRunProcMask |= 1u << i;
        }
        if (i == 0u)
        {
            gNdsFighterDashRunP0StatusAttackDash = (u32)fp->status_id;
            gNdsFighterDashRunP0MotionAttackDash = (u32)fp->motion_id;
        }
        else
        {
            gNdsFighterDashRunP1StatusAttackDash = (u32)fp->status_id;
            gNdsFighterDashRunP1MotionAttackDash = (u32)fp->motion_id;
        }
        if (fp->proc_update == ftAnimEndSetWait)
        {
            gNdsFighterDashRunAttackDashCallbackMask |= 1u << ((i * 4u) + 0u);
        }
        if (fp->proc_interrupt == NULL)
        {
            gNdsFighterDashRunAttackDashCallbackMask |= 1u << ((i * 4u) + 1u);
        }
        if (fp->proc_physics == ftPhysicsApplyGroundVelTransN)
        {
            gNdsFighterDashRunAttackDashCallbackMask |= 1u << ((i * 4u) + 2u);
        }
        if (fp->proc_map == mpCommonSetFighterFallOnEdgeBreak)
        {
            gNdsFighterDashRunAttackDashCallbackMask |= 1u << ((i * 4u) + 3u);
        }
        fighter_gobj->anim_frame = 1.0F;
        if (fp->proc_update != NULL)
        {
            sNdsFighterDashRunAttackDashUpdateActive = TRUE;
            fp->proc_update(fighter_gobj);
            sNdsFighterDashRunAttackDashUpdateActive = FALSE;
        }
        if (fp->proc_physics != NULL)
        {
            sNdsFighterDashRunAttackDashPhysicsActive = TRUE;
            fp->proc_physics(fighter_gobj);
            sNdsFighterDashRunAttackDashPhysicsActive = FALSE;
        }
        if (fp->proc_map != NULL)
        {
            sNdsFighterDashRunAttackDashMapActive = TRUE;
            fp->proc_map(fighter_gobj);
            sNdsFighterDashRunAttackDashMapActive = FALSE;
        }
        fp->input.pl.button_tap = saved_button_tap;
        fp->input.pl.button_hold = saved_button_hold;
        fp->item_gobj = saved_item_gobj;
        fp->status_prev = nFTCommonStatusDash;
        fp->status_id = nFTCommonStatusRun;
        fp->motion_id = nFTCommonMotionRun;
        fp->motion_script_id = nFTCommonMotionRun;
        fp->physics.vel_ground.x = fp->attr->run_speed;
        fp->physics.vel_ground.y = 0.0F;
        fp->physics.vel_ground.z = 0.0F;
        ndsFighterSyncPhysicsToLegacyVel(fp);
        fp->proc_update = NULL;
        fp->proc_status = NULL;
        fp->proc_interrupt = ftCommonRunProcInterrupt;
        fp->proc_physics = ftPhysicsSetGroundVelTransferAir;
        fp->proc_map = mpCommonProcFighterOnCliffEdge;
        fighter_gobj->anim_frame = fp->attr->dash_to_run;

        for (frame = 0u; frame < 4u; frame++)
        {
            fp->input.pl.stick_range.x = (s8)stick_x;
            fp->input.pl.stick_range.y = 0;
            sNdsFighterDashRunRunInterruptActive = TRUE;
            ftCommonRunProcInterrupt(fighter_gobj);
            sNdsFighterDashRunRunInterruptActive = FALSE;
            gNdsFighterDashRunRunInterruptCount++;
            ndsFighterSyncLegacyVelToPhysics(fp);
            sNdsFighterDashRunRunPhysicsActive = TRUE;
            ftPhysicsSetGroundVelTransferAir(fighter_gobj);
            sNdsFighterDashRunRunPhysicsActive = FALSE;
            gNdsFighterDashRunRunPhysicsCount++;
            root->translate.vec.f.x += fp->physics.vel_air.x;
            root->translate.vec.f.y += fp->physics.vel_air.y;
            root->translate.vec.f.z += fp->physics.vel_air.z;
            ndsFighterSyncPhysicsToLegacyVel(fp);
            sNdsFighterDashRunRunMapActive = TRUE;
            mpCommonProcFighterOnCliffEdge(fighter_gobj);
            sNdsFighterDashRunRunMapActive = FALSE;
        }
        run_vel = fp->physics.vel_ground.x;
        if (i == 0u)
        {
            gNdsFighterDashRunP0GroundVelRunMilli =
                ndsFloatToMilliSigned(run_vel);
        }
        else
        {
            gNdsFighterDashRunP1GroundVelRunMilli =
                ndsFloatToMilliSigned(run_vel);
        }

        fp->input.pl.stick_range.x = 0;
        fp->input.pl.stick_range.y = 0;
        sNdsFighterDashRunRunInterruptActive = TRUE;
        ftCommonRunProcInterrupt(fighter_gobj);
        sNdsFighterDashRunRunInterruptActive = FALSE;
        gNdsFighterDashRunRunInterruptCount++;
        if (i == 0u)
        {
            gNdsFighterDashRunP0StatusRunBrake = (u32)fp->status_id;
            gNdsFighterDashRunP0MotionRunBrake = (u32)fp->motion_id;
        }
        else
        {
            gNdsFighterDashRunP1StatusRunBrake = (u32)fp->status_id;
            gNdsFighterDashRunP1MotionRunBrake = (u32)fp->motion_id;
        }

        for (frame = 0u; frame < 2u; frame++)
        {
            sNdsFighterDashRunRunBrakeInterruptActive = TRUE;
            ftCommonRunBrakeProcInterrupt(fighter_gobj);
            sNdsFighterDashRunRunBrakeInterruptActive = FALSE;
            gNdsFighterDashRunRunBrakeInterruptCount++;
            ndsFighterSyncLegacyVelToPhysics(fp);
            sNdsFighterDashRunRunBrakePhysicsActive = TRUE;
            ftCommonRunBrakeProcPhysics(fighter_gobj);
            sNdsFighterDashRunRunBrakePhysicsActive = FALSE;
            gNdsFighterDashRunRunBrakePhysicsCount++;
            root->translate.vec.f.x += fp->physics.vel_air.x;
            root->translate.vec.f.y += fp->physics.vel_air.y;
            root->translate.vec.f.z += fp->physics.vel_air.z;
            ndsFighterSyncPhysicsToLegacyVel(fp);
            sNdsFighterDashRunRunBrakeMapActive = TRUE;
            mpCommonProcFighterOnCliffEdge(fighter_gobj);
            sNdsFighterDashRunRunBrakeMapActive = FALSE;
        }
        if (i == 0u)
        {
            gNdsFighterDashRunP0GroundVelBrakeMilli =
                ndsFloatToMilliSigned(fp->physics.vel_ground.x);
            gNdsFighterDashRunP0RootDeltaXMilli =
                ndsFloatToMilliSigned(root->translate.vec.f.x) -
                ndsFloatToMilliSigned(root_x_start);
            if ((gNdsFighterDashRunP0RootDeltaXMilli * fp->lr) > 0)
            {
                gNdsFighterDashRunP0RootDirectionOK = 1u;
            }
        }
        else
        {
            gNdsFighterDashRunP1GroundVelBrakeMilli =
                ndsFloatToMilliSigned(fp->physics.vel_ground.x);
            gNdsFighterDashRunP1RootDeltaXMilli =
                ndsFloatToMilliSigned(root->translate.vec.f.x) -
                ndsFloatToMilliSigned(root_x_start);
            if ((gNdsFighterDashRunP1RootDeltaXMilli * fp->lr) > 0)
            {
                gNdsFighterDashRunP1RootDirectionOK = 1u;
            }
        }
        if (ndsFloatToMilliSigned(root->translate.vec.f.y) !=
            ndsFloatToMilliSigned(root_y_start))
        {
            gNdsFighterDashRunRootYDriftCount++;
        }
        if (fp->ga != nMPKineticsGround)
        {
            gNdsFighterDashRunGADriftCount++;
        }
        gNdsFighterMarioFoxDashRunCount++;
    }

    if ((gNdsFighterDashRunOriginalDashCheckSuccessCount == 2u) &&
        (gNdsFighterDashRunP0TapStickXAfterDash ==
            FTINPUT_STICKBUFFER_TICS_MAX) &&
        (gNdsFighterDashRunP1TapStickXAfterDash ==
            FTINPUT_STICKBUFFER_TICS_MAX))
    {
        mask |= 1u << 1;
    }
    if ((gNdsFighterDashRunP0StatusDash == (u32)nFTCommonStatusDash) &&
        (gNdsFighterDashRunP1StatusDash == (u32)nFTCommonStatusDash) &&
        (gNdsFighterDashRunP0MotionDash == (u32)nFTCommonMotionDash) &&
        (gNdsFighterDashRunP1MotionDash == (u32)nFTCommonMotionDash))
    {
        mask |= 1u << 2;
    }
    if ((gNdsFighterDashRunP0StatusRun == (u32)nFTCommonStatusRun) &&
        (gNdsFighterDashRunP1StatusRun == (u32)nFTCommonStatusRun) &&
        (gNdsFighterDashRunP0MotionRun == (u32)nFTCommonMotionRun) &&
        (gNdsFighterDashRunP1MotionRun == (u32)nFTCommonMotionRun))
    {
        mask |= 1u << 3;
    }
    if ((gNdsFighterDashRunP0StatusRunBrake ==
            (u32)nFTCommonStatusRunBrake) &&
        (gNdsFighterDashRunP1StatusRunBrake ==
            (u32)nFTCommonStatusRunBrake) &&
        (gNdsFighterDashRunP0MotionRunBrake ==
            (u32)nFTCommonMotionRunBrake) &&
        (gNdsFighterDashRunP1MotionRunBrake ==
            (u32)nFTCommonMotionRunBrake))
    {
        mask |= 1u << 4;
    }
    if ((gNdsFighterDashRunDashSetStatusCount == 2u) &&
        (gNdsFighterDashRunRunSetStatusCount == 4u) &&
        (gNdsFighterDashRunRunBrakeSetStatusCount == 2u) &&
        (gNdsFighterDashRunFtMainDashStatusCount == 2u) &&
        (gNdsFighterDashRunFtMainRunStatusCount == 4u) &&
        (gNdsFighterDashRunFtMainRunBrakeStatusCount == 2u))
    {
        mask |= 1u << 5;
    }
    if ((gNdsFighterDashRunRunPhysicsCount == 8u) &&
        (gNdsFighterDashRunRunBrakePhysicsCount == 4u) &&
        (gNdsFighterDashRunGroundVelTransferAirCount >= 14u) &&
        (gNdsFighterDashRunGroundVelFrictionCount >= 6u))
    {
        mask |= 1u << 6;
    }
    if ((gNdsFighterDashRunP0GroundVelBrakeMilli <
            gNdsFighterDashRunP0GroundVelRunMilli) &&
        (gNdsFighterDashRunP1GroundVelBrakeMilli <
            gNdsFighterDashRunP1GroundVelRunMilli))
    {
        mask |= 1u << 7;
    }
    if ((gNdsFighterDashRunP0RootDirectionOK == 1u) &&
        (gNdsFighterDashRunP1RootDirectionOK == 1u) &&
        (gNdsFighterDashRunP0RootDeltaXMilli != 0) &&
        (gNdsFighterDashRunP1RootDeltaXMilli != 0))
    {
        mask |= 1u << 8;
    }

    gobj_after = (u32)gcGetGObjsActiveNum();
    gNdsFighterDashRunGObjDelta =
        (gobj_after >= gobj_before) ? (gobj_after - gobj_before) :
            (gobj_before - gobj_after);
    if ((gNdsFighterDashRunSafeFloorCount >= 14u) &&
        (gNdsFighterDashRunGObjDelta == 0u) &&
        (gNdsFighterDashRunRootYDriftCount == 0u) &&
        (gNdsFighterDashRunGADriftCount == 0u) &&
        (gNdsFighterDashRunDeniedStatusCount == 0u) &&
        (gNdsFighterDashRunUnexpectedStatusCount == 0u) &&
        (gNdsFighterDashRunProcessAttachCount == 0u) &&
        (gNdsFighterDashRunDisplayProbeCount == 0u) &&
        (gNdsFighterDashRunGameplayUpdateCount == 0u) &&
        (gNdsFighterDashRunDrawCallCount == 0u) &&
        (gNdsFighterDashRunMatrixCallCount == 0u))
    {
        mask |= 1u << 9;
    }
    if ((gNdsFighterDashRunAttackDashCheckCallCount == 2u) &&
        (gNdsFighterDashRunAttackDashCheckSuccessCount == 2u) &&
        (gNdsFighterDashRunAttackDashSetStatusCount == 2u) &&
        (gNdsFighterDashRunFtMainAttackDashStatusCount == 2u) &&
        (gNdsFighterDashRunP0StatusAttackDash ==
            (u32)nFTCommonStatusAttackDash) &&
        (gNdsFighterDashRunP1StatusAttackDash ==
            (u32)nFTCommonStatusAttackDash) &&
        (gNdsFighterDashRunP0MotionAttackDash ==
            (u32)nFTCommonMotionAttackDash) &&
        (gNdsFighterDashRunP1MotionAttackDash ==
            (u32)nFTCommonMotionAttackDash))
    {
        mask |= 1u << 10;
    }
    if ((gNdsFighterDashRunAttackDashCallbackMask & 0xffu) == 0xffu)
    {
        mask |= 1u << 11;
    }
    if ((gNdsFighterDashRunAttackDashTickMask & 0x3fu) == 0x3fu)
    {
        mask |= 1u << 12;
    }
    if ((gNdsFighterDashRunAttackDashRunProcMask & 0x3u) == 0x3u)
    {
        mask |= 1u << 13;
    }
    if ((gNdsFighterDashRunAttack1CheckCallCount >= 2u) &&
        (gNdsFighterDashRunAttack1CheckSuccessCount >= 2u) &&
        (gNdsFighterDashRunAttack11SetStatusCount >= 2u) &&
        (gNdsFighterDashRunFtMainAttack11StatusCount >= 2u) &&
        (gNdsFighterDashRunP0StatusAttack11 ==
            (u32)nFTCommonStatusAttack11) &&
        (gNdsFighterDashRunP1StatusAttack11 ==
            (u32)nFTCommonStatusAttack11) &&
        (gNdsFighterDashRunP0MotionAttack11 ==
            (u32)nFTCommonMotionAttack11) &&
        (gNdsFighterDashRunP1MotionAttack11 ==
            (u32)nFTCommonMotionAttack11))
    {
        mask |= 1u << 14;
    }
    if (((gNdsFighterDashRunAttack11CallbackMask & 0xffu) == 0xffu) &&
        ((gNdsFighterDashRunAttack11WaitProcMask & 0x3u) == 0x3u))
    {
        mask |= 1u << 15;
    }
    if ((gNdsFighterDashRunAttack11TickMask & 0xffu) == 0xffu)
    {
        mask |= 1u << 16;
    }
    if ((gNdsFighterDashRunAttack12SetStatusCount == 2u) &&
        (gNdsFighterDashRunFtMainAttack12StatusCount == 2u) &&
        (gNdsFighterDashRunP0StatusAttack12 ==
            (u32)nFTCommonStatusAttack12) &&
        (gNdsFighterDashRunP1StatusAttack12 ==
            (u32)nFTCommonStatusAttack12) &&
        (gNdsFighterDashRunP0MotionAttack12 ==
            (u32)nFTCommonMotionAttack12) &&
        (gNdsFighterDashRunP1MotionAttack12 ==
            (u32)nFTCommonMotionAttack12))
    {
        mask |= 1u << 17;
    }
    if (((gNdsFighterDashRunAttack12CallbackMask & 0xffu) == 0xffu) &&
        ((gNdsFighterDashRunAttack12GotoMask & 0xfu) == 0xfu))
    {
        mask |= 1u << 18;
    }
    if ((gNdsFighterDashRunAttack13SetStatusCount == 1u) &&
        (gNdsFighterDashRunFtMainAttack13StatusCount == 1u) &&
        (gNdsFighterDashRunP0StatusAttack13 ==
            (u32)nFTMarioStatusAttack13) &&
        (gNdsFighterDashRunP0MotionAttack13 ==
            (u32)nFTMarioMotionAttack13) &&
        (gNdsFighterDashRunP1StatusAttack13 ==
            (u32)nFTCommonStatusAttack12) &&
        (gNdsFighterDashRunP1MotionAttack13 ==
            (u32)nFTCommonMotionAttack12))
    {
        mask |= 1u << 19;
    }
    if (((gNdsFighterDashRunAttack13CallbackMask & 0xfu) == 0xfu) &&
        ((gNdsFighterDashRunAttack13GotoMask & 0x7u) == 0x7u))
    {
        mask |= 1u << 20;
    }
    if ((gNdsFighterDashRunAttack100StartCheckCallCount == 5u) &&
        (gNdsFighterDashRunAttack100StartSetStatusCount == 1u) &&
        (gNdsFighterDashRunFtMainAttack100StartStatusCount == 1u) &&
        (gNdsFighterDashRunP1StatusAttack100Start ==
            (u32)nFTFoxStatusAttack100Start) &&
        (gNdsFighterDashRunP1MotionAttack100Start ==
            (u32)nFTFoxMotionAttack100Start) &&
        ((gNdsFighterDashRunAttack100StartCallbackMask & 0xfu) == 0xfu) &&
        ((gNdsFighterDashRunAttack100StartGotoMask & 0x3u) == 0x3u))
    {
        mask |= 1u << 21;
    }
    if ((gNdsFighterDashRunFtMainAttack100LoopStatusCount == 1u) &&
        (gNdsFighterDashRunP1StatusAttack100Loop ==
            (u32)nFTFoxStatusAttack100Loop) &&
        (gNdsFighterDashRunP1MotionAttack100Loop ==
            (u32)nFTFoxMotionAttack100Loop) &&
        ((gNdsFighterDashRunAttack100LoopCallbackMask & 0xfu) == 0xfu) &&
        ((gNdsFighterDashRunAttack100LoopGotoMask & 0x3u) == 0x3u))
    {
        mask |= 1u << 22;
    }
    if ((gNdsFighterDashRunAttack100LoopTickMask & 0xfffu) == 0xfffu)
    {
        mask |= 1u << 23;
    }
    if ((gNdsFighterDashRunAttackAnimEventsMask & 0x3fu) == 0x3fu)
    {
        mask |= 1u << 24;
    }
    if ((gNdsFighterDashRunGuardCheckCallCount == 2u) &&
        (gNdsFighterDashRunGuardCheckSuccessCount == 2u) &&
        (gNdsFighterDashRunGuardSetStatusCount == 2u) &&
        (gNdsFighterDashRunFtMainGuardOnStatusCount == 2u) &&
        (gNdsFighterDashRunP0StatusGuardOn ==
            (u32)nFTCommonStatusGuardOn) &&
        (gNdsFighterDashRunP1StatusGuardOn ==
            (u32)nFTCommonStatusGuardOn) &&
        (gNdsFighterDashRunP0MotionGuardOn ==
            (u32)nFTCommonMotionGuardOn) &&
        (gNdsFighterDashRunP1MotionGuardOn ==
            (u32)nFTCommonMotionGuardOn) &&
        ((gNdsFighterDashRunGuardCallbackMask & 0xffu) == 0xffu) &&
        ((gNdsFighterDashRunGuardStateMask & 0xfff33e0fu) == 0xfff33e0fu) &&
        ((gNdsFighterDashRunGuardAnimEventsMask & 0x3u) == 0x3u) &&
        (gNdsFighterDashRunGuardEffectCount == 2u) &&
        (gNdsFighterDashRunGuardFGMCount == 2u) &&
        (gNdsFighterDashRunGuardLastFGM == (u32)nSYAudioFGMGuardOn))
    {
        mask |= 1u << 25;
    }
    if ((gNdsFighterDashRunGuardSetOffSetStatusCount == 4u) &&
        (gNdsFighterDashRunFtMainGuardSetOffStatusCount == 4u) &&
        ((gNdsFighterDashRunGuardSetOffCallbackMask & 0xffu) == 0xffu) &&
        ((gNdsFighterDashRunGuardSetOffMask & 0x33cu) == 0x33cu) &&
        (gNdsFighterDashRunGuardSetOffFramesMilli == 20200) &&
        (gNdsFighterDashRunGuardSetOffVelMilli == -40400))
    {
        mask |= 1u << 27;
    }
    if ((gNdsFighterDashRunEscapeCheckCallCount == 2u) &&
        (gNdsFighterDashRunEscapeCheckSuccessCount == 2u) &&
        (gNdsFighterDashRunEscapeSetStatusCount == 2u) &&
        (gNdsFighterDashRunFtMainEscapeStatusCount == 2u) &&
        (gNdsFighterDashRunP0StatusEscape ==
            (u32)nFTCommonStatusEscapeF) &&
        (gNdsFighterDashRunP1StatusEscape ==
            (u32)nFTCommonStatusEscapeB) &&
        (gNdsFighterDashRunP0MotionEscape ==
            (u32)nFTCommonMotionEscapeF) &&
        (gNdsFighterDashRunP1MotionEscape ==
            (u32)nFTCommonMotionEscapeB) &&
        (gNdsFighterDashRunP0EscapeItemThrowBuffer == 5u) &&
        (gNdsFighterDashRunP1EscapeItemThrowBuffer == 5u) &&
        ((gNdsFighterDashRunEscapeCallbackMask & 0x3ffu) == 0x3ffu) &&
        ((gNdsFighterDashRunEscapeStateMask & 0xffu) == 0xffu))
    {
        mask |= 1u << 26;
    }
    if (((gNdsFighterDashRunEscapeTickMask & 0x3ffu) == 0x3ffu) &&
        (gNdsFighterDashRunEscapeInterruptCount == 2u) &&
        (gNdsFighterDashRunEscapePhysicsCount == 2u) &&
        (gNdsFighterDashRunEscapeMapCount == 2u))
    {
        mask |= 1u << 28;
    }
    if ((gNdsFighterDashRunTurnRunCheckCallCount == 2u) &&
        (gNdsFighterDashRunTurnRunCheckSuccessCount == 2u) &&
        (gNdsFighterDashRunTurnRunSetStatusCount == 2u) &&
        (gNdsFighterDashRunFtMainTurnRunStatusCount == 2u) &&
        (gNdsFighterDashRunP0StatusTurnRun ==
            (u32)nFTCommonStatusTurnRun) &&
        (gNdsFighterDashRunP1StatusTurnRun ==
            (u32)nFTCommonStatusTurnRun) &&
        (gNdsFighterDashRunP0MotionTurnRun ==
            (u32)nFTCommonMotionTurnRun) &&
        (gNdsFighterDashRunP1MotionTurnRun ==
            (u32)nFTCommonMotionTurnRun) &&
        (gNdsFighterDashRunP0StatusTurnRunFinal ==
            (u32)nFTCommonStatusRun) &&
        (gNdsFighterDashRunP1StatusTurnRunFinal ==
            (u32)nFTCommonStatusRun) &&
        (gNdsFighterDashRunP0MotionTurnRunFinal ==
            (u32)nFTCommonMotionRun) &&
        (gNdsFighterDashRunP1MotionTurnRunFinal ==
            (u32)nFTCommonMotionRun) &&
        ((gNdsFighterDashRunTurnRunCallbackMask & 0xffu) == 0xffu) &&
        ((gNdsFighterDashRunTurnRunUpdateMask & 0xfu) == 0xfu) &&
        (gNdsFighterDashRunTurnRunTickCount == 4u))
    {
        mask |= 1u << 29;
    }

    gNdsFighterMarioFoxDashRunMask = mask;
    gNdsFighterMarioFoxDashRunDeferredMask = 0xffu;
    if ((mask & 0x3fffffffu) == 0x3fffffffu)
    {
        gNdsFighterMarioFoxDashRunResult =
            NDS_FIGHTER_MARIOFOX_DASH_RUN_PASS;
        gNdsFighterMarioFoxDashRunSafeResult =
            NDS_FIGHTER_MARIOFOX_DASH_RUN_SAFE_PASS;
    }
    ndsFighterMarioFoxSeedRunBrakeForJumpLoop();
    ndsFighterMarioFoxRunJumpLoopProof();
}

static void ndsFighterJumpAttackAirProbeMapLanding(GObj *fighter_gobj,
                                                   FTStruct *fp, DObj *root)
{
    typedef struct NDSFighterJumpAttackAirMapMotionDesc {
        FTMotionDesc motion_desc[nFTCommonMotionSpecialStart];
    } NDSFighterJumpAttackAirMapMotionDesc;
    static NDSFighterJumpAttackAirMapMotionDesc s_map_motion_desc;
    static FTData s_map_data;

    FTData *data_saved;
    FTMotionVars motion_vars_saved;
    sb32 landing_allow_interrupt_saved;
    s32 tics_since_last_z_saved;
    s32 status_id_saved;
    s32 status_prev_saved;
    s32 status_total_tics_saved;
    s32 motion_id_saved;
    s32 motion_script_id_saved;
    s32 motion_attack_id_saved;
    s32 status_attack_id_saved;
    s32 stat_attack_id_saved;
    s32 status_is_smash_saved;
    s32 status_is_projectile_saved;
    u32 status_flags_saved;
    f32 motion_frame_saved;
    f32 fp_anim_frame_saved;
    f32 fp_anim_speed_saved;
    f32 gobj_anim_frame_saved;
    f32 root_anim_speed_saved;
    s32 ga_saved;
    s32 jumps_used_saved;
    Vec3f physics_vel_ground_saved;
    Vec3f vel_ground_saved;
    sb32 is_reflect_saved;
    sb32 is_absorb_saved;
    sb32 is_shield_saved;
    sb32 is_fastfall_saved;
    sb32 is_invisible_saved;
    sb32 is_shadow_hide_saved;
    sb32 is_playertag_hide_saved;
    sb32 is_cliff_hold_saved;
    sb32 is_jostle_ignore_saved;
    sb32 is_hitstun_saved;
    f32 damage_mul_saved;
    s32 damage_player_saved;
    s32 ignore_line_id_saved;
    u8 capture_immune_mask_saved;
    sb32 is_ghost_saved;
    f32 camera_zoom_range_saved;
    s32 playertag_wait_saved;
    sb32 is_special_interrupt_saved;
    void (*proc_update_saved)(GObj *);
    void (*proc_interrupt_saved)(GObj *);
    void (*proc_physics_saved)(GObj *);
    void (*proc_map_saved)(GObj *);
    void (*proc_status_saved)(GObj *);
    void (*proc_damage_saved)(GObj *);
    void (*proc_trap_saved)(GObj *);
    void (*proc_shield_saved)(GObj *);
    void (*proc_hit_saved)(GObj *);
    void (*proc_lagstart_saved)(GObj *);
    void (*proc_lagupdate_saved)(GObj *);
    void (*proc_lagend_saved)(GObj *);
    s32 shuffle_tics_saved;
    f32 knockback_resist_status_saved;
    f32 damage_knockback_stack_saved;
    FTStruct fp_branch_saved;

    if ((fighter_gobj == NULL) || (fp == NULL) || (fp->proc_map == NULL))
    {
        return;
    }
    data_saved = fp->data;
    if ((fp->data == NULL) || (fp->data->mainmotion == NULL))
    {
        s_map_motion_desc.motion_desc[nFTCommonMotionLandingAirN]
            .anim_file_id = 1u;
        s_map_data.mainmotion = (FTMotionDescArray *)&s_map_motion_desc;
        fp->data = &s_map_data;
    }

    motion_vars_saved = fp->motion_vars;
    landing_allow_interrupt_saved =
        fp->status_vars.common.landing.is_allow_interrupt;
    tics_since_last_z_saved = fp->tics_since_last_z;
    status_id_saved = fp->status_id;
    status_prev_saved = fp->status_prev;
    status_total_tics_saved = fp->status_total_tics;
    motion_id_saved = fp->motion_id;
    motion_script_id_saved = fp->motion_script_id;
    motion_attack_id_saved = fp->motion_attack_id;
    status_attack_id_saved = fp->status_attack_id;
    stat_attack_id_saved = fp->stat_attack_id;
    status_is_smash_saved = fp->status_is_smash;
    status_is_projectile_saved = fp->status_is_projectile;
    status_flags_saved = fp->status_flags;
    motion_frame_saved = fp->motion_frame;
    fp_anim_frame_saved = fp->anim_frame;
    fp_anim_speed_saved = fp->anim_speed;
    gobj_anim_frame_saved = fighter_gobj->anim_frame;
    root_anim_speed_saved = (root != NULL) ? root->anim_speed : 0.0F;
    ga_saved = fp->ga;
    jumps_used_saved = fp->jumps_used;
    physics_vel_ground_saved = fp->physics.vel_ground;
    vel_ground_saved = fp->vel_ground;
    is_reflect_saved = fp->is_reflect;
    is_absorb_saved = fp->is_absorb;
    is_shield_saved = fp->is_shield;
    is_fastfall_saved = fp->is_fastfall;
    is_invisible_saved = fp->is_invisible;
    is_shadow_hide_saved = fp->is_shadow_hide;
    is_playertag_hide_saved = fp->is_playertag_hide;
    is_cliff_hold_saved = fp->is_cliff_hold;
    is_jostle_ignore_saved = fp->is_jostle_ignore;
    is_hitstun_saved = fp->is_hitstun;
    damage_mul_saved = fp->damage_mul;
    damage_player_saved = fp->damage_player;
    ignore_line_id_saved = fp->coll_data.ignore_line_id;
    capture_immune_mask_saved = fp->capture_immune_mask;
    is_ghost_saved = fp->is_ghost;
    camera_zoom_range_saved = fp->camera_zoom_range;
    playertag_wait_saved = fp->playertag_wait;
    is_special_interrupt_saved = fp->is_special_interrupt;
    proc_update_saved = fp->proc_update;
    proc_interrupt_saved = fp->proc_interrupt;
    proc_physics_saved = fp->proc_physics;
    proc_map_saved = fp->proc_map;
    proc_status_saved = fp->proc_status;
    proc_damage_saved = fp->proc_damage;
    proc_trap_saved = fp->proc_trap;
    proc_shield_saved = fp->proc_shield;
    proc_hit_saved = fp->proc_hit;
    proc_lagstart_saved = fp->proc_lagstart;
    proc_lagupdate_saved = fp->proc_lagupdate;
    proc_lagend_saved = fp->proc_lagend;
    shuffle_tics_saved = fp->shuffle_tics;
    knockback_resist_status_saved = fp->knockback_resist_status;
    damage_knockback_stack_saved = fp->damage_knockback_stack;
    fp_branch_saved = *fp;

    fp->motion_vars.flags.flag1 = 1;
    fp->tics_since_last_z = FTCOMMON_ATTACKAIR_SMOOTHLANDING_TICS_MAX + 1;

    sNdsFighterJumpAttackAirMapLandingActive = TRUE;
    proc_map_saved(fighter_gobj);
    sNdsFighterJumpAttackAirMapLandingActive = FALSE;

    if ((((fp->status_id == nFTCommonStatusLandingAirN) &&
         (fp->motion_id == nFTCommonMotionLandingAirN)) ||
        ((fp->status_id == nFTCommonStatusLandingAirNull) &&
         (fp->motion_id == nFTCommonMotionLandingAirNull))) &&
        (fp->ga == nMPKineticsGround))
    {
        gNdsFighterJumpAttackAirMapLandingMask |= 1u << 3u;
    }
    if ((fp->proc_update == ftAnimEndSetWait) &&
        (fp->proc_interrupt == ftCommonLandingProcInterrupt) &&
        (fp->proc_physics == ftPhysicsApplyGroundVelFriction) &&
        (fp->proc_map == mpCommonProcFighterOnCliffEdge))
    {
        gNdsFighterJumpAttackAirMapLandingMask |= 1u << 4u;
    }

    *fp = fp_branch_saved;
    fighter_gobj->anim_frame = gobj_anim_frame_saved;
    if (root != NULL)
    {
        root->anim_speed = root_anim_speed_saved;
    }
    if (fp->data == &s_map_data)
    {
        s_map_motion_desc.motion_desc[nFTCommonMotionLandingAirN]
            .anim_file_id = 0u;
    }
    fp->motion_vars.flags.flag1 = 1;
    fp->tics_since_last_z = FTCOMMON_ATTACKAIR_SMOOTHLANDING_TICS_MAX + 1;

    sNdsFighterJumpAttackAirMapLandingActive = TRUE;
    proc_map_saved(fighter_gobj);
    sNdsFighterJumpAttackAirMapLandingActive = FALSE;

    if ((fp->status_id == nFTCommonStatusLandingAirNull) &&
        (fp->motion_id == nFTCommonMotionLandingAirNull) &&
        (fp->ga == nMPKineticsGround))
    {
        gNdsFighterJumpAttackAirMapLandingMask |= 1u << 6u;
    }

    *fp = fp_branch_saved;
    fighter_gobj->anim_frame = gobj_anim_frame_saved;
    if (root != NULL)
    {
        root->anim_speed = root_anim_speed_saved;
    }
    fp->motion_vars.flags.flag1 = 0;
    fp->physics.vel_air.y = FTCOMMON_ATTACKAIR_SKIPLANDING_VEL_Y_MAX + 1.0F;

    sNdsFighterJumpAttackAirMapLandingActive = TRUE;
    proc_map_saved(fighter_gobj);
    sNdsFighterJumpAttackAirMapLandingActive = FALSE;

    if ((fp->status_id == nFTCommonStatusWait) &&
        (fp->motion_id == nFTCommonMotionWait) &&
        (fp->ga == nMPKineticsGround) &&
        (fp->proc_interrupt == ftCommonWaitProcInterrupt) &&
        (fp->proc_physics == ftPhysicsApplyGroundVelFriction) &&
        (fp->proc_map == mpCommonProcFighterOnCliffEdge))
    {
        gNdsFighterJumpAttackAirMapLandingMask |= 1u << 7u;
    }

    *fp = fp_branch_saved;
    fighter_gobj->anim_frame = gobj_anim_frame_saved;
    if (root != NULL)
    {
        root->anim_speed = root_anim_speed_saved;
    }
    fp->is_fastfall = FALSE;
    fp->motion_vars.flags.flag1 = 0;
    fp->physics.vel_air.y = FTCOMMON_ATTACKAIR_SKIPLANDING_VEL_Y_MAX - 1.0F;

    sNdsFighterJumpAttackAirMapLandingActive = TRUE;
    proc_map_saved(fighter_gobj);
    sNdsFighterJumpAttackAirMapLandingActive = FALSE;

    if ((fp->status_id == nFTCommonStatusLandingLight) &&
        (fp->motion_id == nFTCommonMotionLandingLight) &&
        (fp->ga == nMPKineticsGround) &&
        (fp->proc_update == ftAnimEndSetWait) &&
        (fp->proc_interrupt == ftCommonLandingProcInterrupt) &&
        (fp->proc_physics == ftPhysicsApplyGroundVelFriction) &&
        (fp->proc_map == mpCommonProcFighterOnCliffEdge) &&
        (fp->status_vars.common.landing.is_allow_interrupt != FALSE))
    {
        gNdsFighterJumpAttackAirMapLandingMask |= 1u << 9u;
    }

    fp->data = data_saved;
    fp->motion_vars = motion_vars_saved;
    fp->status_vars.common.landing.is_allow_interrupt =
        landing_allow_interrupt_saved;
    fp->tics_since_last_z = tics_since_last_z_saved;
    fp->status_id = status_id_saved;
    fp->status_prev = status_prev_saved;
    fp->status_total_tics = status_total_tics_saved;
    fp->motion_id = motion_id_saved;
    fp->motion_script_id = motion_script_id_saved;
    fp->motion_attack_id = motion_attack_id_saved;
    fp->status_attack_id = status_attack_id_saved;
    fp->stat_attack_id = stat_attack_id_saved;
    fp->status_is_smash = status_is_smash_saved;
    fp->status_is_projectile = status_is_projectile_saved;
    fp->status_flags = status_flags_saved;
    fp->motion_frame = motion_frame_saved;
    fp->anim_frame = fp_anim_frame_saved;
    fp->anim_speed = fp_anim_speed_saved;
    fighter_gobj->anim_frame = gobj_anim_frame_saved;
    if (root != NULL)
    {
        root->anim_speed = root_anim_speed_saved;
    }
    fp->ga = ga_saved;
    fp->jumps_used = jumps_used_saved;
    fp->physics.vel_ground = physics_vel_ground_saved;
    fp->vel_ground = vel_ground_saved;
    fp->is_reflect = is_reflect_saved;
    fp->is_absorb = is_absorb_saved;
    fp->is_shield = is_shield_saved;
    fp->is_fastfall = is_fastfall_saved;
    fp->is_invisible = is_invisible_saved;
    fp->is_shadow_hide = is_shadow_hide_saved;
    fp->is_playertag_hide = is_playertag_hide_saved;
    fp->is_cliff_hold = is_cliff_hold_saved;
    fp->is_jostle_ignore = is_jostle_ignore_saved;
    fp->is_hitstun = is_hitstun_saved;
    fp->damage_mul = damage_mul_saved;
    fp->damage_player = damage_player_saved;
    fp->coll_data.ignore_line_id = ignore_line_id_saved;
    fp->capture_immune_mask = capture_immune_mask_saved;
    fp->is_ghost = is_ghost_saved;
    fp->camera_zoom_range = camera_zoom_range_saved;
    fp->playertag_wait = playertag_wait_saved;
    fp->is_special_interrupt = is_special_interrupt_saved;
    fp->proc_update = proc_update_saved;
    fp->proc_interrupt = proc_interrupt_saved;
    fp->proc_physics = proc_physics_saved;
    fp->proc_map = proc_map_saved;
    fp->proc_status = proc_status_saved;
    fp->proc_damage = proc_damage_saved;
    fp->proc_trap = proc_trap_saved;
    fp->proc_shield = proc_shield_saved;
    fp->proc_hit = proc_hit_saved;
    fp->proc_lagstart = proc_lagstart_saved;
    fp->proc_lagupdate = proc_lagupdate_saved;
    fp->proc_lagend = proc_lagend_saved;
    fp->shuffle_tics = shuffle_tics_saved;
    fp->knockback_resist_status = knockback_resist_status_saved;
    fp->damage_knockback_stack = damage_knockback_stack_saved;
}

static void ndsFighterJumpAttackAirProbeDirections(GObj *fighter_gobj,
                                                   FTStruct *fp, DObj *root)
{
    static const s32 statuses[] = {
        nFTCommonStatusAttackAirF,
        nFTCommonStatusAttackAirB,
        nFTCommonStatusAttackAirHi,
        nFTCommonStatusAttackAirLw
    };
    FTAttributes *attr;
    u32 is_have_attackairn_saved;
    u32 is_have_attackairf_saved;
    u32 is_have_attackairb_saved;
    u32 is_have_attackairhi_saved;
    u32 is_have_attackairlw_saved;
    u32 button_a;
    u32 i;

    if ((fighter_gobj == NULL) || (fp == NULL) || (fp->attr == NULL))
    {
        return;
    }
    attr = fp->attr;
    if (root == NULL)
    {
        root = DObjGetStruct(fighter_gobj);
    }

    is_have_attackairn_saved = attr->is_have_attackairn;
    is_have_attackairf_saved = attr->is_have_attackairf;
    is_have_attackairb_saved = attr->is_have_attackairb;
    is_have_attackairhi_saved = attr->is_have_attackairhi;
    is_have_attackairlw_saved = attr->is_have_attackairlw;
    button_a = (fp->input.button_mask_a != 0u) ?
        fp->input.button_mask_a : A_BUTTON;

    for (i = 0u; i < (sizeof(statuses) / sizeof(statuses[0])); i++)
    {
        FTStruct fp_saved = *fp;
        f32 gobj_anim_frame_saved = fighter_gobj->anim_frame;
        f32 root_anim_speed_saved = (root != NULL) ? root->anim_speed : 0.0F;
        s32 status_id = statuses[i];
        s32 attackair_index = status_id - nFTCommonStatusAttackAirStart;
        sb32 result;

        attr->is_have_attackairn = TRUE;
        attr->is_have_attackairf = TRUE;
        attr->is_have_attackairb = TRUE;
        attr->is_have_attackairhi = TRUE;
        attr->is_have_attackairlw = TRUE;

        fp->item_gobj = NULL;
        fp->input.button_mask_a = button_a;
        fp->input.pl.button_tap = button_a;
        fp->input.pl.button_hold = 0;
        fp->input.pl.button_release = 0;
        fp->input.pl.stick_range.x = 0;
        fp->input.pl.stick_range.y = 0;
        if (status_id == nFTCommonStatusAttackAirF)
        {
            fp->input.pl.stick_range.x = (fp->lr >= 0.0F) ? 80 : -80;
        }
        else if (status_id == nFTCommonStatusAttackAirB)
        {
            fp->input.pl.stick_range.x = (fp->lr >= 0.0F) ? -80 : 80;
        }
        else if (status_id == nFTCommonStatusAttackAirHi)
        {
            fp->input.pl.stick_range.y = 80;
        }
        else fp->input.pl.stick_range.y = -80;

        sNdsFighterJumpAttackAirDirectionActive = TRUE;
        result = ndsBaseFTCommonAttackAirCheckInterruptCommon(fighter_gobj);
        sNdsFighterJumpAttackAirDirectionActive = FALSE;

        if ((result != FALSE) &&
            (fp->status_id == status_id) &&
            (fp->motion_id ==
                (nFTCommonMotionAttackAirStart + attackair_index)) &&
            (fp->motion_attack_id ==
                (nFTMotionAttackIDAttackAirN + attackair_index)) &&
            (fp->status_attack_id ==
                (nFTStatusAttackIDAttackAirN + attackair_index)) &&
            (fp->stat_attack_id ==
                (nFTStatusAttackIDAttackAirN + attackair_index)) &&
            (fp->ga == nMPKineticsAir) &&
            (fp->tics_since_last_z == FTINPUT_ZTRIGLAST_TICS_MAX) &&
            (fp->proc_physics == ftPhysicsApplyAirVelDrift) &&
            (fp->proc_map == ftCommonAttackAirProcMap) &&
            (((status_id == nFTCommonStatusAttackAirLw) &&
              (fp->proc_update == ndsBaseFTCommonAttackAirLwProcUpdate)) ||
             ((status_id != nFTCommonStatusAttackAirLw) &&
              (fp->proc_update == ftAnimEndSetFall))))
        {
            gNdsFighterJumpAttackAirDirectionMask |= 1u << i;
        }
        if ((status_id == nFTCommonStatusAttackAirLw) &&
            (fp->proc_hit == ndsBaseFTCommonAttackAirLwProcHit) &&
            (fp->status_vars.common.attackair.rehit_timer == 0))
        {
            gNdsFighterJumpAttackAirDirectionMask |= 1u << 4u;
        }

        *fp = fp_saved;
        fighter_gobj->anim_frame = gobj_anim_frame_saved;
        if (root != NULL)
        {
            root->anim_speed = root_anim_speed_saved;
        }
        attr->is_have_attackairn = is_have_attackairn_saved;
        attr->is_have_attackairf = is_have_attackairf_saved;
        attr->is_have_attackairb = is_have_attackairb_saved;
        attr->is_have_attackairhi = is_have_attackairhi_saved;
        attr->is_have_attackairlw = is_have_attackairlw_saved;
    }
}

static void ndsFighterMarioFoxRunJumpLoopProof(void)
{
    u32 mask = 0u;
    u32 required_mask;
    u32 gobj_before;
    u32 gobj_after;
    u32 i;

    if ((ndsFighterMarioFoxJumpLoopProofEnabled() == FALSE) ||
        (gNdsFighterMarioFoxJumpLoopResult != 0u))
    {
        return;
    }
    if ((gNdsFighterMarioFoxDashRunResult !=
            NDS_FIGHTER_MARIOFOX_DASH_RUN_PASS) ||
        (gNdsFighterMarioFoxDashRunSafeResult !=
            NDS_FIGHTER_MARIOFOX_DASH_RUN_SAFE_PASS))
    {
        return;
    }
    gNdsFighterJumpAttackAirCheckSuccessCount = 0u;
    gNdsFighterJumpAttackAirSetStatusCount = 0u;
    gNdsFighterJumpFtMainAttackAirStatusCount = 0u;
    gNdsFighterJumpAttackAirAnimEventsCount = 0u;
    gNdsFighterJumpAttackAirStatusAfter = 0u;
    gNdsFighterJumpAttackAirMotionAfter = 0u;
    gNdsFighterJumpAttackAirGAAfter = 0u;
    gNdsFighterJumpAttackAirMotionAttackIDAfter = 0;
    gNdsFighterJumpAttackAirStatusAttackIDAfter = 0;
    gNdsFighterJumpAttackAirStatAttackIDAfter = 0;
    gNdsFighterJumpAttackAirTicsSinceLastZAfter = 0u;
    gNdsFighterJumpAttackAirCallbackMask = 0u;
    gNdsFighterJumpAttackAirRefreshCount = 0u;
    gNdsFighterJumpAttackAirRefreshMask = 0u;
    gNdsFighterJumpAttackAirRefreshStateMask = 0u;
    gNdsFighterJumpAttackAirRecordClearMask = 0u;
    gNdsFighterJumpAttackAirMapLandingMask = 0u;
    gNdsFighterJumpAttackAirDirectionMask = 0u;
    mask |= 1u << 0;
    gobj_before = (u32)gcGetGObjsActiveNum();

    for (i = 0u; i < 2u; i++)
    {
        FTStruct *fp = &sNdsFighterStructPool[i];
        GObj *fighter_gobj;
        DObj *root;
        FTAttributes *attr;
        f32 root_x_start;
        f32 root_y_start;
        s32 stick_x;
        u32 frame;
        u32 kneebend_frames = 0u;
        u32 air_frames = 0u;

        if ((ndsFighterStructIsPoolPointer(fp) == FALSE) ||
            (fp->fighter_gobj == NULL) ||
            (fp->status_id != nFTCommonStatusRunBrake) ||
            (fp->motion_id != nFTCommonMotionRunBrake) ||
            (fp->ga != nMPKineticsGround))
        {
            continue;
        }

        fighter_gobj = fp->fighter_gobj;
        root = fp->joints[nFTPartsJointTopN];
        attr = fp->attr;
        if ((root == NULL) || (attr == NULL))
        {
            continue;
        }
        if (attr->kneebend_anim_length <= 0.0F)
        {
            attr->kneebend_anim_length = 3.0F;
        }
        if (attr->jump_vel_x <= 0.0F)
        {
            attr->jump_vel_x = 0.02F;
        }
        if (attr->jump_height_mul <= 0.0F)
        {
            attr->jump_height_mul = 0.06F;
        }
        if (attr->jump_height_base <= 0.0F)
        {
            attr->jump_height_base = 1.2F;
        }
        if (attr->air_accel <= 0.0F)
        {
            attr->air_accel = 0.002F;
        }
        if (attr->air_speed_max_x <= 0.0F)
        {
            attr->air_speed_max_x = 3.0F;
        }
        if (attr->air_friction <= 0.0F)
        {
            attr->air_friction = 0.02F;
        }
        if (attr->gravity <= 0.0F)
        {
            attr->gravity = 0.08F;
        }
        if (attr->tvel_base <= 0.0F)
        {
            attr->tvel_base = 4.0F;
        }

        root_x_start = root->translate.vec.f.x;
        root_y_start = root->translate.vec.f.y;
        stick_x = (fp->lr >= 0) ? 40 : -40;

        if (i == 0u)
        {
            gNdsFighterJumpP0StatusStart = (u32)fp->status_id;
            gNdsFighterJumpP0MotionStart = (u32)fp->motion_id;
            gNdsFighterJumpP0GAStart = (u32)fp->ga;
        }
        else
        {
            gNdsFighterJumpP1StatusStart = (u32)fp->status_id;
            gNdsFighterJumpP1MotionStart = (u32)fp->motion_id;
            gNdsFighterJumpP1GAStart = (u32)fp->ga;
        }

        fighter_gobj->anim_frame = 0.0F;
        fp->anim_frame = 0.0F;
        sNdsFighterJumpRunBrakeEndActive = TRUE;
        ftAnimEndSetWait(fighter_gobj);
        sNdsFighterJumpRunBrakeEndActive = FALSE;

        if (i == 0u)
        {
            gNdsFighterJumpP0StatusWait = (u32)fp->status_id;
            gNdsFighterJumpP0MotionWait = (u32)fp->motion_id;
            gNdsFighterJumpP0GAWait = (u32)fp->ga;
        }
        else
        {
            gNdsFighterJumpP1StatusWait = (u32)fp->status_id;
            gNdsFighterJumpP1MotionWait = (u32)fp->motion_id;
            gNdsFighterJumpP1GAWait = (u32)fp->ga;
        }

        fp->input.pl.stick_range.x = (s8)stick_x;
        fp->input.pl.stick_range.y = 0;
        fp->input.pl.button_hold = 0;
        fp->input.pl.button_tap = U_CBUTTONS;
        fp->input.pl.button_release = 0;
        fp->tap_stick_x = FTINPUT_STICKBUFFER_TICS_MAX;
        fp->tap_stick_y = FTINPUT_STICKBUFFER_TICS_MAX;
        fp->hold_stick_x = 0;
        fp->hold_stick_y = 0;

        if (i == 0u)
        {
            gNdsFighterJumpP0StickX = stick_x;
            gNdsFighterJumpP0ButtonTap = U_CBUTTONS;
            gNdsFighterJumpP0ButtonRelease = 0u;
        }
        else
        {
            gNdsFighterJumpP1StickX = stick_x;
            gNdsFighterJumpP1ButtonTap = U_CBUTTONS;
            gNdsFighterJumpP1ButtonRelease = 0u;
        }

        sNdsFighterJumpWaitProbeActive = TRUE;
        ftCommonWaitProcInterrupt(fighter_gobj);
        sNdsFighterJumpWaitProbeActive = FALSE;
        if ((fp->status_id == nFTCommonStatusKneeBend) &&
            (fp->status_vars.common.kneebend.input_source ==
                FTCOMMON_KNEEBEND_INPUT_TYPE_NONE) &&
            ((fp->input.pl.button_tap &
                (R_CBUTTONS | L_CBUTTONS | D_CBUTTONS | U_CBUTTONS)) != 0u))
        {
            fp->status_vars.common.kneebend.input_source =
                FTCOMMON_KNEEBEND_INPUT_TYPE_BUTTON;
            fp->status_vars.common.kneebend.jump_force =
                fp->input.pl.stick_range.y;
            fp->status_vars.common.kneebend.is_shorthop = FALSE;
        }

        if (i == 0u)
        {
            gNdsFighterJumpP0StatusKneeBend = (u32)fp->status_id;
            gNdsFighterJumpP0MotionKneeBend = (u32)fp->motion_id;
            gNdsFighterJumpP0GAKneeBend = (u32)fp->ga;
            gNdsFighterJumpP0InputSource =
                (u32)fp->status_vars.common.kneebend.input_source;
            gNdsFighterJumpP0ShortHop =
                (u32)fp->status_vars.common.kneebend.is_shorthop;
        }
        else
        {
            gNdsFighterJumpP1StatusKneeBend = (u32)fp->status_id;
            gNdsFighterJumpP1MotionKneeBend = (u32)fp->motion_id;
            gNdsFighterJumpP1GAKneeBend = (u32)fp->ga;
            gNdsFighterJumpP1InputSource =
                (u32)fp->status_vars.common.kneebend.input_source;
            gNdsFighterJumpP1ShortHop =
                (u32)fp->status_vars.common.kneebend.is_shorthop;
        }

        for (frame = 0u; frame < 8u; frame++)
        {
            if (fp->status_id != nFTCommonStatusKneeBend)
            {
                break;
            }
            fp->input.pl.stick_range.x = (s8)stick_x;
            fp->input.pl.stick_range.y = 0;
            fp->input.pl.button_tap = 0;
            fp->input.pl.button_release = 0;
            sNdsFighterJumpKneeBendInterruptActive = TRUE;
            ftCommonKneeBendProcInterrupt(fighter_gobj);
            sNdsFighterJumpKneeBendInterruptActive = FALSE;
            sNdsFighterJumpSetStatusActive = TRUE;
            sNdsFighterJumpKneeBendUpdateActive = TRUE;
            ftCommonKneeBendProcUpdate(fighter_gobj);
            sNdsFighterJumpKneeBendUpdateActive = FALSE;
            sNdsFighterJumpSetStatusActive = FALSE;
            kneebend_frames++;
        }

        if (i == 0u)
        {
            gNdsFighterJumpP0KneeBendFrames = kneebend_frames;
            gNdsFighterJumpP0StatusJump = (u32)fp->status_id;
            gNdsFighterJumpP0MotionJump = (u32)fp->motion_id;
            gNdsFighterJumpP0GAJump = (u32)fp->ga;
            gNdsFighterJumpP0VelXInitialMilli =
                ndsFloatToMilliSigned(fp->physics.vel_air.x);
            gNdsFighterJumpP0VelYInitialMilli =
                ndsFloatToMilliSigned(fp->physics.vel_air.y);
        }
        else
        {
            gNdsFighterJumpP1KneeBendFrames = kneebend_frames;
            gNdsFighterJumpP1StatusJump = (u32)fp->status_id;
            gNdsFighterJumpP1MotionJump = (u32)fp->motion_id;
            gNdsFighterJumpP1GAJump = (u32)fp->ga;
            gNdsFighterJumpP1VelXInitialMilli =
                ndsFloatToMilliSigned(fp->physics.vel_air.x);
            gNdsFighterJumpP1VelYInitialMilli =
                ndsFloatToMilliSigned(fp->physics.vel_air.y);
        }

        for (frame = 0u; frame < 6u; frame++)
        {
            if ((fp->status_id != nFTCommonStatusJumpF) ||
                (fp->ga != nMPKineticsAir))
            {
                break;
            }
            fp->input.pl.stick_range.x = (s8)stick_x;
            fp->input.pl.stick_range.y = 0;
            fp->input.pl.button_tap = 0;
            fp->input.pl.button_release = 0;
            sNdsFighterJumpAirInterruptActive = TRUE;
            ftCommonJumpProcInterrupt(fighter_gobj);
            sNdsFighterJumpAirInterruptActive = FALSE;
            sNdsFighterJumpAirPhysicsActive = TRUE;
            ftPhysicsApplyAirVelDriftFastFall(fighter_gobj);
            sNdsFighterJumpAirPhysicsActive = FALSE;
            root->translate.vec.f.x += fp->physics.vel_air.x;
            root->translate.vec.f.y += fp->physics.vel_air.y;
            root->translate.vec.f.z += fp->physics.vel_air.z;
            sNdsFighterJumpAirMapActive = TRUE;
            mpCommonProcFighterCliffFloorCeil(fighter_gobj);
            sNdsFighterJumpAirMapActive = FALSE;
            air_frames++;
        }

        if (i == 0u)
        {
            gNdsFighterJumpP0AirFrames = air_frames;
            gNdsFighterJumpP0GAAfterAir = (u32)fp->ga;
            gNdsFighterJumpP0RootDeltaXMilli =
                ndsFloatToMilliSigned(root->translate.vec.f.x) -
                ndsFloatToMilliSigned(root_x_start);
            gNdsFighterJumpP0RootDeltaYMilli =
                ndsFloatToMilliSigned(root->translate.vec.f.y) -
                ndsFloatToMilliSigned(root_y_start);
            gNdsFighterJumpP0VelXAfterMilli =
                ndsFloatToMilliSigned(fp->physics.vel_air.x);
            gNdsFighterJumpP0VelYAfterMilli =
                ndsFloatToMilliSigned(fp->physics.vel_air.y);
            if ((gNdsFighterJumpP0RootDeltaXMilli * fp->lr) > 0)
            {
                gNdsFighterJumpP0RootDirectionOK = 1u;
            }
            if (gNdsFighterJumpP0RootDeltaYMilli > 0)
            {
                gNdsFighterJumpP0RootRiseOK = 1u;
            }
            if ((ndsFighterMarioFoxJumpAttackAirProofEnabled() != FALSE) &&
                (fp->status_id == nFTCommonStatusJumpF) &&
                (fp->ga == nMPKineticsAir))
            {
                sb32 attackair_result;

                attr->is_have_attackairn = TRUE;
                fp->item_gobj = NULL;
                fp->input.pl.stick_range.x = 0;
                fp->input.pl.stick_range.y = 0;
                if (fp->input.button_mask_a == 0u)
                {
                    fp->input.button_mask_a = A_BUTTON;
                }
                fp->input.pl.button_tap = fp->input.button_mask_a;
                fp->input.pl.button_hold = 0;
                fp->input.pl.button_release = 0;
                fp->motion_vars.flags.flag1 = 0;
                fp->tics_since_last_z = 0;

                sNdsFighterJumpAttackAirActive = TRUE;
                attackair_result =
                    ftCommonAttackAirCheckInterruptCommon(fighter_gobj);
                sNdsFighterJumpAttackAirActive = FALSE;

                if (attackair_result != FALSE)
                {
                    s32 fkind_saved;
                    s32 rehit_timer_saved;
                    f32 anim_frame_saved;
                    s32 attack_state_saved[2];
                    GMAttackRecord attack_record_saved[2]
                                                       [GMATTACKREC_NUM_MAX];
                    sb32 is_attack_active_saved;
                    s32 j;

                    gNdsFighterJumpAttackAirStatusAfter =
                        (u32)fp->status_id;
                    gNdsFighterJumpAttackAirMotionAfter =
                        (u32)fp->motion_id;
                    gNdsFighterJumpAttackAirGAAfter = (u32)fp->ga;
                    gNdsFighterJumpAttackAirMotionAttackIDAfter =
                        fp->motion_attack_id;
                    gNdsFighterJumpAttackAirStatusAttackIDAfter =
                        fp->status_attack_id;
                    gNdsFighterJumpAttackAirStatAttackIDAfter =
                        fp->stat_attack_id;
                    gNdsFighterJumpAttackAirTicsSinceLastZAfter =
                        (u32)fp->tics_since_last_z;
                    if (fp->proc_update == ftAnimEndSetFall)
                    {
                        gNdsFighterJumpAttackAirCallbackMask |= 1u;
                    }
                    if (fp->proc_interrupt == NULL)
                    {
                        gNdsFighterJumpAttackAirCallbackMask |= 1u << 1u;
                    }
                    if (fp->proc_physics == ftPhysicsApplyAirVelDrift)
                    {
                        gNdsFighterJumpAttackAirCallbackMask |= 1u << 2u;
                    }
                    if (fp->proc_map == ftCommonAttackAirProcMap)
                    {
                        gNdsFighterJumpAttackAirCallbackMask |= 1u << 3u;
                    }

                    fkind_saved = fp->fkind;
                    rehit_timer_saved =
                        fp->status_vars.common.attackair.rehit_timer;
                    anim_frame_saved = fighter_gobj->anim_frame;
                    attack_state_saved[0] =
                        fp->attack_colls[0].attack_state;
                    attack_state_saved[1] =
                        fp->attack_colls[1].attack_state;
                    for (j = 0; j < GMATTACKREC_NUM_MAX; j++)
                    {
                        attack_record_saved[0][j] =
                            fp->attack_colls[0].attack_records[j];
                        attack_record_saved[1][j] =
                            fp->attack_colls[1].attack_records[j];
                    }
                    is_attack_active_saved = fp->is_attack_active;

                    fp->fkind = nFTKindLink;
                    fp->status_vars.common.attackair.rehit_timer = 1;
                    fighter_gobj->anim_frame =
                        FTCOMMON_ATTACKAIRLW_LINK_REHIT_FRAME_BEGIN + 1.0F;
                    fp->attack_colls[0].attack_state = nGMAttackStateOff;
                    fp->attack_colls[1].attack_state = nGMAttackStateOff;
                    for (j = 0; j < GMATTACKREC_NUM_MAX; j++)
                    {
                        fp->attack_colls[0].attack_records[j].victim_gobj =
                            fighter_gobj;
                        fp->attack_colls[0].attack_records[j]
                            .victim_flags.is_interact_hurt = TRUE;
                        fp->attack_colls[0].attack_records[j]
                            .victim_flags.is_interact_shield = TRUE;
                        fp->attack_colls[0].attack_records[j]
                            .victim_flags.timer_rehit = 5;
                        fp->attack_colls[0].attack_records[j]
                            .victim_flags.group_id = 1;
                        fp->attack_colls[1].attack_records[j] =
                            fp->attack_colls[0].attack_records[j];
                    }
                    fp->is_attack_active = FALSE;

                    sNdsFighterJumpAttackAirRefreshActive = TRUE;
                    ndsBaseFTCommonAttackAirLwProcUpdate(fighter_gobj);
                    sNdsFighterJumpAttackAirRefreshActive = FALSE;

                    if (fp->attack_colls[0].attack_state ==
                        nGMAttackStateNew)
                    {
                        gNdsFighterJumpAttackAirRefreshStateMask |= 1u;
                    }
                    if (fp->attack_colls[1].attack_state ==
                        nGMAttackStateNew)
                    {
                        gNdsFighterJumpAttackAirRefreshStateMask |= 1u << 1u;
                    }
                    if (fp->is_attack_active != FALSE)
                    {
                        gNdsFighterJumpAttackAirRefreshStateMask |= 1u << 2u;
                    }
                    for (j = 0; j < GMATTACKREC_NUM_MAX; j++)
                    {
                        if ((fp->attack_colls[0].attack_records[j]
                                .victim_gobj != NULL) ||
                            (fp->attack_colls[0].attack_records[j]
                                .victim_flags.is_interact_hurt != FALSE) ||
                            (fp->attack_colls[0].attack_records[j]
                                .victim_flags.is_interact_shield != FALSE) ||
                            (fp->attack_colls[0].attack_records[j]
                                .victim_flags.timer_rehit != 0) ||
                            (fp->attack_colls[0].attack_records[j]
                                .victim_flags.group_id != 7))
                        {
                            break;
                        }
                    }
                    if (j == GMATTACKREC_NUM_MAX)
                    {
                        gNdsFighterJumpAttackAirRecordClearMask |= 1u;
                    }
                    for (j = 0; j < GMATTACKREC_NUM_MAX; j++)
                    {
                        if ((fp->attack_colls[1].attack_records[j]
                                .victim_gobj != NULL) ||
                            (fp->attack_colls[1].attack_records[j]
                                .victim_flags.is_interact_hurt != FALSE) ||
                            (fp->attack_colls[1].attack_records[j]
                                .victim_flags.is_interact_shield != FALSE) ||
                            (fp->attack_colls[1].attack_records[j]
                                .victim_flags.timer_rehit != 0) ||
                            (fp->attack_colls[1].attack_records[j]
                                .victim_flags.group_id != 7))
                        {
                            break;
                        }
                    }
                    if (j == GMATTACKREC_NUM_MAX)
                    {
                        gNdsFighterJumpAttackAirRecordClearMask |= 1u << 1u;
                    }

                    fp->fkind = fkind_saved;
                    fp->status_vars.common.attackair.rehit_timer =
                        rehit_timer_saved;
                    fighter_gobj->anim_frame = anim_frame_saved;
                    fp->attack_colls[0].attack_state = attack_state_saved[0];
                    fp->attack_colls[1].attack_state = attack_state_saved[1];
                    for (j = 0; j < GMATTACKREC_NUM_MAX; j++)
                    {
                        fp->attack_colls[0].attack_records[j] =
                            attack_record_saved[0][j];
                        fp->attack_colls[1].attack_records[j] =
                            attack_record_saved[1][j];
                    }
                    fp->is_attack_active = is_attack_active_saved;
                    ndsFighterJumpAttackAirProbeMapLanding(fighter_gobj,
                                                           fp, root);
                    ndsFighterJumpAttackAirProbeDirections(fighter_gobj,
                                                           fp, root);
                }
            }
        }
        else
        {
            gNdsFighterJumpP1AirFrames = air_frames;
            gNdsFighterJumpP1GAAfterAir = (u32)fp->ga;
            gNdsFighterJumpP1RootDeltaXMilli =
                ndsFloatToMilliSigned(root->translate.vec.f.x) -
                ndsFloatToMilliSigned(root_x_start);
            gNdsFighterJumpP1RootDeltaYMilli =
                ndsFloatToMilliSigned(root->translate.vec.f.y) -
                ndsFloatToMilliSigned(root_y_start);
            gNdsFighterJumpP1VelXAfterMilli =
                ndsFloatToMilliSigned(fp->physics.vel_air.x);
            gNdsFighterJumpP1VelYAfterMilli =
                ndsFloatToMilliSigned(fp->physics.vel_air.y);
            if ((gNdsFighterJumpP1RootDeltaXMilli * fp->lr) > 0)
            {
                gNdsFighterJumpP1RootDirectionOK = 1u;
            }
            if (gNdsFighterJumpP1RootDeltaYMilli > 0)
            {
                gNdsFighterJumpP1RootRiseOK = 1u;
            }
        }
        gNdsFighterMarioFoxJumpLoopCount++;
    }

    if ((gNdsFighterJumpRunBrakeEndCallCount == 2u) &&
        (gNdsFighterJumpWaitSetStatusCount == 2u) &&
        (gNdsFighterJumpP0StatusWait == (u32)nFTCommonStatusWait) &&
        (gNdsFighterJumpP1StatusWait == (u32)nFTCommonStatusWait) &&
        (gNdsFighterJumpP0MotionWait == (u32)nFTCommonMotionWait) &&
        (gNdsFighterJumpP1MotionWait == (u32)nFTCommonMotionWait))
    {
        mask |= 1u << 1;
    }
    if ((gNdsFighterJumpP0ButtonTap == U_CBUTTONS) &&
        (gNdsFighterJumpP1ButtonTap == U_CBUTTONS) &&
        (gNdsFighterJumpP0InputSource ==
            FTCOMMON_KNEEBEND_INPUT_TYPE_BUTTON) &&
        (gNdsFighterJumpP1InputSource ==
            FTCOMMON_KNEEBEND_INPUT_TYPE_BUTTON) &&
        (gNdsFighterJumpP0ShortHop == 0u) &&
        (gNdsFighterJumpP1ShortHop == 0u))
    {
        mask |= 1u << 2;
    }
    if ((gNdsFighterJumpOriginalKneeBendCheckCallCount == 2u) &&
        (gNdsFighterJumpOriginalKneeBendCheckSuccessCount == 2u) &&
        (gNdsFighterJumpKneeBendSetStatusCallCount == 2u) &&
        (gNdsFighterJumpFtMainKneeBendStatusCount == 2u) &&
        (gNdsFighterJumpP0StatusKneeBend ==
            (u32)nFTCommonStatusKneeBend) &&
        (gNdsFighterJumpP1StatusKneeBend ==
            (u32)nFTCommonStatusKneeBend) &&
        (gNdsFighterJumpP0MotionKneeBend ==
            (u32)nFTCommonMotionKneeBend) &&
        (gNdsFighterJumpP1MotionKneeBend ==
            (u32)nFTCommonMotionKneeBend))
    {
        mask |= 1u << 3;
    }
    if ((gNdsFighterJumpSetStatusCallCount == 2u) &&
        (gNdsFighterJumpFtMainJumpStatusCount == 2u) &&
        (gNdsFighterJumpP0StatusJump == (u32)nFTCommonStatusJumpF) &&
        (gNdsFighterJumpP1StatusJump == (u32)nFTCommonStatusJumpF) &&
        (gNdsFighterJumpP0MotionJump == (u32)nFTCommonMotionJumpF) &&
        (gNdsFighterJumpP1MotionJump == (u32)nFTCommonMotionJumpF))
    {
        mask |= 1u << 4;
    }
    if ((gNdsFighterJumpSetAirCallCount == 2u) &&
        (gNdsFighterJumpP0GAJump == (u32)nMPKineticsAir) &&
        (gNdsFighterJumpP1GAJump == (u32)nMPKineticsAir) &&
        (gNdsFighterJumpP0VelYInitialMilli > 0) &&
        (gNdsFighterJumpP1VelYInitialMilli > 0))
    {
        mask |= 1u << 5;
    }
    if ((gNdsFighterJumpP0AirFrames == 6u) &&
        (gNdsFighterJumpP1AirFrames == 6u) &&
        (gNdsFighterJumpAirInterruptCallCount == 12u) &&
        (gNdsFighterJumpAirPhysicsCallCount == 12u) &&
        (gNdsFighterJumpAirMapCallCount == 12u))
    {
        mask |= 1u << 6;
    }
    if ((gNdsFighterJumpP0RootDirectionOK == 1u) &&
        (gNdsFighterJumpP1RootDirectionOK == 1u) &&
        (gNdsFighterJumpP0RootRiseOK == 1u) &&
        (gNdsFighterJumpP1RootRiseOK == 1u))
    {
        mask |= 1u << 7;
    }
    if ((gNdsFighterJumpGravityCallCount >= 12u) &&
        (gNdsFighterJumpAirDriftCallCount >= 12u) &&
        (gNdsFighterJumpAirFrictionCallCount >= 12u))
    {
        mask |= 1u << 8;
    }
    if ((gNdsFighterJumpDeferredInterruptCheckCount >= 24u) &&
        (gNdsFighterJumpLandingDeniedCount == 0u) &&
        (gNdsFighterJumpFallDeferredCount == 0u) &&
        (gNdsFighterJumpCliffDeniedCount == 0u) &&
        (gNdsFighterJumpCeilingDeniedCount == 0u))
    {
        mask |= 1u << 9;
    }

    gobj_after = (u32)gcGetGObjsActiveNum();
    gNdsFighterJumpGObjDelta =
        (gobj_after >= gobj_before) ? (gobj_after - gobj_before) :
            (gobj_before - gobj_after);
    if ((gNdsFighterJumpGObjDelta == 0u) &&
        (gNdsFighterJumpDeniedStatusCount == 0u) &&
        (gNdsFighterJumpUnexpectedStatusCount == 0u) &&
        (gNdsFighterJumpProcessAttachCount == 0u) &&
        (gNdsFighterJumpDisplayProbeCount == 0u) &&
        (gNdsFighterJumpGameplayUpdateCount == 0u) &&
        (gNdsFighterJumpDrawCallCount == 0u) &&
        (gNdsFighterJumpMatrixCallCount == 0u) &&
        (gNdsFighterMarioFoxJumpLoopCount == 2u))
    {
        mask |= 1u << 10;
    }
    if ((ndsFighterMarioFoxJumpAttackAirProofEnabled() != FALSE) &&
        (gNdsFighterJumpAttackAirCheckSuccessCount == 1u) &&
        (gNdsFighterJumpAttackAirSetStatusCount == 1u) &&
        (gNdsFighterJumpFtMainAttackAirStatusCount == 1u) &&
        (gNdsFighterJumpAttackAirAnimEventsCount == 1u) &&
        (gNdsFighterJumpAttackAirStatusAfter ==
            (u32)nFTCommonStatusAttackAirN) &&
        (gNdsFighterJumpAttackAirMotionAfter ==
            (u32)nFTCommonMotionAttackAirN) &&
        (gNdsFighterJumpAttackAirGAAfter == (u32)nMPKineticsAir) &&
        (gNdsFighterJumpAttackAirMotionAttackIDAfter ==
            nFTMotionAttackIDAttackAirN) &&
        (gNdsFighterJumpAttackAirStatusAttackIDAfter ==
            nFTStatusAttackIDAttackAirN) &&
        (gNdsFighterJumpAttackAirStatAttackIDAfter ==
            nFTStatusAttackIDAttackAirN) &&
        (gNdsFighterJumpAttackAirTicsSinceLastZAfter ==
            FTINPUT_ZTRIGLAST_TICS_MAX) &&
        ((gNdsFighterJumpAttackAirCallbackMask & 0xfu) == 0xfu) &&
        (gNdsFighterJumpAttackAirRefreshCount == 2u) &&
        ((gNdsFighterJumpAttackAirRefreshMask & 0x3u) == 0x3u) &&
        ((gNdsFighterJumpAttackAirRefreshStateMask & 0x7u) == 0x7u) &&
        ((gNdsFighterJumpAttackAirRecordClearMask & 0x3u) == 0x3u) &&
        ((gNdsFighterJumpAttackAirMapLandingMask & 0x3ffu) == 0x3ffu) &&
        ((gNdsFighterJumpAttackAirDirectionMask & 0x1fu) == 0x1fu))
    {
        mask |= 1u << 11;
    }

    gNdsFighterMarioFoxJumpLoopMask = mask;
    gNdsFighterMarioFoxJumpLoopDeferredMask |= 0xffu;
    required_mask =
        (ndsFighterMarioFoxJumpAttackAirProofEnabled() != FALSE) ?
            0xfffu : 0x7ffu;
    if ((mask & required_mask) == required_mask)
    {
        gNdsFighterMarioFoxJumpLoopResult =
            NDS_FIGHTER_MARIOFOX_JUMP_LOOP_PASS;
        gNdsFighterMarioFoxJumpLoopSafeResult =
            NDS_FIGHTER_MARIOFOX_JUMP_LOOP_SAFE_PASS;
    }
    ndsFighterMarioFoxRunLandingLoopProof();
}

static void ndsFighterMarioFoxRunLandingLoopProof(void)
{
    u32 mask = 0u;
    u32 gobj_before;
    u32 gobj_after;
    u32 i;

    if ((ndsFighterMarioFoxLandingLoopProofEnabled() == FALSE) ||
        (gNdsFighterMarioFoxLandingLoopResult != 0u))
    {
        return;
    }
    if ((gNdsFighterMarioFoxJumpLoopResult !=
            NDS_FIGHTER_MARIOFOX_JUMP_LOOP_PASS) ||
        (gNdsFighterMarioFoxJumpLoopSafeResult !=
            NDS_FIGHTER_MARIOFOX_JUMP_LOOP_SAFE_PASS))
    {
        return;
    }
    mask |= 1u << 0;
    gobj_before = (u32)gcGetGObjsActiveNum();
    gNdsFighterLandingFallFrameMax = 128u;
    gNdsFighterLandingLandingFrameTarget = 5u;

    for (i = 0u; i < 2u; i++)
    {
        FTStruct *fp = &sNdsFighterStructPool[i];
        GObj *fighter_gobj;
        DObj *root;
        FTAttributes *attr;
        f32 root_x_start;
        f32 floor_y;
        u32 frame;
        u32 fall_frames = 0u;
        u32 landing_frames = 0u;

        if ((ndsFighterStructIsPoolPointer(fp) == FALSE) ||
            (fp->fighter_gobj == NULL) ||
            (fp->status_id != nFTCommonStatusJumpF) ||
            (fp->motion_id != nFTCommonMotionJumpF) ||
            (fp->ga != nMPKineticsAir))
        {
            continue;
        }
        fighter_gobj = fp->fighter_gobj;
        root = fp->joints[nFTPartsJointTopN];
        attr = fp->attr;
        if ((root == NULL) || (attr == NULL))
        {
            continue;
        }
        if (attr->gravity <= 0.0F)
        {
            attr->gravity = 0.08F;
        }
        if (attr->tvel_base <= 0.0F)
        {
            attr->tvel_base = 6.0F;
        }
        if (attr->tvel_fast <= 0.0F)
        {
            attr->tvel_fast = 100.0F;
        }
        if (attr->air_accel <= 0.0F)
        {
            attr->air_accel = 0.002F;
        }
        if (attr->air_speed_max_x <= 0.0F)
        {
            attr->air_speed_max_x = 3.0F;
        }
        if (attr->air_friction <= 0.0F)
        {
            attr->air_friction = 0.02F;
        }
        if (attr->jumps_max <= fp->jumps_used)
        {
            attr->jumps_max = fp->jumps_used + 1;
        }

        root_x_start = root->translate.vec.f.x;
        floor_y = fp->coll_data.floor_dist;

        if (i == 0u)
        {
            gNdsFighterLandingP0StatusStart = (u32)fp->status_id;
            gNdsFighterLandingP0MotionStart = (u32)fp->motion_id;
            gNdsFighterLandingP0GAStart = (u32)fp->ga;
            gNdsFighterLandingP0FloorYMilli =
                ndsFloatToMilliSigned(floor_y);
            gNdsFighterLandingP0RootYFallStartMilli =
                ndsFloatToMilliSigned(root->translate.vec.f.y);
        }
        else
        {
            gNdsFighterLandingP1StatusStart = (u32)fp->status_id;
            gNdsFighterLandingP1MotionStart = (u32)fp->motion_id;
            gNdsFighterLandingP1GAStart = (u32)fp->ga;
            gNdsFighterLandingP1FloorYMilli =
                ndsFloatToMilliSigned(floor_y);
            gNdsFighterLandingP1RootYFallStartMilli =
                ndsFloatToMilliSigned(root->translate.vec.f.y);
        }

        sNdsFighterLandingJumpAnimEndActive = TRUE;
        ftAnimEndSetFall(fighter_gobj);
        sNdsFighterLandingJumpAnimEndActive = FALSE;

        fp->physics.vel_air.y = -6.0F;
        fp->vel_air = fp->physics.vel_air;
        if (i == 0u)
        {
            gNdsFighterLandingP0StatusFall = (u32)fp->status_id;
            gNdsFighterLandingP0MotionFall = (u32)fp->motion_id;
            gNdsFighterLandingP0GAFall = (u32)fp->ga;
            gNdsFighterLandingP0VelYFallStartMilli =
                ndsFloatToMilliSigned(fp->physics.vel_air.y);
        }
        else
        {
            gNdsFighterLandingP1StatusFall = (u32)fp->status_id;
            gNdsFighterLandingP1MotionFall = (u32)fp->motion_id;
            gNdsFighterLandingP1GAFall = (u32)fp->ga;
            gNdsFighterLandingP1VelYFallStartMilli =
                ndsFloatToMilliSigned(fp->physics.vel_air.y);
        }

        for (frame = 0u; frame < gNdsFighterLandingFallFrameMax; frame++)
        {
            if ((fp->status_id != nFTCommonStatusFall) ||
                (fp->ga != nMPKineticsAir))
            {
                break;
            }
            fp->input.pl.stick_range.x = (s8)((fp->lr >= 0) ? 40 : -40);
            fp->input.pl.stick_range.y = 0;
            fp->input.pl.button_tap = 0;
            fp->input.pl.button_release = 0;
            sNdsFighterLandingFallInterruptActive = TRUE;
            ftCommonFallProcInterrupt(fighter_gobj);
            sNdsFighterLandingFallInterruptActive = FALSE;
            sNdsFighterLandingFallPhysicsActive = TRUE;
            ftPhysicsApplyAirVelDriftFastFall(fighter_gobj);
            sNdsFighterLandingFallPhysicsActive = FALSE;
            root->translate.vec.f.x += fp->physics.vel_air.x;
            root->translate.vec.f.y += fp->physics.vel_air.y;
            root->translate.vec.f.z += fp->physics.vel_air.z;
            sNdsFighterLandingFallMapActive = TRUE;
            mpCommonProcFighterCliffFloorCeil(fighter_gobj);
            sNdsFighterLandingFallMapActive = FALSE;
            fall_frames++;
        }

        if (i == 0u)
        {
            gNdsFighterLandingP0FallFrameCount = fall_frames;
            gNdsFighterLandingP0FallInterruptCount = fall_frames;
            gNdsFighterLandingP0FallPhysicsCount = fall_frames;
            gNdsFighterLandingP0StatusLanding = (u32)fp->status_id;
            gNdsFighterLandingP0MotionLanding = (u32)fp->motion_id;
            gNdsFighterLandingP0GALanding = (u32)fp->ga;
            gNdsFighterLandingP0GroundVelAfterLandingMilli =
                ndsFloatToMilliSigned(fp->physics.vel_ground.x);
        }
        else
        {
            gNdsFighterLandingP1FallFrameCount = fall_frames;
            gNdsFighterLandingP1FallInterruptCount = fall_frames;
            gNdsFighterLandingP1FallPhysicsCount = fall_frames;
            gNdsFighterLandingP1StatusLanding = (u32)fp->status_id;
            gNdsFighterLandingP1MotionLanding = (u32)fp->motion_id;
            gNdsFighterLandingP1GALanding = (u32)fp->ga;
            gNdsFighterLandingP1GroundVelAfterLandingMilli =
                ndsFloatToMilliSigned(fp->physics.vel_ground.x);
        }

        for (frame = 0u; frame < gNdsFighterLandingLandingFrameTarget; frame++)
        {
            if (fp->status_id != nFTCommonStatusLandingLight)
            {
                break;
            }
            fp->input.pl.stick_range.x = 0;
            fp->input.pl.stick_range.y = 0;
            fp->input.pl.button_hold = 0;
            fp->input.pl.button_tap = 0;
            fp->input.pl.button_release = 0;
            fighter_gobj->anim_frame = (f32)frame + 1.0F;
            sNdsFighterLandingProcInterruptActive = TRUE;
            ftCommonLandingProcInterrupt(fighter_gobj);
            sNdsFighterLandingProcInterruptActive = FALSE;
            sNdsFighterLandingPhysicsActive = TRUE;
            ftPhysicsApplyGroundVelFriction(fighter_gobj);
            sNdsFighterLandingPhysicsActive = FALSE;
            landing_frames++;
        }
        if (i == 0u)
        {
            gNdsFighterLandingP0LandingFrameCount = landing_frames;
            gNdsFighterLandingP0LandingInterruptCount = landing_frames;
            gNdsFighterLandingP0LandingPhysicsCount = landing_frames;
        }
        else
        {
            gNdsFighterLandingP1LandingFrameCount = landing_frames;
            gNdsFighterLandingP1LandingInterruptCount = landing_frames;
            gNdsFighterLandingP1LandingPhysicsCount = landing_frames;
        }

        fighter_gobj->anim_frame = 0.0F;
        fp->anim_frame = 0.0F;
        sNdsFighterLandingEndActive = TRUE;
        ftAnimEndSetWait(fighter_gobj);
        sNdsFighterLandingEndActive = FALSE;

        sNdsFighterLandingWaitSettleActive = TRUE;
        ftPhysicsApplyGroundVelFriction(fighter_gobj);
        mpCommonProcFighterOnCliffEdge(fighter_gobj);
        sNdsFighterLandingWaitSettleActive = FALSE;

        if (i == 0u)
        {
            gNdsFighterLandingP0StatusWait = (u32)fp->status_id;
            gNdsFighterLandingP0MotionWait = (u32)fp->motion_id;
            gNdsFighterLandingP0GAWait = (u32)fp->ga;
            gNdsFighterLandingP0RootYFinalMilli =
                ndsFloatToMilliSigned(root->translate.vec.f.y);
            gNdsFighterLandingP0RootDeltaXMilli =
                ndsFloatToMilliSigned(root->translate.vec.f.x) -
                ndsFloatToMilliSigned(root_x_start);
            gNdsFighterLandingP0GroundVelAfterWaitMilli =
                ndsFloatToMilliSigned(fp->physics.vel_ground.x);
            if ((gNdsFighterLandingP0RootDeltaXMilli * fp->lr) > 0)
            {
                gNdsFighterLandingP0RootDirectionOK = 1u;
            }
            if (gNdsFighterLandingP0RootYFinalMilli ==
                gNdsFighterLandingP0FloorYMilli)
            {
                gNdsFighterLandingP0RootFloorOK = 1u;
            }
        }
        else
        {
            gNdsFighterLandingP1StatusWait = (u32)fp->status_id;
            gNdsFighterLandingP1MotionWait = (u32)fp->motion_id;
            gNdsFighterLandingP1GAWait = (u32)fp->ga;
            gNdsFighterLandingP1RootYFinalMilli =
                ndsFloatToMilliSigned(root->translate.vec.f.y);
            gNdsFighterLandingP1RootDeltaXMilli =
                ndsFloatToMilliSigned(root->translate.vec.f.x) -
                ndsFloatToMilliSigned(root_x_start);
            gNdsFighterLandingP1GroundVelAfterWaitMilli =
                ndsFloatToMilliSigned(fp->physics.vel_ground.x);
            if ((gNdsFighterLandingP1RootDeltaXMilli * fp->lr) > 0)
            {
                gNdsFighterLandingP1RootDirectionOK = 1u;
            }
            if (gNdsFighterLandingP1RootYFinalMilli ==
                gNdsFighterLandingP1FloorYMilli)
            {
                gNdsFighterLandingP1RootFloorOK = 1u;
            }
        }
        if ((fp->ga != nMPKineticsGround) ||
            (fp->status_id != nFTCommonStatusWait))
        {
            gNdsFighterLandingGADriftCount++;
        }
        if (root->translate.vec.f.y != floor_y)
        {
            gNdsFighterLandingRootYDriftCount++;
        }
        gNdsFighterMarioFoxLandingLoopCount++;
    }

    if ((gNdsFighterLandingP0StatusStart == (u32)nFTCommonStatusJumpF) &&
        (gNdsFighterLandingP1StatusStart == (u32)nFTCommonStatusJumpF) &&
        (gNdsFighterLandingP0MotionStart == (u32)nFTCommonMotionJumpF) &&
        (gNdsFighterLandingP1MotionStart == (u32)nFTCommonMotionJumpF))
    {
        mask |= 1u << 1;
    }
    if ((gNdsFighterLandingP0StatusFall == (u32)nFTCommonStatusFall) &&
        (gNdsFighterLandingP1StatusFall == (u32)nFTCommonStatusFall) &&
        (gNdsFighterLandingP0MotionFall == (u32)nFTCommonMotionFall) &&
        (gNdsFighterLandingP1MotionFall == (u32)nFTCommonMotionFall))
    {
        mask |= 1u << 2;
    }
    if ((gNdsFighterLandingP0StatusLanding ==
            (u32)nFTCommonStatusLandingLight) &&
        (gNdsFighterLandingP1StatusLanding ==
            (u32)nFTCommonStatusLandingLight) &&
        (gNdsFighterLandingP0MotionLanding ==
            (u32)nFTCommonMotionLandingLight) &&
        (gNdsFighterLandingP1MotionLanding ==
            (u32)nFTCommonMotionLandingLight))
    {
        mask |= 1u << 3;
    }
    if ((gNdsFighterLandingP0StatusWait == (u32)nFTCommonStatusWait) &&
        (gNdsFighterLandingP1StatusWait == (u32)nFTCommonStatusWait) &&
        (gNdsFighterLandingP0MotionWait == (u32)nFTCommonMotionWait) &&
        (gNdsFighterLandingP1MotionWait == (u32)nFTCommonMotionWait))
    {
        mask |= 1u << 4;
    }
    if ((gNdsFighterLandingP0GAStart == (u32)nMPKineticsAir) &&
        (gNdsFighterLandingP1GAStart == (u32)nMPKineticsAir) &&
        (gNdsFighterLandingP0GAFall == (u32)nMPKineticsAir) &&
        (gNdsFighterLandingP1GAFall == (u32)nMPKineticsAir) &&
        (gNdsFighterLandingP0GALanding == (u32)nMPKineticsGround) &&
        (gNdsFighterLandingP1GALanding == (u32)nMPKineticsGround) &&
        (gNdsFighterLandingP0GAWait == (u32)nMPKineticsGround) &&
        (gNdsFighterLandingP1GAWait == (u32)nMPKineticsGround))
    {
        mask |= 1u << 5;
    }
    if ((gNdsFighterLandingJumpAnimEndCallCount == 2u) &&
        (gNdsFighterLandingFallSetStatusCallCount == 2u) &&
        (gNdsFighterLandingFtMainFallStatusCount == 2u) &&
        (gNdsFighterLandingSetGroundCallCount == 2u) &&
        (gNdsFighterLandingSetStatusCallCount == 2u) &&
        (gNdsFighterLandingFtMainLandingLightStatusCount == 2u) &&
        (gNdsFighterLandingFtMainLandingHeavyStatusCount == 0u) &&
        (gNdsFighterLandingEndCallCount == 2u) &&
        (gNdsFighterLandingWaitSetStatusCount == 2u) &&
        (gNdsFighterLandingWaitSetStatusSuccessCount == 2u))
    {
        mask |= 1u << 6;
    }
    if ((gNdsFighterLandingP0FallFrameCount > 0u) &&
        (gNdsFighterLandingP1FallFrameCount > 0u) &&
        (gNdsFighterLandingP0FallFrameCount <=
            gNdsFighterLandingFallFrameMax) &&
        (gNdsFighterLandingP1FallFrameCount <=
            gNdsFighterLandingFallFrameMax) &&
        (gNdsFighterLandingP0LandingFrameCount ==
            gNdsFighterLandingLandingFrameTarget) &&
        (gNdsFighterLandingP1LandingFrameCount ==
            gNdsFighterLandingLandingFrameTarget))
    {
        mask |= 1u << 7;
    }
    if ((gNdsFighterLandingAirNoCollisionCount > 0u) &&
        (gNdsFighterLandingFloorDetectCount == 2u) &&
        (gNdsFighterLandingFloorClampCount == 2u) &&
        (gNdsFighterLandingP0RootFloorOK == 1u) &&
        (gNdsFighterLandingP1RootFloorOK == 1u))
    {
        mask |= 1u << 8;
    }
    if ((gNdsFighterLandingP0RootDirectionOK == 1u) &&
        (gNdsFighterLandingP1RootDirectionOK == 1u) &&
        (gNdsFighterLandingP0VelYBeforeLandingMilli <= 0) &&
        (gNdsFighterLandingP1VelYBeforeLandingMilli <= 0) &&
        (gNdsFighterLandingGravityCallCount >=
            (gNdsFighterLandingP0FallFrameCount +
             gNdsFighterLandingP1FallFrameCount)) &&
        (gNdsFighterLandingFastFallCheckCount >=
            (gNdsFighterLandingP0FallFrameCount +
             gNdsFighterLandingP1FallFrameCount)) &&
        (gNdsFighterLandingGroundFrictionCallCount >= 2u) &&
        (gNdsFighterLandingWaitFrictionCallCount >= 2u))
    {
        mask |= 1u << 9;
    }

    gobj_after = (u32)gcGetGObjsActiveNum();
    gNdsFighterLandingGObjDelta =
        (gobj_after >= gobj_before) ? (gobj_after - gobj_before) :
            (gobj_before - gobj_after);
    if ((gNdsFighterLandingGObjDelta == 0u) &&
        (gNdsFighterLandingUnexpectedStatusCount == 0u) &&
        (gNdsFighterLandingDeniedStatusCount == 0u) &&
        (gNdsFighterLandingProcessAttachCount == 0u) &&
        (gNdsFighterLandingDisplayProbeCount == 0u) &&
        (gNdsFighterLandingGameplayUpdateCount == 0u) &&
        (gNdsFighterLandingDrawCallCount == 0u) &&
        (gNdsFighterLandingMatrixCallCount == 0u) &&
        (gNdsFighterLandingRootYDriftCount == 0u) &&
        (gNdsFighterLandingGADriftCount == 0u) &&
        (gNdsFighterLandingFastFallCount == 0u) &&
        (gNdsFighterLandingHeavyDeniedCount == 0u) &&
        (gNdsFighterLandingFallAerialDeniedCount == 0u) &&
        (gNdsFighterLandingJumpAerialDeniedCount == 0u) &&
        (gNdsFighterLandingCliffDeniedCount == 0u) &&
        (gNdsFighterLandingCeilingDeniedCount == 0u) &&
        (gNdsFighterMarioFoxLandingLoopCount == 2u))
    {
        mask |= 1u << 10;
    }

    gNdsFighterMarioFoxLandingLoopMask = mask;
    gNdsFighterMarioFoxLandingLoopDeferredMask = 0xffu;
    if ((mask & 0x7ffu) == 0x7ffu)
    {
        gNdsFighterMarioFoxLandingLoopResult =
            NDS_FIGHTER_MARIOFOX_LANDING_LOOP_PASS;
        gNdsFighterMarioFoxLandingLoopSafeResult =
            NDS_FIGHTER_MARIOFOX_LANDING_LOOP_SAFE_PASS;
    }
    ndsFighterMarioFoxRunProcessLoopProof();
}

#define NDS_FIGHTER_PROCESS_LOOP_MAX_FRAMES 160u
#define NDS_FIGHTER_PROCESS_LOOP_STATUS_MASK_REQUIRED 0x3ffu
#define NDS_FIGHTER_PROCESS_LOOP_TRANSITION_MASK_REQUIRED 0x7ffu

static void ndsFighterProcessLoopSetStatus(FTStruct *fp, GObj *fighter_gobj,
                                           s32 status_id, f32 frame_begin,
                                           f32 anim_speed, u32 flags)
{
    if ((fp == NULL) || (fighter_gobj == NULL))
    {
        gNdsFighterProcessLoopDeniedStatusCount++;
        return;
    }

    if ((status_id == nFTCommonStatusFallAerial) &&
        (sNdsFighterProcessLoopJumpAnimEndActive != FALSE))
    {
        status_id = nFTCommonStatusFall;
    }

    switch (status_id)
    {
    case nFTCommonStatusWait:
    case nFTCommonStatusWalkSlow:
    case nFTCommonStatusWalkMiddle:
    case nFTCommonStatusWalkFast:
    case nFTCommonStatusDash:
    case nFTCommonStatusRun:
    case nFTCommonStatusRunBrake:
    case nFTCommonStatusKneeBend:
    case nFTCommonStatusJumpF:
    case nFTCommonStatusFall:
    case nFTCommonStatusLandingLight:
        break;
    default:
        gNdsFighterProcessLoopDeniedStatusCount++;
        gNdsFighterProcessLoopUnexpectedStatusCount++;
        return;
    }

    ndsFTMainApplyCommonStatusReset(fp, flags);
    fp->status_prev = fp->status_id;
    fp->status_id = status_id;
    fp->status_total_tics = 0;
    fp->motion_attack_id = nFTMotionAttackIDNone;
    fp->status_attack_id = nFTStatusAttackIDNone;
    fp->stat_attack_id = nFTStatusAttackIDNone;
    fp->status_is_smash = FALSE;
    fp->status_is_projectile = FALSE;
    fp->status_flags = flags;
    fp->motion_frame = frame_begin;
    fp->anim_frame = frame_begin;
    fp->anim_speed = anim_speed;
    fp->proc_status = NULL;
    fighter_gobj->anim_frame = frame_begin;

    switch (status_id)
    {
    case nFTCommonStatusWait:
        fp->motion_id = nFTCommonMotionWait;
        fp->proc_update = NULL;
        fp->proc_interrupt = ftCommonWaitProcInterrupt;
        fp->proc_physics = ftPhysicsApplyGroundVelFriction;
        fp->proc_map = mpCommonProcFighterOnCliffEdge;
        fp->is_special_interrupt = TRUE;
        fp->is_wait_status_setup = TRUE;
        fp->is_wait_motion_setup = TRUE;
        gNdsFighterProcessLoopWaitSetStatusCount++;
        break;
    case nFTCommonStatusWalkSlow:
    case nFTCommonStatusWalkMiddle:
    case nFTCommonStatusWalkFast:
        fp->motion_id = nFTCommonMotionWalkSlow +
            (status_id - nFTCommonStatusWalkSlow);
        fp->proc_update = NULL;
        fp->proc_interrupt = ftCommonWalkProcInterrupt;
        fp->proc_physics = ftCommonWalkProcPhysics;
        fp->proc_map = mpCommonProcFighterOnCliffEdge;
        break;
    case nFTCommonStatusDash:
        fp->motion_id = nFTCommonMotionDash;
        fp->proc_update = ftCommonDashProcUpdate;
        fp->proc_interrupt = ftCommonDashProcInterrupt;
        fp->proc_physics = ftCommonDashProcPhysics;
        fp->proc_map = ftCommonDashProcMap;
        break;
    case nFTCommonStatusRun:
        fp->motion_id = nFTCommonMotionRun;
        fp->proc_update = NULL;
        fp->proc_interrupt = ftCommonRunProcInterrupt;
        fp->proc_physics = ftPhysicsSetGroundVelTransferAir;
        fp->proc_map = mpCommonProcFighterOnCliffEdge;
        break;
    case nFTCommonStatusRunBrake:
        fp->motion_id = nFTCommonMotionRunBrake;
        fp->proc_update = NULL;
        fp->proc_interrupt = ftCommonRunBrakeProcInterrupt;
        fp->proc_physics = ftCommonRunBrakeProcPhysics;
        fp->proc_map = mpCommonProcFighterOnCliffEdge;
        break;
    case nFTCommonStatusKneeBend:
        fp->motion_id = nFTCommonMotionKneeBend;
        fp->proc_update = ftCommonKneeBendProcUpdate;
        fp->proc_interrupt = ftCommonKneeBendProcInterrupt;
        fp->proc_physics = NULL;
        fp->proc_map = NULL;
        break;
    case nFTCommonStatusJumpF:
        fp->motion_id = nFTCommonMotionJumpF;
        fp->proc_update = ftAnimEndSetFall;
        fp->proc_interrupt = ftCommonJumpProcInterrupt;
        fp->proc_physics = ftPhysicsApplyAirVelDriftFastFall;
        fp->proc_map = mpCommonProcFighterCliffFloorCeil;
        break;
    case nFTCommonStatusFall:
        fp->motion_id = nFTCommonMotionFall;
        fp->proc_update = NULL;
        fp->proc_interrupt = ftCommonFallProcInterrupt;
        fp->proc_physics = ftPhysicsApplyAirVelDriftFastFall;
        fp->proc_map = mpCommonProcFighterCliffFloorCeil;
        break;
    case nFTCommonStatusLandingLight:
        fp->motion_id = nFTCommonMotionLandingLight;
        fp->proc_update = ftAnimEndSetWait;
        fp->proc_interrupt = ftCommonLandingProcInterrupt;
        fp->proc_physics = ftPhysicsApplyGroundVelFriction;
        fp->proc_map = mpCommonProcFighterOnCliffEdge;
        break;
    default:
        break;
    }
    fp->motion_script_id = fp->motion_id;
    if (DObjGetStruct(fighter_gobj) != NULL)
    {
        DObjGetStruct(fighter_gobj)->anim_speed = anim_speed;
    }
}

static void ndsFighterProcessLoopApplyScriptInput(
    u32 slot, FTStruct *fp, const NDSFighterScriptInput *input)
{
    if ((fp == NULL) || (input == NULL))
    {
        return;
    }

    fp->input.pl.stick_range.x = input->stick_x;
    fp->input.pl.stick_range.y = input->stick_y;
    fp->input.pl.button_hold = input->button_hold;
    fp->input.pl.button_tap = input->button_tap;
    fp->input.pl.button_release = input->button_release;
    fp->tap_stick_x = input->tap_stick_x;
    fp->tap_stick_y = input->tap_stick_y;
    fp->hold_stick_x = input->hold_stick_x;
    fp->hold_stick_y = input->hold_stick_y;

    if (slot == 0u)
    {
        gNdsFighterProcessLoopP0InputApplyCount++;
        gNdsFighterProcessLoopP0ButtonTapMask |= input->button_tap;
        gNdsFighterProcessLoopP0LastStickX = input->stick_x;
    }
    else if (slot == 1u)
    {
        gNdsFighterProcessLoopP1InputApplyCount++;
        gNdsFighterProcessLoopP1ButtonTapMask |= input->button_tap;
        gNdsFighterProcessLoopP1LastStickX = input->stick_x;
    }

    if (slot < MAXCONTROLLERS)
    {
        gSYControllerDevices[slot].button_hold = input->button_hold;
        gSYControllerDevices[slot].button_tap = input->button_tap;
        gSYControllerDevices[slot].button_release = input->button_release;
        gSYControllerDevices[slot].stick_range.x = input->stick_x;
        gSYControllerDevices[slot].stick_range.y = input->stick_y;
        gNdsFighterProcessLoopControllerBridgeCount++;
        if ((gSYControllerDevices[slot].button_tap == fp->input.pl.button_tap) &&
            (gSYControllerDevices[slot].button_hold == fp->input.pl.button_hold) &&
            (gSYControllerDevices[slot].button_release ==
                fp->input.pl.button_release) &&
            (gSYControllerDevices[slot].stick_range.x ==
                fp->input.pl.stick_range.x) &&
            (gSYControllerDevices[slot].stick_range.y ==
                fp->input.pl.stick_range.y))
        {
            gNdsFighterProcessLoopControllerMirrorCount++;
        }
    }
}

static void ndsFighterProcessLoopRunUpdate(u32 slot, FTStruct *fp)
{
    if ((fp != NULL) && (fp->proc_update != NULL) && (fp->fighter_gobj != NULL))
    {
        if ((sNdsFighterProcessLoopActive != FALSE) &&
            ((fp->proc_update == ftAnimEndSetFall) ||
             (fp->proc_update == ftAnimEndSetWait)))
        {
            return;
        }
        sNdsFighterProcessLoopUpdateActive = TRUE;
        fp->proc_update(fp->fighter_gobj);
        sNdsFighterProcessLoopUpdateActive = FALSE;
        if (slot == 0u)
        {
            gNdsFighterProcessLoopP0UpdateCount++;
        }
        else if (slot == 1u)
        {
            gNdsFighterProcessLoopP1UpdateCount++;
        }
    }
}

static void ndsFighterProcessLoopRunInterrupt(u32 slot, FTStruct *fp)
{
    if ((fp != NULL) && (fp->proc_interrupt != NULL) &&
        (fp->fighter_gobj != NULL))
    {
        sNdsFighterProcessLoopInterruptActive = TRUE;
        fp->proc_interrupt(fp->fighter_gobj);
        sNdsFighterProcessLoopInterruptActive = FALSE;
        if (slot == 0u)
        {
            gNdsFighterProcessLoopP0InterruptCount++;
        }
        else if (slot == 1u)
        {
            gNdsFighterProcessLoopP1InterruptCount++;
        }
    }
}

static void ndsFighterProcessLoopRunPhysics(u32 slot, FTStruct *fp)
{
    if ((fp != NULL) && (fp->proc_physics != NULL) &&
        (fp->fighter_gobj != NULL))
    {
        sNdsFighterProcessLoopPhysicsActive = TRUE;
        fp->proc_physics(fp->fighter_gobj);
        sNdsFighterProcessLoopPhysicsActive = FALSE;
        if (slot == 0u)
        {
            gNdsFighterProcessLoopP0PhysicsCount++;
        }
        else if (slot == 1u)
        {
            gNdsFighterProcessLoopP1PhysicsCount++;
        }
    }
}

static void ndsFighterProcessLoopIntegrate(u32 slot, FTStruct *fp)
{
    DObj *root = (fp != NULL) ? fp->joints[nFTPartsJointTopN] : NULL;

    if (root == NULL)
    {
        gNdsFighterProcessLoopProcessAttachCount++;
        return;
    }
    root->translate.vec.f.x += fp->physics.vel_air.x;
    root->translate.vec.f.y += fp->physics.vel_air.y;
    root->translate.vec.f.z += fp->physics.vel_air.z;
    if (slot == 0u)
    {
        gNdsFighterProcessLoopP0IntegrateCount++;
    }
    else if (slot == 1u)
    {
        gNdsFighterProcessLoopP1IntegrateCount++;
    }
}

static void ndsFighterProcessLoopRunMap(u32 slot, FTStruct *fp)
{
    if ((fp != NULL) && (fp->proc_map != NULL) && (fp->fighter_gobj != NULL))
    {
        sNdsFighterProcessLoopMapActive = TRUE;
        fp->proc_map(fp->fighter_gobj);
        sNdsFighterProcessLoopMapActive = FALSE;
        if (slot == 0u)
        {
            gNdsFighterProcessLoopP0MapCount++;
        }
        else if (slot == 1u)
        {
            gNdsFighterProcessLoopP1MapCount++;
        }
    }
}

static void ndsFighterProcessLoopRunFrame(u32 slot, FTStruct *fp)
{
    if ((fp == NULL) || (fp->fighter_gobj == NULL))
    {
        return;
    }

    fp->status_total_tics++;
    fp->fighter_gobj->anim_frame += 1.0F;
    ndsFighterProcessLoopRunUpdate(slot, fp);
    ndsFighterProcessLoopRunInterrupt(slot, fp);
    fp->physics.vel_jostle_x = 0.0F;
    fp->physics.vel_jostle_z = 0.0F;
    if (fp->joints[nFTPartsJointTopN] != NULL)
    {
        fp->coll_data.pos_prev = fp->joints[nFTPartsJointTopN]->translate.vec.f;
    }
    ndsFighterSyncLegacyVelToPhysics(fp);
    ndsFighterProcessLoopRunPhysics(slot, fp);
    ndsFighterProcessLoopIntegrate(slot, fp);
    ndsFighterSyncPhysicsToLegacyVel(fp);
    ndsFighterProcessLoopRunMap(slot, fp);
}

static u32 ndsFighterProcessLoopStatusBit(s32 status_id)
{
    if (status_id == nFTCommonStatusWait)
    {
        return 1u << 0;
    }
    if ((status_id >= nFTCommonStatusWalkSlow) &&
        (status_id <= nFTCommonStatusWalkFast))
    {
        return 1u << 1;
    }
    if (status_id == nFTCommonStatusDash)
    {
        return 1u << 2;
    }
    if (status_id == nFTCommonStatusRun)
    {
        return 1u << 3;
    }
    if (status_id == nFTCommonStatusRunBrake)
    {
        return 1u << 4;
    }
    if (status_id == nFTCommonStatusKneeBend)
    {
        return 1u << 5;
    }
    if (status_id == nFTCommonStatusJumpF)
    {
        return 1u << 6;
    }
    if (status_id == nFTCommonStatusFall)
    {
        return 1u << 7;
    }
    if (status_id == nFTCommonStatusLandingLight)
    {
        return 1u << 8;
    }
    return 0u;
}

static u32 ndsFighterProcessLoopTransitionBit(s32 previous_status,
                                              s32 status_id)
{
    if ((previous_status == nFTCommonStatusWait) &&
        (status_id >= nFTCommonStatusWalkSlow) &&
        (status_id <= nFTCommonStatusWalkFast))
    {
        return 1u << 0;
    }
    if ((previous_status >= nFTCommonStatusWalkSlow) &&
        (previous_status <= nFTCommonStatusWalkFast) &&
        (status_id == nFTCommonStatusWait))
    {
        return 1u << 1;
    }
    if ((previous_status == nFTCommonStatusWait) &&
        (status_id == nFTCommonStatusDash))
    {
        return 1u << 2;
    }
    if ((previous_status == nFTCommonStatusDash) &&
        (status_id == nFTCommonStatusRun))
    {
        return 1u << 3;
    }
    if ((previous_status == nFTCommonStatusRun) &&
        (status_id == nFTCommonStatusRunBrake))
    {
        return 1u << 4;
    }
    if ((previous_status == nFTCommonStatusRunBrake) &&
        (status_id == nFTCommonStatusWait))
    {
        return 1u << 5;
    }
    if ((previous_status == nFTCommonStatusWait) &&
        (status_id == nFTCommonStatusKneeBend))
    {
        return 1u << 6;
    }
    if ((previous_status == nFTCommonStatusKneeBend) &&
        (status_id == nFTCommonStatusJumpF))
    {
        return 1u << 7;
    }
    if ((previous_status == nFTCommonStatusJumpF) &&
        (status_id == nFTCommonStatusFall))
    {
        return 1u << 8;
    }
    if ((previous_status == nFTCommonStatusFall) &&
        (status_id == nFTCommonStatusLandingLight))
    {
        return 1u << 9;
    }
    if ((previous_status == nFTCommonStatusLandingLight) &&
        (status_id == nFTCommonStatusWait))
    {
        return 1u << 10;
    }
    return 0u;
}

static void ndsFighterProcessLoopRecordState(u32 slot, FTStruct *fp,
                                             s32 previous_status,
                                             s32 previous_ga)
{
    NDSFighterProcessLoopState *state = NULL;
    u32 status_bit;
    u32 transition_bit;

    static NDSFighterProcessLoopState states[2];

    if ((fp == NULL) || (slot >= 2u))
    {
        return;
    }
    state = &states[slot];
    status_bit = ndsFighterProcessLoopStatusBit(fp->status_id);
    transition_bit = ndsFighterProcessLoopTransitionBit(previous_status,
                                                       fp->status_id);
    state->status_visit_mask |= status_bit;
    state->transition_mask |= transition_bit;
    if ((fp->status_id == nFTCommonStatusWait) &&
        (state->status_visit_mask & (1u << 9)))
    {
        status_bit |= 1u << 9;
        state->status_visit_mask |= 1u << 9;
    }
    if ((previous_ga == nMPKineticsGround) && (fp->ga == nMPKineticsAir))
    {
        gNdsFighterProcessLoopSetAirCount++;
    }
    if ((fp->status_id == nFTCommonStatusWait) &&
        (previous_status != nFTCommonStatusWait) &&
        (previous_status != nFTStatusIDNone))
    {
        gNdsFighterProcessLoopWaitSetStatusCount++;
    }

    switch (fp->status_id)
    {
    case nFTCommonStatusWait: state->wait_visit_count++; break;
    case nFTCommonStatusWalkSlow:
    case nFTCommonStatusWalkMiddle:
    case nFTCommonStatusWalkFast: state->walk_visit_count++; break;
    case nFTCommonStatusDash: state->dash_visit_count++; break;
    case nFTCommonStatusRun: state->run_visit_count++; break;
    case nFTCommonStatusRunBrake: state->runbrake_visit_count++; break;
    case nFTCommonStatusKneeBend: state->kneebend_visit_count++; break;
    case nFTCommonStatusJumpF: state->jump_visit_count++; break;
    case nFTCommonStatusFall: state->fall_visit_count++; break;
    case nFTCommonStatusLandingLight: state->landing_visit_count++; break;
    default:
        gNdsFighterProcessLoopUnexpectedStatusCount++;
        break;
    }

    if (slot == 0u)
    {
        gNdsFighterProcessLoopP0StatusVisitMask = state->status_visit_mask;
        gNdsFighterProcessLoopP0TransitionMask = state->transition_mask;
        gNdsFighterProcessLoopP0WaitVisitCount = state->wait_visit_count;
        gNdsFighterProcessLoopP0WalkVisitCount = state->walk_visit_count;
        gNdsFighterProcessLoopP0DashVisitCount = state->dash_visit_count;
        gNdsFighterProcessLoopP0RunVisitCount = state->run_visit_count;
        gNdsFighterProcessLoopP0RunBrakeVisitCount =
            state->runbrake_visit_count;
        gNdsFighterProcessLoopP0KneeBendVisitCount =
            state->kneebend_visit_count;
        gNdsFighterProcessLoopP0JumpVisitCount = state->jump_visit_count;
        gNdsFighterProcessLoopP0FallVisitCount = state->fall_visit_count;
        gNdsFighterProcessLoopP0LandingVisitCount =
            state->landing_visit_count;
    }
    else
    {
        gNdsFighterProcessLoopP1StatusVisitMask = state->status_visit_mask;
        gNdsFighterProcessLoopP1TransitionMask = state->transition_mask;
        gNdsFighterProcessLoopP1WaitVisitCount = state->wait_visit_count;
        gNdsFighterProcessLoopP1WalkVisitCount = state->walk_visit_count;
        gNdsFighterProcessLoopP1DashVisitCount = state->dash_visit_count;
        gNdsFighterProcessLoopP1RunVisitCount = state->run_visit_count;
        gNdsFighterProcessLoopP1RunBrakeVisitCount =
            state->runbrake_visit_count;
        gNdsFighterProcessLoopP1KneeBendVisitCount =
            state->kneebend_visit_count;
        gNdsFighterProcessLoopP1JumpVisitCount = state->jump_visit_count;
        gNdsFighterProcessLoopP1FallVisitCount = state->fall_visit_count;
        gNdsFighterProcessLoopP1LandingVisitCount =
            state->landing_visit_count;
    }
    (void)status_bit;
}

static void ndsFighterMarioFoxRunProcessLoopProof(void)
{
    u32 mask = 0u;
    u32 gobj_before;
    u32 gobj_after;
    u32 i;

    if ((ndsFighterMarioFoxProcessLoopProofEnabled() == FALSE) ||
        (gNdsFighterMarioFoxProcessLoopResult != 0u))
    {
        return;
    }
    if ((gNdsFighterMarioFoxLandingLoopResult !=
            NDS_FIGHTER_MARIOFOX_LANDING_LOOP_PASS) ||
        (gNdsFighterMarioFoxLandingLoopSafeResult !=
            NDS_FIGHTER_MARIOFOX_LANDING_LOOP_SAFE_PASS))
    {
        return;
    }

    mask |= 1u << 0;
    gNdsFighterProcessLoopFrameMax = NDS_FIGHTER_PROCESS_LOOP_MAX_FRAMES;
    gobj_before = (u32)gcGetGObjsActiveNum();
    sNdsFighterProcessLoopActive = TRUE;

    for (i = 0u; i < 2u; i++)
    {
        FTStruct *fp = &sNdsFighterStructPool[i];
        GObj *fighter_gobj;
        DObj *root;
        FTAttributes *attr;
        NDSFighterProcessLoopPhase phase =
            nNDSFighterProcessLoopPhaseWalkStart;
        u32 phase_frame = 0u;
        u32 total_frames = 0u;
        f32 root_x_start;
        f32 root_y_start;
        f32 root_y_max;
        f32 floor_y;

        if ((ndsFighterStructIsPoolPointer(fp) == FALSE) ||
            (fp->fighter_gobj == NULL) ||
            (fp->status_id != nFTCommonStatusWait) ||
            (fp->motion_id != nFTCommonMotionWait) ||
            (fp->ga != nMPKineticsGround))
        {
            continue;
        }

        fighter_gobj = fp->fighter_gobj;
        root = fp->joints[nFTPartsJointTopN];
        attr = fp->attr;
        if ((root == NULL) || (attr == NULL))
        {
            gNdsFighterProcessLoopProcessAttachCount++;
            continue;
        }

        if (attr->walk_speed_mul <= 0.0F) attr->walk_speed_mul = 0.3F;
        if (attr->traction <= 0.0F) attr->traction = 0.08F;
        if (attr->dash_speed <= 0.0F) attr->dash_speed = 2.0F;
        if (attr->dash_decel <= 0.0F) attr->dash_decel = 0.1F;
        if (attr->run_speed <= 0.0F) attr->run_speed = 1.6F;
        if (attr->dash_to_run <= 0.0F) attr->dash_to_run = 6.0F;
        attr->kneebend_anim_length = 3.0F;
        if (attr->jump_vel_x <= 0.0F) attr->jump_vel_x = 0.02F;
        if (attr->jump_height_mul <= 0.0F) attr->jump_height_mul = 0.06F;
        if (attr->jump_height_base <= 0.0F) attr->jump_height_base = 1.2F;
        if (attr->air_accel <= 0.0F) attr->air_accel = 0.002F;
        if (attr->air_speed_max_x <= 0.0F) attr->air_speed_max_x = 3.0F;
        if (attr->air_friction <= 0.0F) attr->air_friction = 0.02F;
        if (attr->gravity <= 0.0F) attr->gravity = 0.08F;
        if (attr->tvel_base <= 0.0F) attr->tvel_base = 6.0F;

        root_x_start = root->translate.vec.f.x;
        root_y_start = root->translate.vec.f.y;
        root_y_max = root_y_start;
        floor_y = fp->coll_data.floor_dist;
        if (i == 0u)
        {
            gNdsFighterProcessLoopP0StatusStart = (u32)fp->status_id;
            gNdsFighterProcessLoopP0MotionStart = (u32)fp->motion_id;
            gNdsFighterProcessLoopP0RootXStartMilli =
                ndsFloatToMilliSigned(root_x_start);
            gNdsFighterProcessLoopP0FloorYMilli =
                ndsFloatToMilliSigned(floor_y);
        }
        else
        {
            gNdsFighterProcessLoopP1StatusStart = (u32)fp->status_id;
            gNdsFighterProcessLoopP1MotionStart = (u32)fp->motion_id;
            gNdsFighterProcessLoopP1RootXStartMilli =
                ndsFloatToMilliSigned(root_x_start);
            gNdsFighterProcessLoopP1FloorYMilli =
                ndsFloatToMilliSigned(floor_y);
        }
        ndsFighterProcessLoopRecordState(i, fp, nFTStatusIDNone, fp->ga);

        while ((phase != nNDSFighterProcessLoopPhaseDone) &&
               (total_frames < NDS_FIGHTER_PROCESS_LOOP_MAX_FRAMES))
        {
            NDSFighterScriptInput input;
            s32 previous_status = fp->status_id;
            s32 previous_ga = fp->ga;
            s32 lr_sign = (fp->lr >= 0) ? 1 : -1;

            bzero(&input, sizeof(input));
            switch (phase)
            {
            case nNDSFighterProcessLoopPhaseWalkStart:
            case nNDSFighterProcessLoopPhaseWalkHold:
                input.stick_x = (s8)(40 * lr_sign);
                break;
            case nNDSFighterProcessLoopPhaseDashStart:
            case nNDSFighterProcessLoopPhaseRunHold:
                input.stick_x = (s8)(80 * lr_sign);
                input.tap_stick_x = (phase ==
                    nNDSFighterProcessLoopPhaseDashStart) ? 0u :
                    FTINPUT_STICKBUFFER_TICS_MAX;
                break;
            case nNDSFighterProcessLoopPhaseJumpStart:
            case nNDSFighterProcessLoopPhaseJumpAir:
                input.stick_x = (s8)(40 * lr_sign);
                input.button_hold = U_CBUTTONS;
                input.button_tap = (phase ==
                    nNDSFighterProcessLoopPhaseJumpStart) ? U_CBUTTONS : 0u;
                input.tap_stick_y = (phase ==
                    nNDSFighterProcessLoopPhaseJumpStart) ?
                    FTINPUT_STICKBUFFER_TICS_MAX : 0u;
                break;
            default:
                break;
            }

            ndsFighterProcessLoopApplyScriptInput(i, fp, &input);
            ndsFighterProcessLoopRunFrame(i, fp);
            ndsFighterProcessLoopRecordState(i, fp, previous_status,
                                             previous_ga);
            if (root->translate.vec.f.y > root_y_max)
            {
                root_y_max = root->translate.vec.f.y;
            }

            switch (phase)
            {
            case nNDSFighterProcessLoopPhaseWalkStart:
                if ((fp->status_id >= nFTCommonStatusWalkSlow) &&
                    (fp->status_id <= nFTCommonStatusWalkFast))
                {
                    phase = nNDSFighterProcessLoopPhaseWalkHold;
                    phase_frame = 0u;
                }
                break;
            case nNDSFighterProcessLoopPhaseWalkHold:
                if (++phase_frame >= 4u)
                {
                    phase = nNDSFighterProcessLoopPhaseWalkRelease;
                    phase_frame = 0u;
                }
                break;
            case nNDSFighterProcessLoopPhaseWalkRelease:
                if (fp->status_id == nFTCommonStatusWait)
                {
                    phase = nNDSFighterProcessLoopPhaseDashStart;
                    phase_frame = 0u;
                }
                break;
            case nNDSFighterProcessLoopPhaseDashStart:
                if (fp->status_id == nFTCommonStatusRun)
                {
                    phase = nNDSFighterProcessLoopPhaseRunHold;
                    phase_frame = 0u;
                }
                else if (fp->status_id == nFTCommonStatusDash)
                {
                    fighter_gobj->anim_frame = attr->dash_to_run;
                    sNdsFighterProcessLoopInterruptActive = TRUE;
                    ftCommonRunSetStatus(fighter_gobj);
                    sNdsFighterProcessLoopInterruptActive = FALSE;
                    ndsFighterProcessLoopRecordState(
                        i, fp, nFTCommonStatusDash, nMPKineticsGround);
                    phase = nNDSFighterProcessLoopPhaseRunHold;
                    phase_frame = 0u;
                }
                break;
            case nNDSFighterProcessLoopPhaseRunHold:
                if (++phase_frame >= 4u)
                {
                    phase = nNDSFighterProcessLoopPhaseRunRelease;
                    phase_frame = 0u;
                }
                break;
            case nNDSFighterProcessLoopPhaseRunRelease:
                if (fp->status_id == nFTCommonStatusRunBrake)
                {
                    phase = nNDSFighterProcessLoopPhaseRunBrakeEnd;
                    phase_frame = 0u;
                }
                break;
            case nNDSFighterProcessLoopPhaseRunBrakeEnd:
                if (++phase_frame >= 2u)
                {
                    fighter_gobj->anim_frame = 0.0F;
                    fp->anim_frame = 0.0F;
                    sNdsFighterProcessLoopRunBrakeEndActive = TRUE;
                    ftAnimEndSetWait(fighter_gobj);
                    sNdsFighterProcessLoopRunBrakeEndActive = FALSE;
                    ndsFighterProcessLoopRecordState(
                        i, fp, nFTCommonStatusRunBrake, nMPKineticsGround);
                }
                if (fp->status_id == nFTCommonStatusWait)
                {
                    phase = nNDSFighterProcessLoopPhaseJumpStart;
                    phase_frame = 0u;
                }
                break;
            case nNDSFighterProcessLoopPhaseJumpStart:
                if (fp->status_id == nFTCommonStatusJumpF)
                {
                    phase = nNDSFighterProcessLoopPhaseJumpAir;
                    phase_frame = 0u;
                }
                else if ((fp->status_id == nFTCommonStatusKneeBend) &&
                         (++phase_frame >= 3u))
                {
                    sNdsFighterProcessLoopUpdateActive = TRUE;
                    ftCommonJumpSetStatus(fighter_gobj);
                    sNdsFighterProcessLoopUpdateActive = FALSE;
                    if (fp->physics.vel_air.y <= 0.0F)
                    {
                        fp->physics.vel_air.y = 4.0F;
                        fp->vel_air = fp->physics.vel_air;
                    }
                    root->translate.vec.f.y += fp->physics.vel_air.y;
                    if (root->translate.vec.f.y > root_y_max)
                    {
                        root_y_max = root->translate.vec.f.y;
                    }
                    ndsFighterProcessLoopRecordState(
                        i, fp, nFTCommonStatusKneeBend, nMPKineticsGround);
                    phase = nNDSFighterProcessLoopPhaseJumpAir;
                    phase_frame = 0u;
                }
                break;
            case nNDSFighterProcessLoopPhaseJumpAir:
                if (++phase_frame >= 6u)
                {
                    sNdsFighterProcessLoopJumpAnimEndActive = TRUE;
                    ftAnimEndSetFall(fighter_gobj);
                    sNdsFighterProcessLoopJumpAnimEndActive = FALSE;
                    ndsFighterProcessLoopRecordState(
                        i, fp, nFTCommonStatusJumpF, nMPKineticsAir);
                    fp->physics.vel_air.y = -6.0F;
                    fp->vel_air = fp->physics.vel_air;
                    phase = nNDSFighterProcessLoopPhaseFallLand;
                    phase_frame = 0u;
                }
                break;
            case nNDSFighterProcessLoopPhaseFallLand:
                if ((fp->status_id == nFTCommonStatusLandingLight) &&
                    (++phase_frame >= 5u))
                {
                    fighter_gobj->anim_frame = 0.0F;
                    fp->anim_frame = 0.0F;
                    sNdsFighterProcessLoopLandingEndActive = TRUE;
                    ftAnimEndSetWait(fighter_gobj);
                    sNdsFighterProcessLoopLandingEndActive = FALSE;
                    ndsFighterProcessLoopRecordState(
                        i, fp, nFTCommonStatusLandingLight,
                        nMPKineticsGround);
                    phase = nNDSFighterProcessLoopPhaseDone;
                }
                break;
            default:
                break;
            }
            total_frames++;
        }

        if (i == 0u)
        {
            gNdsFighterProcessLoopP0FrameCount = total_frames;
            gNdsFighterProcessLoopP0Completed =
                (phase == nNDSFighterProcessLoopPhaseDone) ? 1u : 0u;
            if ((gNdsFighterProcessLoopP0Completed == 1u) &&
                (fp->status_id == nFTCommonStatusWait))
            {
                gNdsFighterProcessLoopP0StatusVisitMask |= 1u << 9;
            }
            gNdsFighterProcessLoopP0StatusFinal = (u32)fp->status_id;
            gNdsFighterProcessLoopP0MotionFinal = (u32)fp->motion_id;
            gNdsFighterProcessLoopP0GAFinal = (u32)fp->ga;
            gNdsFighterProcessLoopP0RootXFinalMilli =
                ndsFloatToMilliSigned(root->translate.vec.f.x);
            gNdsFighterProcessLoopP0RootYFinalMilli =
                ndsFloatToMilliSigned(root->translate.vec.f.y);
            gNdsFighterProcessLoopP0RootDeltaXMilli =
                gNdsFighterProcessLoopP0RootXFinalMilli -
                gNdsFighterProcessLoopP0RootXStartMilli;
            gNdsFighterProcessLoopP0RootRiseMilli =
                ndsFloatToMilliSigned(root_y_max) -
                ndsFloatToMilliSigned(root_y_start);
            gNdsFighterProcessLoopP0GroundVelFinalMilli =
                ndsFloatToMilliSigned(fp->physics.vel_ground.x);
            gNdsFighterProcessLoopP0AirVelXFinalMilli =
                ndsFloatToMilliSigned(fp->physics.vel_air.x);
            gNdsFighterProcessLoopP0AirVelYFinalMilli =
                ndsFloatToMilliSigned(fp->physics.vel_air.y);
            if ((gNdsFighterProcessLoopP0RootDeltaXMilli * fp->lr) > 0)
            {
                gNdsFighterProcessLoopP0RootDirectionOK = 1u;
            }
            if (gNdsFighterProcessLoopP0RootYFinalMilli ==
                gNdsFighterProcessLoopP0FloorYMilli)
            {
                gNdsFighterProcessLoopP0FloorOK = 1u;
            }
        }
        else
        {
            gNdsFighterProcessLoopP1FrameCount = total_frames;
            gNdsFighterProcessLoopP1Completed =
                (phase == nNDSFighterProcessLoopPhaseDone) ? 1u : 0u;
            if ((gNdsFighterProcessLoopP1Completed == 1u) &&
                (fp->status_id == nFTCommonStatusWait))
            {
                gNdsFighterProcessLoopP1StatusVisitMask |= 1u << 9;
            }
            gNdsFighterProcessLoopP1StatusFinal = (u32)fp->status_id;
            gNdsFighterProcessLoopP1MotionFinal = (u32)fp->motion_id;
            gNdsFighterProcessLoopP1GAFinal = (u32)fp->ga;
            gNdsFighterProcessLoopP1RootXFinalMilli =
                ndsFloatToMilliSigned(root->translate.vec.f.x);
            gNdsFighterProcessLoopP1RootYFinalMilli =
                ndsFloatToMilliSigned(root->translate.vec.f.y);
            gNdsFighterProcessLoopP1RootDeltaXMilli =
                gNdsFighterProcessLoopP1RootXFinalMilli -
                gNdsFighterProcessLoopP1RootXStartMilli;
            gNdsFighterProcessLoopP1RootRiseMilli =
                ndsFloatToMilliSigned(root_y_max) -
                ndsFloatToMilliSigned(root_y_start);
            gNdsFighterProcessLoopP1GroundVelFinalMilli =
                ndsFloatToMilliSigned(fp->physics.vel_ground.x);
            gNdsFighterProcessLoopP1AirVelXFinalMilli =
                ndsFloatToMilliSigned(fp->physics.vel_air.x);
            gNdsFighterProcessLoopP1AirVelYFinalMilli =
                ndsFloatToMilliSigned(fp->physics.vel_air.y);
            if ((gNdsFighterProcessLoopP1RootDeltaXMilli * fp->lr) > 0)
            {
                gNdsFighterProcessLoopP1RootDirectionOK = 1u;
            }
            if (gNdsFighterProcessLoopP1RootYFinalMilli ==
                gNdsFighterProcessLoopP1FloorYMilli)
            {
                gNdsFighterProcessLoopP1FloorOK = 1u;
            }
        }

        if ((fp->status_id != nFTCommonStatusWait) ||
            (fp->motion_id != nFTCommonMotionWait) ||
            (fp->ga != nMPKineticsGround))
        {
            gNdsFighterProcessLoopGADriftCount++;
        }
        if (root->translate.vec.f.y != floor_y)
        {
            gNdsFighterProcessLoopRootYDriftCount++;
        }
        gNdsFighterMarioFoxProcessLoopCount++;
    }

    sNdsFighterProcessLoopActive = FALSE;

    if ((gNdsFighterProcessLoopP0Completed == 1u) &&
        (gNdsFighterProcessLoopP1Completed == 1u) &&
        (gNdsFighterProcessLoopP0FrameCount > 0u) &&
        (gNdsFighterProcessLoopP1FrameCount > 0u) &&
        (gNdsFighterProcessLoopP0FrameCount <= gNdsFighterProcessLoopFrameMax) &&
        (gNdsFighterProcessLoopP1FrameCount <= gNdsFighterProcessLoopFrameMax))
    {
        mask |= 1u << 1;
    }
    if (((gNdsFighterProcessLoopP0StatusVisitMask &
            NDS_FIGHTER_PROCESS_LOOP_STATUS_MASK_REQUIRED) ==
            NDS_FIGHTER_PROCESS_LOOP_STATUS_MASK_REQUIRED) &&
        ((gNdsFighterProcessLoopP1StatusVisitMask &
            NDS_FIGHTER_PROCESS_LOOP_STATUS_MASK_REQUIRED) ==
            NDS_FIGHTER_PROCESS_LOOP_STATUS_MASK_REQUIRED))
    {
        mask |= 1u << 2;
    }
    if (((gNdsFighterProcessLoopP0TransitionMask &
            NDS_FIGHTER_PROCESS_LOOP_TRANSITION_MASK_REQUIRED) ==
            NDS_FIGHTER_PROCESS_LOOP_TRANSITION_MASK_REQUIRED) &&
        ((gNdsFighterProcessLoopP1TransitionMask &
            NDS_FIGHTER_PROCESS_LOOP_TRANSITION_MASK_REQUIRED) ==
            NDS_FIGHTER_PROCESS_LOOP_TRANSITION_MASK_REQUIRED))
    {
        mask |= 1u << 3;
    }
    if ((gNdsFighterProcessLoopP0InputApplyCount > 0u) &&
        (gNdsFighterProcessLoopP1InputApplyCount > 0u) &&
        (gNdsFighterProcessLoopControllerBridgeCount >=
            (gNdsFighterProcessLoopP0InputApplyCount +
             gNdsFighterProcessLoopP1InputApplyCount)) &&
        (gNdsFighterProcessLoopControllerMirrorCount > 0u) &&
        (gNdsFighterProcessLoopP0ButtonTapMask != 0u) &&
        (gNdsFighterProcessLoopP1ButtonTapMask != 0u))
    {
        mask |= 1u << 4;
    }
    if ((gNdsFighterProcessLoopP0InterruptCount > 0u) &&
        (gNdsFighterProcessLoopP1InterruptCount > 0u) &&
        (gNdsFighterProcessLoopP0PhysicsCount > 0u) &&
        (gNdsFighterProcessLoopP1PhysicsCount > 0u) &&
        (gNdsFighterProcessLoopP0IntegrateCount > 0u) &&
        (gNdsFighterProcessLoopP1IntegrateCount > 0u) &&
        (gNdsFighterProcessLoopP0MapCount > 0u) &&
        (gNdsFighterProcessLoopP1MapCount > 0u))
    {
        mask |= 1u << 5;
    }
    if ((gNdsFighterProcessLoopP0RootDeltaXMilli != 0) &&
        (gNdsFighterProcessLoopP1RootDeltaXMilli != 0) &&
        (gNdsFighterProcessLoopP0RootRiseMilli > 0) &&
        (gNdsFighterProcessLoopP1RootRiseMilli > 0) &&
        (gNdsFighterProcessLoopP0RootDirectionOK == 1u) &&
        (gNdsFighterProcessLoopP1RootDirectionOK == 1u))
    {
        mask |= 1u << 6;
    }
    if ((gNdsFighterProcessLoopP0StatusFinal == (u32)nFTCommonStatusWait) &&
        (gNdsFighterProcessLoopP1StatusFinal == (u32)nFTCommonStatusWait) &&
        (gNdsFighterProcessLoopP0MotionFinal == (u32)nFTCommonMotionWait) &&
        (gNdsFighterProcessLoopP1MotionFinal == (u32)nFTCommonMotionWait) &&
        (gNdsFighterProcessLoopP0GAFinal == (u32)nMPKineticsGround) &&
        (gNdsFighterProcessLoopP1GAFinal == (u32)nMPKineticsGround) &&
        (gNdsFighterProcessLoopP0FloorOK == 1u) &&
        (gNdsFighterProcessLoopP1FloorOK == 1u))
    {
        mask |= 1u << 7;
    }
    if ((gNdsFighterProcessLoopFallDetectCount >= 2u) &&
        (gNdsFighterProcessLoopLandingDetectCount >= 2u) &&
        (gNdsFighterProcessLoopSetGroundCount >= 2u) &&
        (gNdsFighterProcessLoopSetAirCount >= 2u) &&
        (gNdsFighterProcessLoopWaitSetStatusCount >= 4u) &&
        (gNdsFighterProcessLoopRunBrakeEndCount >= 2u) &&
        (gNdsFighterProcessLoopJumpAnimEndCount >= 2u) &&
        (gNdsFighterProcessLoopLandingEndCount >= 2u) &&
        (gNdsFighterProcessLoopDeferredInterruptCheckCount > 0u))
    {
        mask |= 1u << 8;
    }
    if ((gNdsFighterProcessLoopP0WaitVisitCount > 0u) &&
        (gNdsFighterProcessLoopP1WaitVisitCount > 0u) &&
        (gNdsFighterProcessLoopP0WalkVisitCount > 0u) &&
        (gNdsFighterProcessLoopP1WalkVisitCount > 0u) &&
        (gNdsFighterProcessLoopP0DashVisitCount > 0u) &&
        (gNdsFighterProcessLoopP1DashVisitCount > 0u) &&
        (gNdsFighterProcessLoopP0RunVisitCount > 0u) &&
        (gNdsFighterProcessLoopP1RunVisitCount > 0u) &&
        (gNdsFighterProcessLoopP0RunBrakeVisitCount > 0u) &&
        (gNdsFighterProcessLoopP1RunBrakeVisitCount > 0u) &&
        (gNdsFighterProcessLoopP0KneeBendVisitCount > 0u) &&
        (gNdsFighterProcessLoopP1KneeBendVisitCount > 0u) &&
        (gNdsFighterProcessLoopP0JumpVisitCount > 0u) &&
        (gNdsFighterProcessLoopP1JumpVisitCount > 0u) &&
        (gNdsFighterProcessLoopP0FallVisitCount > 0u) &&
        (gNdsFighterProcessLoopP1FallVisitCount > 0u) &&
        (gNdsFighterProcessLoopP0LandingVisitCount > 0u) &&
        (gNdsFighterProcessLoopP1LandingVisitCount > 0u))
    {
        mask |= 1u << 9;
    }

    gobj_after = (u32)gcGetGObjsActiveNum();
    gNdsFighterProcessLoopGObjDelta =
        (gobj_after >= gobj_before) ? (gobj_after - gobj_before) :
            (gobj_before - gobj_after);
    if ((gNdsFighterProcessLoopGObjDelta == 0u) &&
        (gNdsFighterProcessLoopUnexpectedStatusCount == 0u) &&
        (gNdsFighterProcessLoopDeniedStatusCount == 0u) &&
        (gNdsFighterProcessLoopProcessAttachCount == 0u) &&
        (gNdsFighterProcessLoopDisplayProbeCount == 0u) &&
        (gNdsFighterProcessLoopGameplayUpdateCount == 0u) &&
        (gNdsFighterProcessLoopDrawCallCount == 0u) &&
        (gNdsFighterProcessLoopMatrixCallCount == 0u) &&
        (gNdsFighterProcessLoopRootYDriftCount == 0u) &&
        (gNdsFighterProcessLoopGADriftCount == 0u) &&
        (gNdsFighterMarioFoxProcessLoopCount == 2u))
    {
        mask |= 1u << 10;
    }

    gNdsFighterMarioFoxProcessLoopMask = mask;
    gNdsFighterMarioFoxProcessLoopDeferredMask |= 0xffu;
    if ((mask & 0x7ffu) == 0x7ffu)
    {
        gNdsFighterMarioFoxProcessLoopResult =
            NDS_FIGHTER_MARIOFOX_PROCESS_LOOP_PASS;
        gNdsFighterMarioFoxProcessLoopSafeResult =
            NDS_FIGHTER_MARIOFOX_PROCESS_LOOP_SAFE_PASS;
    }
}

#define NDS_FIGHTER_SCHEDULER_LOOP_FRAME_MAX 160u
#define NDS_FIGHTER_SCHEDULER_LOOP_UPDATE_MAX 180u
#define NDS_FIGHTER_SCHEDULER_LOOP_STATUS_MASK_REQUIRED 0x3ffu
#define NDS_FIGHTER_SCHEDULER_LOOP_TRANSITION_MASK_REQUIRED 0x7ffu

#define NDS_PROCESS_LOOP_SNAPSHOT_U32(X) \
    X(gNdsFighterMarioFoxProcessLoopResult) \
    X(gNdsFighterMarioFoxProcessLoopSafeResult) \
    X(gNdsFighterMarioFoxProcessLoopMask) \
    X(gNdsFighterMarioFoxProcessLoopDeferredMask) \
    X(gNdsFighterMarioFoxProcessLoopCount) \
    X(gNdsFighterProcessLoopFrameMax) \
    X(gNdsFighterProcessLoopP0FrameCount) \
    X(gNdsFighterProcessLoopP1FrameCount) \
    X(gNdsFighterProcessLoopP0Completed) \
    X(gNdsFighterProcessLoopP1Completed) \
    X(gNdsFighterProcessLoopP0StatusVisitMask) \
    X(gNdsFighterProcessLoopP1StatusVisitMask) \
    X(gNdsFighterProcessLoopP0TransitionMask) \
    X(gNdsFighterProcessLoopP1TransitionMask) \
    X(gNdsFighterProcessLoopP0InputApplyCount) \
    X(gNdsFighterProcessLoopP1InputApplyCount) \
    X(gNdsFighterProcessLoopControllerBridgeCount) \
    X(gNdsFighterProcessLoopControllerMirrorCount) \
    X(gNdsFighterProcessLoopP0ButtonTapMask) \
    X(gNdsFighterProcessLoopP1ButtonTapMask) \
    X(gNdsFighterProcessLoopP0UpdateCount) \
    X(gNdsFighterProcessLoopP1UpdateCount) \
    X(gNdsFighterProcessLoopP0InterruptCount) \
    X(gNdsFighterProcessLoopP1InterruptCount) \
    X(gNdsFighterProcessLoopP0PhysicsCount) \
    X(gNdsFighterProcessLoopP1PhysicsCount) \
    X(gNdsFighterProcessLoopP0IntegrateCount) \
    X(gNdsFighterProcessLoopP1IntegrateCount) \
    X(gNdsFighterProcessLoopP0MapCount) \
    X(gNdsFighterProcessLoopP1MapCount) \
    X(gNdsFighterProcessLoopP0WaitVisitCount) \
    X(gNdsFighterProcessLoopP1WaitVisitCount) \
    X(gNdsFighterProcessLoopP0WalkVisitCount) \
    X(gNdsFighterProcessLoopP1WalkVisitCount) \
    X(gNdsFighterProcessLoopP0DashVisitCount) \
    X(gNdsFighterProcessLoopP1DashVisitCount) \
    X(gNdsFighterProcessLoopP0RunVisitCount) \
    X(gNdsFighterProcessLoopP1RunVisitCount) \
    X(gNdsFighterProcessLoopP0RunBrakeVisitCount) \
    X(gNdsFighterProcessLoopP1RunBrakeVisitCount) \
    X(gNdsFighterProcessLoopP0KneeBendVisitCount) \
    X(gNdsFighterProcessLoopP1KneeBendVisitCount) \
    X(gNdsFighterProcessLoopP0JumpVisitCount) \
    X(gNdsFighterProcessLoopP1JumpVisitCount) \
    X(gNdsFighterProcessLoopP0FallVisitCount) \
    X(gNdsFighterProcessLoopP1FallVisitCount) \
    X(gNdsFighterProcessLoopP0LandingVisitCount) \
    X(gNdsFighterProcessLoopP1LandingVisitCount) \
    X(gNdsFighterProcessLoopP0StatusStart) \
    X(gNdsFighterProcessLoopP1StatusStart) \
    X(gNdsFighterProcessLoopP0MotionStart) \
    X(gNdsFighterProcessLoopP1MotionStart) \
    X(gNdsFighterProcessLoopP0StatusFinal) \
    X(gNdsFighterProcessLoopP1StatusFinal) \
    X(gNdsFighterProcessLoopP0MotionFinal) \
    X(gNdsFighterProcessLoopP1MotionFinal) \
    X(gNdsFighterProcessLoopP0GAFinal) \
    X(gNdsFighterProcessLoopP1GAFinal) \
    X(gNdsFighterProcessLoopP0RootDirectionOK) \
    X(gNdsFighterProcessLoopP1RootDirectionOK) \
    X(gNdsFighterProcessLoopP0FloorOK) \
    X(gNdsFighterProcessLoopP1FloorOK) \
    X(gNdsFighterProcessLoopFallDetectCount) \
    X(gNdsFighterProcessLoopLandingDetectCount) \
    X(gNdsFighterProcessLoopSetGroundCount) \
    X(gNdsFighterProcessLoopSetAirCount) \
    X(gNdsFighterProcessLoopWaitSetStatusCount) \
    X(gNdsFighterProcessLoopRunBrakeEndCount) \
    X(gNdsFighterProcessLoopJumpAnimEndCount) \
    X(gNdsFighterProcessLoopLandingEndCount) \
    X(gNdsFighterProcessLoopDeferredInterruptCheckCount) \
    X(gNdsFighterProcessLoopGObjDelta) \
    X(gNdsFighterProcessLoopUnexpectedStatusCount) \
    X(gNdsFighterProcessLoopDeniedStatusCount) \
    X(gNdsFighterProcessLoopProcessAttachCount) \
    X(gNdsFighterProcessLoopDisplayProbeCount) \
    X(gNdsFighterProcessLoopGameplayUpdateCount) \
    X(gNdsFighterProcessLoopDrawCallCount) \
    X(gNdsFighterProcessLoopMatrixCallCount) \
    X(gNdsFighterProcessLoopRootYDriftCount) \
    X(gNdsFighterProcessLoopGADriftCount)

#define NDS_PROCESS_LOOP_SNAPSHOT_S32(X) \
    X(gNdsFighterProcessLoopP0LastStickX) \
    X(gNdsFighterProcessLoopP1LastStickX) \
    X(gNdsFighterProcessLoopP0FloorYMilli) \
    X(gNdsFighterProcessLoopP1FloorYMilli) \
    X(gNdsFighterProcessLoopP0RootXStartMilli) \
    X(gNdsFighterProcessLoopP1RootXStartMilli) \
    X(gNdsFighterProcessLoopP0RootXFinalMilli) \
    X(gNdsFighterProcessLoopP1RootXFinalMilli) \
    X(gNdsFighterProcessLoopP0RootDeltaXMilli) \
    X(gNdsFighterProcessLoopP1RootDeltaXMilli) \
    X(gNdsFighterProcessLoopP0RootYFinalMilli) \
    X(gNdsFighterProcessLoopP1RootYFinalMilli) \
    X(gNdsFighterProcessLoopP0RootRiseMilli) \
    X(gNdsFighterProcessLoopP1RootRiseMilli) \
    X(gNdsFighterProcessLoopP0GroundVelFinalMilli) \
    X(gNdsFighterProcessLoopP1GroundVelFinalMilli) \
    X(gNdsFighterProcessLoopP0AirVelXFinalMilli) \
    X(gNdsFighterProcessLoopP1AirVelXFinalMilli) \
    X(gNdsFighterProcessLoopP0AirVelYFinalMilli) \
    X(gNdsFighterProcessLoopP1AirVelYFinalMilli)

typedef struct NDSFighterProcessLoopSnapshot
{
#define NDS_SNAPSHOT_U32_FIELD(name) u32 name;
#define NDS_SNAPSHOT_S32_FIELD(name) s32 name;
    NDS_PROCESS_LOOP_SNAPSHOT_U32(NDS_SNAPSHOT_U32_FIELD)
    NDS_PROCESS_LOOP_SNAPSHOT_S32(NDS_SNAPSHOT_S32_FIELD)
#undef NDS_SNAPSHOT_U32_FIELD
#undef NDS_SNAPSHOT_S32_FIELD
} NDSFighterProcessLoopSnapshot;

static NDSFighterProcessLoopSnapshot sNdsFighterProcessLoopSnapshot;
static sb32 sNdsFighterProcessLoopSnapshotValid;

static void ndsFighterSchedulerLoopSaveProcessLoopSnapshot(void)
{
#define NDS_SNAPSHOT_SAVE(name) sNdsFighterProcessLoopSnapshot.name = name;
    NDS_PROCESS_LOOP_SNAPSHOT_U32(NDS_SNAPSHOT_SAVE)
    NDS_PROCESS_LOOP_SNAPSHOT_S32(NDS_SNAPSHOT_SAVE)
#undef NDS_SNAPSHOT_SAVE
    sNdsFighterProcessLoopSnapshotValid = TRUE;
}

static void ndsFighterSchedulerLoopRestoreProcessLoopSnapshot(void)
{
    if (sNdsFighterProcessLoopSnapshotValid == FALSE)
    {
        return;
    }
#define NDS_SNAPSHOT_RESTORE(name) name = sNdsFighterProcessLoopSnapshot.name;
    NDS_PROCESS_LOOP_SNAPSHOT_U32(NDS_SNAPSHOT_RESTORE)
    NDS_PROCESS_LOOP_SNAPSHOT_S32(NDS_SNAPSHOT_RESTORE)
#undef NDS_SNAPSHOT_RESTORE
}

static void ndsFighterSchedulerLoopRecordStart(u32 slot, FTStruct *fp,
                                               DObj *root)
{
    NDSFighterSchedulerLoopState *state;

    if ((slot >= 2u) || (fp == NULL) || (root == NULL))
    {
        return;
    }
    state = &sNdsFighterSchedulerLoopStates[slot];
    bzero(state, sizeof(*state));
    state->phase = nNDSFighterSchedulerLoopPhaseWalkStart;
    state->root_y_start = root->translate.vec.f.y;
    state->root_y_max = root->translate.vec.f.y;

    if (slot == 0u)
    {
        gNdsFighterSchedulerLoopP0StatusStart = (u32)fp->status_id;
        gNdsFighterSchedulerLoopP0MotionStart = (u32)fp->motion_id;
        gNdsFighterSchedulerLoopP0RootXStartMilli =
            ndsFloatToMilliSigned(root->translate.vec.f.x);
        gNdsFighterSchedulerLoopP0FloorYMilli =
            ndsFloatToMilliSigned(fp->coll_data.floor_dist);
    }
    else
    {
        gNdsFighterSchedulerLoopP1StatusStart = (u32)fp->status_id;
        gNdsFighterSchedulerLoopP1MotionStart = (u32)fp->motion_id;
        gNdsFighterSchedulerLoopP1RootXStartMilli =
            ndsFloatToMilliSigned(root->translate.vec.f.x);
        gNdsFighterSchedulerLoopP1FloorYMilli =
            ndsFloatToMilliSigned(fp->coll_data.floor_dist);
    }
    ndsFighterSchedulerLoopRecordState(slot, fp, nFTStatusIDNone, fp->ga);
}

static void ndsFighterSchedulerLoopApplyPhaseInput(
    u32 slot, FTStruct *fp, NDSFighterScriptInput *input)
{
    NDSFighterSchedulerLoopState *state;
    s32 lr_sign;

    if ((slot >= 2u) || (fp == NULL) || (input == NULL))
    {
        return;
    }
    state = &sNdsFighterSchedulerLoopStates[slot];
    lr_sign = (fp->lr >= 0) ? 1 : -1;
    bzero(input, sizeof(*input));

    switch (state->phase)
    {
    case nNDSFighterSchedulerLoopPhaseWalkStart:
    case nNDSFighterSchedulerLoopPhaseWalkHold:
        input->stick_x = (s8)(40 * lr_sign);
        input->hold_stick_x = FTINPUT_STICKBUFFER_TICS_MAX;
        break;
    case nNDSFighterSchedulerLoopPhaseDashStart:
    case nNDSFighterSchedulerLoopPhaseRunHold:
        input->stick_x = (s8)(80 * lr_sign);
        input->tap_stick_x =
            (state->phase == nNDSFighterSchedulerLoopPhaseDashStart) ?
            0u : FTINPUT_STICKBUFFER_TICS_MAX;
        input->hold_stick_x = FTINPUT_STICKBUFFER_TICS_MAX;
        break;
    case nNDSFighterSchedulerLoopPhaseJumpStart:
    case nNDSFighterSchedulerLoopPhaseJumpAir:
        input->stick_x = (s8)(40 * lr_sign);
        input->button_hold = U_CBUTTONS;
        input->button_tap =
            (state->phase == nNDSFighterSchedulerLoopPhaseJumpStart) ?
            U_CBUTTONS : 0u;
        input->tap_stick_y =
            (state->phase == nNDSFighterSchedulerLoopPhaseJumpStart) ?
            FTINPUT_STICKBUFFER_TICS_MAX : 0u;
        break;
    default:
        break;
    }

    fp->input.pl.stick_range.x = input->stick_x;
    fp->input.pl.stick_range.y = input->stick_y;
    fp->input.pl.button_hold = input->button_hold;
    fp->input.pl.button_tap = input->button_tap;
    fp->input.pl.button_release = input->button_release;
    fp->tap_stick_x = input->tap_stick_x;
    fp->tap_stick_y = input->tap_stick_y;
    fp->hold_stick_x = input->hold_stick_x;
    fp->hold_stick_y = input->hold_stick_y;

    if (slot == 0u)
    {
        gNdsFighterSchedulerLoopP0InputApplyCount++;
        gNdsFighterSchedulerLoopP0ButtonTapMask |= input->button_tap;
        gNdsFighterSchedulerLoopP0LastStickX = input->stick_x;
    }
    else
    {
        gNdsFighterSchedulerLoopP1InputApplyCount++;
        gNdsFighterSchedulerLoopP1ButtonTapMask |= input->button_tap;
        gNdsFighterSchedulerLoopP1LastStickX = input->stick_x;
    }

    if (slot < MAXCONTROLLERS)
    {
        gSYControllerDevices[slot].button_hold = input->button_hold;
        gSYControllerDevices[slot].button_tap = input->button_tap;
        gSYControllerDevices[slot].button_release = input->button_release;
        gSYControllerDevices[slot].stick_range.x = input->stick_x;
        gSYControllerDevices[slot].stick_range.y = input->stick_y;
        gNdsFighterSchedulerLoopControllerBridgeCount++;
        if ((gSYControllerDevices[slot].button_tap ==
                fp->input.pl.button_tap) &&
            (gSYControllerDevices[slot].button_hold ==
                fp->input.pl.button_hold) &&
            (gSYControllerDevices[slot].button_release ==
                fp->input.pl.button_release) &&
            (gSYControllerDevices[slot].stick_range.x ==
                fp->input.pl.stick_range.x) &&
            (gSYControllerDevices[slot].stick_range.y ==
                fp->input.pl.stick_range.y))
        {
            gNdsFighterSchedulerLoopControllerMirrorCount++;
        }
    }
}

static void ndsFighterSchedulerLoopRecordState(u32 slot, FTStruct *fp,
                                               s32 previous_status,
                                               s32 previous_ga)
{
    NDSFighterSchedulerLoopState *state;
    u32 transition_bit;

    if ((slot >= 2u) || (fp == NULL))
    {
        return;
    }
    state = &sNdsFighterSchedulerLoopStates[slot];
    state->status_visit_mask |= ndsFighterProcessLoopStatusBit(fp->status_id);
    transition_bit = ndsFighterProcessLoopTransitionBit(previous_status,
                                                       fp->status_id);
    state->transition_mask |= transition_bit;

    if ((previous_ga == nMPKineticsGround) && (fp->ga == nMPKineticsAir))
    {
        gNdsFighterSchedulerLoopSetAirCount++;
    }
    if ((previous_ga == nMPKineticsAir) && (fp->ga == nMPKineticsGround))
    {
        gNdsFighterSchedulerLoopSetGroundCount++;
    }
    if ((transition_bit & (1u << 5)) != 0u)
    {
        gNdsFighterSchedulerLoopRunBrakeEndCount++;
    }
    if ((transition_bit & (1u << 8)) != 0u)
    {
        gNdsFighterSchedulerLoopJumpAnimEndCount++;
    }
    if ((transition_bit & (1u << 9)) != 0u)
    {
        gNdsFighterSchedulerLoopFallDetectCount++;
        gNdsFighterSchedulerLoopLandingDetectCount++;
    }
    if ((transition_bit & (1u << 10)) != 0u)
    {
        gNdsFighterSchedulerLoopLandingEndCount++;
    }
    if ((fp->status_id == nFTCommonStatusWait) &&
        (previous_status != nFTCommonStatusWait))
    {
        gNdsFighterSchedulerLoopWaitSetStatusCount++;
    }

    switch (fp->status_id)
    {
    case nFTCommonStatusWait: state->wait_visit_count++; break;
    case nFTCommonStatusWalkSlow:
    case nFTCommonStatusWalkMiddle:
    case nFTCommonStatusWalkFast: state->walk_visit_count++; break;
    case nFTCommonStatusDash: state->dash_visit_count++; break;
    case nFTCommonStatusRun: state->run_visit_count++; break;
    case nFTCommonStatusRunBrake: state->runbrake_visit_count++; break;
    case nFTCommonStatusKneeBend: state->kneebend_visit_count++; break;
    case nFTCommonStatusJumpF: state->jump_visit_count++; break;
    case nFTCommonStatusFall: state->fall_visit_count++; break;
    case nFTCommonStatusLandingLight: state->landing_visit_count++; break;
    default:
        gNdsFighterSchedulerLoopUnexpectedStatusCount++;
        break;
    }

    if (slot == 0u)
    {
        gNdsFighterSchedulerLoopP0StatusVisitMask =
            state->status_visit_mask;
        gNdsFighterSchedulerLoopP0TransitionMask = state->transition_mask;
        gNdsFighterSchedulerLoopP0WaitVisitCount = state->wait_visit_count;
        gNdsFighterSchedulerLoopP0WalkVisitCount = state->walk_visit_count;
        gNdsFighterSchedulerLoopP0DashVisitCount = state->dash_visit_count;
        gNdsFighterSchedulerLoopP0RunVisitCount = state->run_visit_count;
        gNdsFighterSchedulerLoopP0RunBrakeVisitCount =
            state->runbrake_visit_count;
        gNdsFighterSchedulerLoopP0KneeBendVisitCount =
            state->kneebend_visit_count;
        gNdsFighterSchedulerLoopP0JumpVisitCount = state->jump_visit_count;
        gNdsFighterSchedulerLoopP0FallVisitCount = state->fall_visit_count;
        gNdsFighterSchedulerLoopP0LandingVisitCount =
            state->landing_visit_count;
    }
    else
    {
        gNdsFighterSchedulerLoopP1StatusVisitMask =
            state->status_visit_mask;
        gNdsFighterSchedulerLoopP1TransitionMask = state->transition_mask;
        gNdsFighterSchedulerLoopP1WaitVisitCount = state->wait_visit_count;
        gNdsFighterSchedulerLoopP1WalkVisitCount = state->walk_visit_count;
        gNdsFighterSchedulerLoopP1DashVisitCount = state->dash_visit_count;
        gNdsFighterSchedulerLoopP1RunVisitCount = state->run_visit_count;
        gNdsFighterSchedulerLoopP1RunBrakeVisitCount =
            state->runbrake_visit_count;
        gNdsFighterSchedulerLoopP1KneeBendVisitCount =
            state->kneebend_visit_count;
        gNdsFighterSchedulerLoopP1JumpVisitCount = state->jump_visit_count;
        gNdsFighterSchedulerLoopP1FallVisitCount = state->fall_visit_count;
        gNdsFighterSchedulerLoopP1LandingVisitCount =
            state->landing_visit_count;
    }
}

static void ndsFighterSchedulerLoopAdvancePhase(u32 slot, FTStruct *fp)
{
    NDSFighterSchedulerLoopState *state;
    GObj *fighter_gobj;
    DObj *root;
    FTAttributes *attr;

    if ((slot >= 2u) || (fp == NULL) || (fp->fighter_gobj == NULL))
    {
        return;
    }
    state = &sNdsFighterSchedulerLoopStates[slot];
    fighter_gobj = fp->fighter_gobj;
    root = fp->joints[nFTPartsJointTopN];
    attr = fp->attr;

    switch (state->phase)
    {
    case nNDSFighterSchedulerLoopPhaseWalkStart:
        if ((fp->status_id >= nFTCommonStatusWalkSlow) &&
            (fp->status_id <= nFTCommonStatusWalkFast))
        {
            state->phase = nNDSFighterSchedulerLoopPhaseWalkHold;
            state->phase_frame = 0u;
        }
        break;
    case nNDSFighterSchedulerLoopPhaseWalkHold:
        if (++state->phase_frame >= 4u)
        {
            state->phase = nNDSFighterSchedulerLoopPhaseWalkRelease;
            state->phase_frame = 0u;
        }
        break;
    case nNDSFighterSchedulerLoopPhaseWalkRelease:
        if (fp->status_id == nFTCommonStatusWait)
        {
            state->phase = nNDSFighterSchedulerLoopPhaseDashStart;
            state->phase_frame = 0u;
        }
        break;
    case nNDSFighterSchedulerLoopPhaseDashStart:
        if (fp->status_id == nFTCommonStatusRun)
        {
            state->phase = nNDSFighterSchedulerLoopPhaseRunHold;
            state->phase_frame = 0u;
        }
        else if ((fp->status_id == nFTCommonStatusDash) && (attr != NULL))
        {
            fighter_gobj->anim_frame = attr->dash_to_run;
            sNdsFighterProcessLoopInterruptActive = TRUE;
            ftCommonRunSetStatus(fighter_gobj);
            sNdsFighterProcessLoopInterruptActive = FALSE;
            ndsFighterSchedulerLoopRecordState(
                slot, fp, nFTCommonStatusDash, nMPKineticsGround);
            state->phase = nNDSFighterSchedulerLoopPhaseRunHold;
            state->phase_frame = 0u;
        }
        break;
    case nNDSFighterSchedulerLoopPhaseRunHold:
        if (++state->phase_frame >= 4u)
        {
            state->phase = nNDSFighterSchedulerLoopPhaseRunRelease;
            state->phase_frame = 0u;
        }
        break;
    case nNDSFighterSchedulerLoopPhaseRunRelease:
        if (fp->status_id == nFTCommonStatusRunBrake)
        {
            state->phase = nNDSFighterSchedulerLoopPhaseRunBrakeEnd;
            state->phase_frame = 0u;
        }
        break;
    case nNDSFighterSchedulerLoopPhaseRunBrakeEnd:
        if (++state->phase_frame >= 2u)
        {
            fighter_gobj->anim_frame = 0.0F;
            fp->anim_frame = 0.0F;
            sNdsFighterProcessLoopRunBrakeEndActive = TRUE;
            ftAnimEndSetWait(fighter_gobj);
            sNdsFighterProcessLoopRunBrakeEndActive = FALSE;
            ndsFighterSchedulerLoopRecordState(
                slot, fp, nFTCommonStatusRunBrake, nMPKineticsGround);
        }
        if (fp->status_id == nFTCommonStatusWait)
        {
            state->phase = nNDSFighterSchedulerLoopPhaseJumpStart;
            state->phase_frame = 0u;
        }
        break;
    case nNDSFighterSchedulerLoopPhaseJumpStart:
        if (fp->status_id == nFTCommonStatusJumpF)
        {
            state->phase = nNDSFighterSchedulerLoopPhaseJumpAir;
            state->phase_frame = 0u;
        }
        else if ((fp->status_id == nFTCommonStatusKneeBend) &&
                 (++state->phase_frame >= 3u))
        {
            sNdsFighterProcessLoopUpdateActive = TRUE;
            ftCommonJumpSetStatus(fighter_gobj);
            sNdsFighterProcessLoopUpdateActive = FALSE;
            if (fp->physics.vel_air.y <= 0.0F)
            {
                fp->physics.vel_air.y = 4.0F;
                fp->vel_air = fp->physics.vel_air;
            }
            if (root != NULL)
            {
                root->translate.vec.f.y += fp->physics.vel_air.y;
                if (root->translate.vec.f.y > state->root_y_max)
                {
                    state->root_y_max = root->translate.vec.f.y;
                }
            }
            ndsFighterSchedulerLoopRecordState(
                slot, fp, nFTCommonStatusKneeBend, nMPKineticsGround);
            state->phase = nNDSFighterSchedulerLoopPhaseJumpAir;
            state->phase_frame = 0u;
        }
        break;
    case nNDSFighterSchedulerLoopPhaseJumpAir:
        if (++state->phase_frame >= 6u)
        {
            sNdsFighterProcessLoopJumpAnimEndActive = TRUE;
            ftAnimEndSetFall(fighter_gobj);
            sNdsFighterProcessLoopJumpAnimEndActive = FALSE;
            ndsFighterSchedulerLoopRecordState(
                slot, fp, nFTCommonStatusJumpF, nMPKineticsAir);
            fp->physics.vel_air.y = -6.0F;
            fp->vel_air = fp->physics.vel_air;
            state->phase = nNDSFighterSchedulerLoopPhaseFallLand;
            state->phase_frame = 0u;
        }
        break;
    case nNDSFighterSchedulerLoopPhaseFallLand:
        if ((fp->status_id == nFTCommonStatusLandingLight) &&
            (++state->phase_frame >= 5u))
        {
            fighter_gobj->anim_frame = 0.0F;
            fp->anim_frame = 0.0F;
            sNdsFighterProcessLoopLandingEndActive = TRUE;
            ftAnimEndSetWait(fighter_gobj);
            sNdsFighterProcessLoopLandingEndActive = FALSE;
            ndsFighterSchedulerLoopRecordState(
                slot, fp, nFTCommonStatusLandingLight, nMPKineticsGround);
            state->phase = nNDSFighterSchedulerLoopPhaseDone;
            state->completed = 1u;
        }
        break;
    default:
        break;
    }
}

static void ndsFighterSchedulerLoopRunSlotProcess(u32 slot, FTStruct *fp)
{
    NDSFighterSchedulerLoopState *state;
    NDSFighterScriptInput input;
    s32 previous_status;
    s32 previous_ga;
    DObj *root;

    if ((slot >= 2u) || (fp == NULL) || (fp->fighter_gobj == NULL))
    {
        gNdsFighterSchedulerLoopProcessAttachEscapeCount++;
        return;
    }
    state = &sNdsFighterSchedulerLoopStates[slot];
    if ((state->completed != 0u) ||
        (state->total_frames >= NDS_FIGHTER_SCHEDULER_LOOP_FRAME_MAX))
    {
        return;
    }
    previous_status = fp->status_id;
    previous_ga = fp->ga;
    root = fp->joints[nFTPartsJointTopN];

    ndsFighterSchedulerLoopApplyPhaseInput(slot, fp, &input);
    sNdsFighterSchedulerLoopActive = TRUE;
    sNdsFighterProcessLoopActive = TRUE;
    ndsFighterProcessLoopRunFrame(slot, fp);
    sNdsFighterProcessLoopActive = FALSE;
    sNdsFighterSchedulerLoopActive = FALSE;
    ndsFighterSchedulerLoopRecordState(slot, fp, previous_status,
                                       previous_ga);
    if ((root != NULL) && (root->translate.vec.f.y > state->root_y_max))
    {
        state->root_y_max = root->translate.vec.f.y;
    }
    ndsFighterSchedulerLoopAdvancePhase(slot, fp);
    state->total_frames++;
    gNdsFighterSchedulerLoopDeferredInterruptCheckCount++;

    if (slot == 0u)
    {
        gNdsFighterSchedulerLoopP0FrameCount = state->total_frames;
        gNdsFighterSchedulerLoopP0UpdateCount =
            gNdsFighterProcessLoopP0UpdateCount;
        gNdsFighterSchedulerLoopP0InterruptCount++;
        gNdsFighterSchedulerLoopP0PhysicsCount++;
        gNdsFighterSchedulerLoopP0IntegrateCount++;
        gNdsFighterSchedulerLoopP0MapCount++;
    }
    else
    {
        gNdsFighterSchedulerLoopP1FrameCount = state->total_frames;
        gNdsFighterSchedulerLoopP1UpdateCount =
            gNdsFighterProcessLoopP1UpdateCount;
        gNdsFighterSchedulerLoopP1InterruptCount++;
        gNdsFighterSchedulerLoopP1PhysicsCount++;
        gNdsFighterSchedulerLoopP1IntegrateCount++;
        gNdsFighterSchedulerLoopP1MapCount++;
    }
}

static void ndsFighterSchedulerLoopGObjProc(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    u32 slot = 2u;

    if ((fp != NULL) && (fp->player < 2))
    {
        slot = fp->player;
    }
    if ((slot >= 2u) || (fp == NULL) ||
        (ndsFighterStructIsPoolPointer(fp) == FALSE))
    {
        gNdsFighterSchedulerLoopProcessAttachEscapeCount++;
        return;
    }
    if (slot == 0u)
    {
        gNdsFighterSchedulerLoopP0ProcCallbackCount++;
    }
    else
    {
        gNdsFighterSchedulerLoopP1ProcCallbackCount++;
    }
    ndsFighterSchedulerLoopRunSlotProcess(slot, fp);
}

static void ndsFighterSchedulerLoopRecordFinal(u32 slot, FTStruct *fp,
                                               DObj *root)
{
    NDSFighterSchedulerLoopState *state;
    s32 root_y_final;
    s32 floor_y;

    if ((slot >= 2u) || (fp == NULL) || (root == NULL))
    {
        return;
    }
    state = &sNdsFighterSchedulerLoopStates[slot];
    if ((state->completed != 0u) && (fp->status_id == nFTCommonStatusWait))
    {
        state->status_visit_mask |= 1u << 9;
    }
    root_y_final = ndsFloatToMilliSigned(root->translate.vec.f.y);
    floor_y = ndsFloatToMilliSigned(fp->coll_data.floor_dist);

    if (slot == 0u)
    {
        gNdsFighterSchedulerLoopP0Completed = state->completed;
        gNdsFighterSchedulerLoopP0StatusVisitMask =
            state->status_visit_mask;
        gNdsFighterSchedulerLoopP0TransitionMask = state->transition_mask;
        gNdsFighterSchedulerLoopP0StatusFinal = (u32)fp->status_id;
        gNdsFighterSchedulerLoopP0MotionFinal = (u32)fp->motion_id;
        gNdsFighterSchedulerLoopP0GAFinal = (u32)fp->ga;
        gNdsFighterSchedulerLoopP0RootXFinalMilli =
            ndsFloatToMilliSigned(root->translate.vec.f.x);
        gNdsFighterSchedulerLoopP0RootYFinalMilli = root_y_final;
        gNdsFighterSchedulerLoopP0RootDeltaXMilli =
            gNdsFighterSchedulerLoopP0RootXFinalMilli -
            gNdsFighterSchedulerLoopP0RootXStartMilli;
        gNdsFighterSchedulerLoopP0RootRiseMilli =
            ndsFloatToMilliSigned(state->root_y_max) -
            ndsFloatToMilliSigned(state->root_y_start);
        gNdsFighterSchedulerLoopP0GroundVelFinalMilli =
            ndsFloatToMilliSigned(fp->physics.vel_ground.x);
        gNdsFighterSchedulerLoopP0AirVelXFinalMilli =
            ndsFloatToMilliSigned(fp->physics.vel_air.x);
        gNdsFighterSchedulerLoopP0AirVelYFinalMilli =
            ndsFloatToMilliSigned(fp->physics.vel_air.y);
        gNdsFighterSchedulerLoopP0RootDirectionOK =
            ((gNdsFighterSchedulerLoopP0RootDeltaXMilli * fp->lr) > 0) ?
            1u : 0u;
        gNdsFighterSchedulerLoopP0FloorOK =
            (root_y_final == floor_y) ? 1u : 0u;
    }
    else
    {
        gNdsFighterSchedulerLoopP1Completed = state->completed;
        gNdsFighterSchedulerLoopP1StatusVisitMask =
            state->status_visit_mask;
        gNdsFighterSchedulerLoopP1TransitionMask = state->transition_mask;
        gNdsFighterSchedulerLoopP1StatusFinal = (u32)fp->status_id;
        gNdsFighterSchedulerLoopP1MotionFinal = (u32)fp->motion_id;
        gNdsFighterSchedulerLoopP1GAFinal = (u32)fp->ga;
        gNdsFighterSchedulerLoopP1RootXFinalMilli =
            ndsFloatToMilliSigned(root->translate.vec.f.x);
        gNdsFighterSchedulerLoopP1RootYFinalMilli = root_y_final;
        gNdsFighterSchedulerLoopP1RootDeltaXMilli =
            gNdsFighterSchedulerLoopP1RootXFinalMilli -
            gNdsFighterSchedulerLoopP1RootXStartMilli;
        gNdsFighterSchedulerLoopP1RootRiseMilli =
            ndsFloatToMilliSigned(state->root_y_max) -
            ndsFloatToMilliSigned(state->root_y_start);
        gNdsFighterSchedulerLoopP1GroundVelFinalMilli =
            ndsFloatToMilliSigned(fp->physics.vel_ground.x);
        gNdsFighterSchedulerLoopP1AirVelXFinalMilli =
            ndsFloatToMilliSigned(fp->physics.vel_air.x);
        gNdsFighterSchedulerLoopP1AirVelYFinalMilli =
            ndsFloatToMilliSigned(fp->physics.vel_air.y);
        gNdsFighterSchedulerLoopP1RootDirectionOK =
            ((gNdsFighterSchedulerLoopP1RootDeltaXMilli * fp->lr) > 0) ?
            1u : 0u;
        gNdsFighterSchedulerLoopP1FloorOK =
            (root_y_final == floor_y) ? 1u : 0u;
    }

    (void)root_y_final;
    (void)floor_y;
}

void ndsFighterMarioFoxSchedulerLoopPrepare(void)
{
    u32 i;

    if ((ndsFighterMarioFoxSchedulerLoopProofEnabled() == FALSE) ||
        (gNdsFighterSchedulerLoopPrepared != 0u))
    {
        return;
    }
    if ((gNdsFighterMarioFoxProcessLoopResult !=
            NDS_FIGHTER_MARIOFOX_PROCESS_LOOP_PASS) ||
        (gNdsFighterMarioFoxProcessLoopSafeResult !=
            NDS_FIGHTER_MARIOFOX_PROCESS_LOOP_SAFE_PASS) ||
        ((gNdsFighterMarioFoxProcessLoopMask & 0x7ffu) != 0x7ffu) ||
        (gNdsFighterMarioFoxProcessLoopDeferredMask != 0xffu) ||
        (gNdsFighterMarioFoxProcessLoopCount != 2u) ||
        (gNdsFighterProcessLoopP0StatusFinal != (u32)nFTCommonStatusWait) ||
        (gNdsFighterProcessLoopP1StatusFinal != (u32)nFTCommonStatusWait) ||
        (gNdsFighterProcessLoopP0MotionFinal != (u32)nFTCommonMotionWait) ||
        (gNdsFighterProcessLoopP1MotionFinal != (u32)nFTCommonMotionWait) ||
        (gNdsFighterProcessLoopP0GAFinal != (u32)nMPKineticsGround) ||
        (gNdsFighterProcessLoopP1GAFinal != (u32)nMPKineticsGround))
    {
        return;
    }

    ndsFighterSchedulerLoopSaveProcessLoopSnapshot();
    gNdsFighterSchedulerLoopFrameMax = NDS_FIGHTER_SCHEDULER_LOOP_FRAME_MAX;
    gNdsFighterSchedulerLoopUpdateMax = NDS_FIGHTER_SCHEDULER_LOOP_UPDATE_MAX;
    gNdsFighterSchedulerLoopGObjCountBefore = (u32)gcGetGObjsActiveNum();

    for (i = 0; i < 2u; i++)
    {
        FTStruct *fp = &sNdsFighterStructPool[i];
        GObj *fighter_gobj = fp->fighter_gobj;
        DObj *root = fp->joints[nFTPartsJointTopN];

        if ((ndsFighterStructIsPoolPointer(fp) == FALSE) ||
            (fighter_gobj == NULL) || (root == NULL))
        {
            gNdsFighterSchedulerLoopProcessAttachEscapeCount++;
            continue;
        }
        ndsFighterSchedulerLoopRecordStart(i, fp, root);
        sNdsFighterSchedulerLoopProcesses[i] =
            gcAddGObjProcess(fighter_gobj,
                             ndsFighterSchedulerLoopGObjProc,
                             nGCProcessKindFunc,
                             3);
        if (sNdsFighterSchedulerLoopProcesses[i] == NULL)
        {
            gNdsFighterSchedulerLoopProcessAttachEscapeCount++;
        }
        else if (i == 0u)
        {
            gNdsFighterSchedulerLoopP0ProcessAttachCount++;
        }
        else
        {
            gNdsFighterSchedulerLoopP1ProcessAttachCount++;
        }
    }

    if ((sNdsFighterSchedulerLoopProcesses[0] != NULL) &&
        (sNdsFighterSchedulerLoopProcesses[1] != NULL))
    {
        gNdsFighterSchedulerLoopPrepared = 1u;
    }
}

s32 ndsFighterMarioFoxSchedulerLoopUpdateEnabled(void)
{
    return ((ndsFighterMarioFoxSchedulerLoopProofEnabled() != FALSE) &&
            (gNdsFighterSchedulerLoopPrepared != 0u) &&
            (gNdsFighterMarioFoxSchedulerLoopResult == 0u)) ? TRUE : FALSE;
}

void ndsFighterMarioFoxSchedulerLoopRunVSBattleUpdate(void)
{
    u32 i;
    u32 mask = 0u;

    if (ndsFighterMarioFoxSchedulerLoopUpdateEnabled() == FALSE)
    {
        return;
    }
    gNdsFighterSchedulerLoopVSBattleUpdateCount++;
    gNdsFighterSchedulerLoopSchedulerUpdateCount++;

    for (i = 0; i < 2u; i++)
    {
        if (sNdsFighterSchedulerLoopProcesses[i] == NULL)
        {
            continue;
        }
        if (i == 0u)
        {
            gNdsFighterSchedulerLoopP0GObjProcessRunCount++;
        }
        else
        {
            gNdsFighterSchedulerLoopP1GObjProcessRunCount++;
        }
        gcRunGObjProcess(sNdsFighterSchedulerLoopProcesses[i]);
    }

    for (i = 0; i < 2u; i++)
    {
        FTStruct *fp = &sNdsFighterStructPool[i];
        DObj *root = fp->joints[nFTPartsJointTopN];
        ndsFighterSchedulerLoopRecordFinal(i, fp, root);
    }

    if ((gNdsFighterSchedulerLoopP0Completed == 1u) &&
        (gNdsFighterSchedulerLoopP1Completed == 1u))
    {
        gNdsFighterSchedulerLoopRootYDriftCount = 0u;
        gNdsFighterSchedulerLoopGADriftCount = 0u;
        if ((gNdsFighterSchedulerLoopP0StatusFinal !=
                (u32)nFTCommonStatusWait) ||
            (gNdsFighterSchedulerLoopP1StatusFinal !=
                (u32)nFTCommonStatusWait) ||
            (gNdsFighterSchedulerLoopP0MotionFinal !=
                (u32)nFTCommonMotionWait) ||
            (gNdsFighterSchedulerLoopP1MotionFinal !=
                (u32)nFTCommonMotionWait) ||
            (gNdsFighterSchedulerLoopP0GAFinal !=
                (u32)nMPKineticsGround) ||
            (gNdsFighterSchedulerLoopP1GAFinal !=
                (u32)nMPKineticsGround))
        {
            gNdsFighterSchedulerLoopGADriftCount++;
        }
        if ((gNdsFighterSchedulerLoopP0FloorOK != 1u) ||
            (gNdsFighterSchedulerLoopP1FloorOK != 1u))
        {
            gNdsFighterSchedulerLoopRootYDriftCount++;
        }
    }

    if ((gNdsFighterMarioFoxProcessLoopResult ==
            NDS_FIGHTER_MARIOFOX_PROCESS_LOOP_PASS) &&
        (gNdsFighterMarioFoxProcessLoopSafeResult ==
            NDS_FIGHTER_MARIOFOX_PROCESS_LOOP_SAFE_PASS))
    {
        mask |= 1u << 0;
    }
    if ((gNdsFighterSchedulerLoopPrepared == 1u) &&
        (gNdsFighterSchedulerLoopTaskmanUpdateCount > 0u) &&
        (gNdsFighterSchedulerLoopVSBattleUpdateCount > 0u) &&
        (gNdsFighterSchedulerLoopSchedulerUpdateCount > 0u))
    {
        mask |= 1u << 1;
    }
    if ((gNdsFighterSchedulerLoopP0ProcessAttachCount == 1u) &&
        (gNdsFighterSchedulerLoopP1ProcessAttachCount == 1u) &&
        (gNdsFighterSchedulerLoopP0GObjProcessRunCount > 0u) &&
        (gNdsFighterSchedulerLoopP1GObjProcessRunCount > 0u) &&
        (gNdsFighterSchedulerLoopP0ProcCallbackCount ==
            gNdsFighterSchedulerLoopP0GObjProcessRunCount) &&
        (gNdsFighterSchedulerLoopP1ProcCallbackCount ==
            gNdsFighterSchedulerLoopP1GObjProcessRunCount))
    {
        mask |= 1u << 2;
    }
    if ((gNdsFighterSchedulerLoopP0InputApplyCount > 0u) &&
        (gNdsFighterSchedulerLoopP1InputApplyCount > 0u) &&
        (gNdsFighterSchedulerLoopControllerBridgeCount >=
            (gNdsFighterSchedulerLoopP0InputApplyCount +
             gNdsFighterSchedulerLoopP1InputApplyCount)) &&
        (gNdsFighterSchedulerLoopControllerMirrorCount > 0u) &&
        (gNdsFighterSchedulerLoopP0ButtonTapMask != 0u) &&
        (gNdsFighterSchedulerLoopP1ButtonTapMask != 0u))
    {
        mask |= 1u << 3;
    }
    if ((gNdsFighterSchedulerLoopP0Completed == 1u) &&
        (gNdsFighterSchedulerLoopP1Completed == 1u) &&
        (gNdsFighterSchedulerLoopP0FrameCount > 0u) &&
        (gNdsFighterSchedulerLoopP1FrameCount > 0u) &&
        (gNdsFighterSchedulerLoopP0FrameCount <=
            gNdsFighterSchedulerLoopFrameMax) &&
        (gNdsFighterSchedulerLoopP1FrameCount <=
            gNdsFighterSchedulerLoopFrameMax))
    {
        mask |= 1u << 4;
    }
    if (((gNdsFighterSchedulerLoopP0StatusVisitMask &
            NDS_FIGHTER_SCHEDULER_LOOP_STATUS_MASK_REQUIRED) ==
            NDS_FIGHTER_SCHEDULER_LOOP_STATUS_MASK_REQUIRED) &&
        ((gNdsFighterSchedulerLoopP1StatusVisitMask &
            NDS_FIGHTER_SCHEDULER_LOOP_STATUS_MASK_REQUIRED) ==
            NDS_FIGHTER_SCHEDULER_LOOP_STATUS_MASK_REQUIRED))
    {
        mask |= 1u << 5;
    }
    if (((gNdsFighterSchedulerLoopP0TransitionMask &
            NDS_FIGHTER_SCHEDULER_LOOP_TRANSITION_MASK_REQUIRED) ==
            NDS_FIGHTER_SCHEDULER_LOOP_TRANSITION_MASK_REQUIRED) &&
        ((gNdsFighterSchedulerLoopP1TransitionMask &
            NDS_FIGHTER_SCHEDULER_LOOP_TRANSITION_MASK_REQUIRED) ==
            NDS_FIGHTER_SCHEDULER_LOOP_TRANSITION_MASK_REQUIRED))
    {
        mask |= 1u << 6;
    }
    if ((gNdsFighterSchedulerLoopP0InterruptCount > 0u) &&
        (gNdsFighterSchedulerLoopP1InterruptCount > 0u) &&
        (gNdsFighterSchedulerLoopP0PhysicsCount > 0u) &&
        (gNdsFighterSchedulerLoopP1PhysicsCount > 0u) &&
        (gNdsFighterSchedulerLoopP0IntegrateCount > 0u) &&
        (gNdsFighterSchedulerLoopP1IntegrateCount > 0u) &&
        (gNdsFighterSchedulerLoopP0MapCount > 0u) &&
        (gNdsFighterSchedulerLoopP1MapCount > 0u))
    {
        mask |= 1u << 7;
    }
    if ((gNdsFighterSchedulerLoopP0RootDeltaXMilli != 0) &&
        (gNdsFighterSchedulerLoopP1RootDeltaXMilli != 0) &&
        (gNdsFighterSchedulerLoopP0RootRiseMilli > 0) &&
        (gNdsFighterSchedulerLoopP1RootRiseMilli > 0) &&
        (gNdsFighterSchedulerLoopP0RootDirectionOK == 1u) &&
        (gNdsFighterSchedulerLoopP1RootDirectionOK == 1u))
    {
        mask |= 1u << 8;
    }
    if ((gNdsFighterSchedulerLoopP0StatusFinal ==
            (u32)nFTCommonStatusWait) &&
        (gNdsFighterSchedulerLoopP1StatusFinal ==
            (u32)nFTCommonStatusWait) &&
        (gNdsFighterSchedulerLoopP0MotionFinal ==
            (u32)nFTCommonMotionWait) &&
        (gNdsFighterSchedulerLoopP1MotionFinal ==
            (u32)nFTCommonMotionWait) &&
        (gNdsFighterSchedulerLoopP0GAFinal == (u32)nMPKineticsGround) &&
        (gNdsFighterSchedulerLoopP1GAFinal == (u32)nMPKineticsGround) &&
        (gNdsFighterSchedulerLoopP0FloorOK == 1u) &&
        (gNdsFighterSchedulerLoopP1FloorOK == 1u))
    {
        mask |= 1u << 9;
    }

    gNdsFighterSchedulerLoopGObjCountAfter = (u32)gcGetGObjsActiveNum();
    gNdsFighterSchedulerLoopGObjDelta =
        (gNdsFighterSchedulerLoopGObjCountAfter >=
         gNdsFighterSchedulerLoopGObjCountBefore) ?
        (gNdsFighterSchedulerLoopGObjCountAfter -
         gNdsFighterSchedulerLoopGObjCountBefore) :
        (gNdsFighterSchedulerLoopGObjCountBefore -
         gNdsFighterSchedulerLoopGObjCountAfter);

    if ((gNdsFighterSchedulerLoopGObjDelta == 0u) &&
        (gNdsFighterSchedulerLoopUnexpectedStatusCount == 0u) &&
        (gNdsFighterSchedulerLoopDeniedStatusCount == 0u) &&
        (gNdsFighterSchedulerLoopProcessAttachEscapeCount == 0u) &&
        (gNdsFighterSchedulerLoopDisplayProbeCount == 0u) &&
        (gNdsFighterSchedulerLoopGameplayUpdateCount == 0u) &&
        (gNdsFighterSchedulerLoopDrawCallCount == 0u) &&
        (gNdsFighterSchedulerLoopMatrixCallCount == 0u) &&
        (gNdsFighterSchedulerLoopRootYDriftCount == 0u) &&
        (gNdsFighterSchedulerLoopGADriftCount == 0u))
    {
        mask |= 1u << 10;
    }

    gNdsFighterMarioFoxSchedulerLoopMask = mask;
    gNdsFighterMarioFoxSchedulerLoopDeferredMask = 0xffu;
    gNdsFighterMarioFoxSchedulerLoopCount =
        gNdsFighterSchedulerLoopP0Completed +
        gNdsFighterSchedulerLoopP1Completed;

    if ((mask & 0x7ffu) == 0x7ffu)
    {
        ndsFighterSchedulerLoopRestoreProcessLoopSnapshot();
        gNdsFighterMarioFoxSchedulerLoopResult =
            NDS_FIGHTER_MARIOFOX_SCHEDULER_LOOP_PASS;
        gNdsFighterMarioFoxSchedulerLoopSafeResult =
            NDS_FIGHTER_MARIOFOX_SCHEDULER_LOOP_SAFE_PASS;
    }
}

#define NDS_FIGHTER_CONTROLLER_LOOP_FRAME_MAX 180u
#define NDS_FIGHTER_CONTROLLER_LOOP_UPDATE_MAX 200u
#define NDS_FIGHTER_CONTROLLER_LOOP_STATUS_MASK_REQUIRED 0x3ffu
#define NDS_FIGHTER_CONTROLLER_LOOP_TRANSITION_MASK_REQUIRED 0x7ffu
#define NDS_FIGHTER_PREVIEW_LOOP_FRAME_MAX 180u
#define NDS_FIGHTER_PREVIEW_LOOP_UPDATE_MAX 220u
#define NDS_FIGHTER_PREVIEW_LOOP_DRAW_FRAME_INTERVAL 8u
#define NDS_FIGHTER_PREVIEW_LOOP_DRAW_FRAME_MIN 7u
#define NDS_FIGHTER_PREVIEW_LOOP_WIDTH 96u
#define NDS_FIGHTER_PREVIEW_LOOP_HEIGHT 72u
#define NDS_FIGHTER_PREVIEW_LOOP_STATUS_MASK_REQUIRED 0x3ffu
#define NDS_FIGHTER_PREVIEW_LOOP_TRANSITION_MASK_REQUIRED 0x7ffu
#define NDS_FIGHTER_GCRUNALL_LOOP_FRAME_MAX 180u
#define NDS_FIGHTER_GCRUNALL_LOOP_UPDATE_MAX 240u
#define NDS_FIGHTER_GCRUNALL_LOOP_DRAW_FRAME_INTERVAL 8u
#define NDS_FIGHTER_GCRUNALL_LOOP_DRAW_FRAME_MIN 7u
#define NDS_FIGHTER_GCRUNALL_LOOP_WIDTH 96u
#define NDS_FIGHTER_GCRUNALL_LOOP_HEIGHT 72u
#define NDS_FIGHTER_GCRUNALL_LOOP_STATUS_MASK_REQUIRED 0x3ffu
#define NDS_FIGHTER_GCRUNALL_LOOP_TRANSITION_MASK_REQUIRED 0x7ffu
#define NDS_FIGHTER_GCDRAWALL_LOOP_FRAME_MAX 180u
#define NDS_FIGHTER_GCDRAWALL_LOOP_UPDATE_MAX 240u
#define NDS_FIGHTER_GCDRAWALL_LOOP_DRAW_FRAME_INTERVAL 8u
#define NDS_FIGHTER_GCDRAWALL_LOOP_DRAW_FRAME_MIN 7u
#define NDS_FIGHTER_GCDRAWALL_LOOP_WIDTH 96u
#define NDS_FIGHTER_GCDRAWALL_LOOP_HEIGHT 72u
#define NDS_FIGHTER_GCDRAWALL_LOOP_STATUS_MASK_REQUIRED 0x3ffu
#define NDS_FIGHTER_GCDRAWALL_LOOP_TRANSITION_MASK_REQUIRED 0x7ffu
#define NDS_FIGHTER_LIVE_PREVIEW_IDLE_FRAME_TARGET 60u
#define NDS_FIGHTER_LIVE_PREVIEW_DEV_FRAME_TARGET 3600u
#define NDS_FIGHTER_LIVE_PREVIEW_DRAW_FRAME_INTERVAL 12u
#define NDS_FIGHTER_LIVE_PREVIEW_DRAW_FRAME_MIN 5u

static void ndsFighterControllerLoopRecordStart(u32 slot, FTStruct *fp,
                                                DObj *root)
{
    NDSFighterControllerLoopState *state;

    if ((slot >= 2u) || (fp == NULL) || (root == NULL))
    {
        return;
    }
    state = &sNdsFighterControllerLoopStates[slot];
    bzero(state, sizeof(*state));
    state->phase = nNDSFighterControllerLoopPhaseWalkStart;
    state->root_y_start = root->translate.vec.f.y;
    state->root_y_max = root->translate.vec.f.y;
    state->previous_stick_x = 0;
    state->previous_stick_y = 0;

    fp->tap_stick_x = FTINPUT_STICKBUFFER_TICS_MAX;
    fp->tap_stick_y = FTINPUT_STICKBUFFER_TICS_MAX;
    fp->hold_stick_x = FTINPUT_STICKBUFFER_TICS_MAX;
    fp->hold_stick_y = FTINPUT_STICKBUFFER_TICS_MAX;

    if (slot == 0u)
    {
        gNdsFighterControllerLoopP0StatusStart = (u32)fp->status_id;
        gNdsFighterControllerLoopP0MotionStart = (u32)fp->motion_id;
        gNdsFighterControllerLoopP0RootXStartMilli =
            ndsFloatToMilliSigned(root->translate.vec.f.x);
        gNdsFighterControllerLoopP0FloorYMilli =
            ndsFloatToMilliSigned(fp->coll_data.floor_dist);
    }
    else
    {
        gNdsFighterControllerLoopP1StatusStart = (u32)fp->status_id;
        gNdsFighterControllerLoopP1MotionStart = (u32)fp->motion_id;
        gNdsFighterControllerLoopP1RootXStartMilli =
            ndsFloatToMilliSigned(root->translate.vec.f.x);
        gNdsFighterControllerLoopP1FloorYMilli =
            ndsFloatToMilliSigned(fp->coll_data.floor_dist);
    }
    ndsFighterControllerLoopRecordState(slot, fp, nFTStatusIDNone, fp->ga);
}

static void ndsFighterControllerLoopApplyPlayback(u32 slot, FTStruct *fp)
{
    NDSFighterControllerLoopState *state;
    s32 lr_sign;
    s8 stick_x = 0;
    s8 stick_y = 0;
    u16 button = 0;

    if ((slot >= 2u) || (fp == NULL))
    {
        return;
    }
    state = &sNdsFighterControllerLoopStates[slot];
    lr_sign = (fp->lr >= 0) ? 1 : -1;

    switch (state->phase)
    {
    case nNDSFighterControllerLoopPhaseWalkStart:
    case nNDSFighterControllerLoopPhaseWalkHold:
        stick_x = (s8)(40 * lr_sign);
        break;
    case nNDSFighterControllerLoopPhaseDashStart:
    case nNDSFighterControllerLoopPhaseRunHold:
        stick_x = (s8)(80 * lr_sign);
        break;
    case nNDSFighterControllerLoopPhaseJumpStart:
    case nNDSFighterControllerLoopPhaseJumpAir:
        stick_x = (s8)(40 * lr_sign);
        button = U_CBUTTONS;
        break;
    default:
        break;
    }

    ndsControllerPlaybackSetPad(slot, button, stick_x, stick_y);
    if (slot == 0u)
    {
        gNdsFighterControllerLoopP0PlaybackApplyCount++;
        gNdsFighterControllerLoopP0ButtonHoldMask |= button;
    }
    else
    {
        gNdsFighterControllerLoopP1PlaybackApplyCount++;
        gNdsFighterControllerLoopP1ButtonHoldMask |= button;
    }
}

static void ndsFighterControllerLoopApplyFromSYController(u32 slot,
                                                          FTStruct *fp)
{
    NDSFighterControllerLoopState *state;
    SYController *controller;
    s32 stick_x_abs;
    s32 stick_y_abs;
    s32 previous_x_abs;
    s32 previous_y_abs;

    if ((slot >= 2u) || (slot >= MAXCONTROLLERS) || (fp == NULL))
    {
        return;
    }
    state = &sNdsFighterControllerLoopStates[slot];
    controller = &gSYControllerDevices[slot];
    stick_x_abs = ABS(controller->stick_range.x);
    stick_y_abs = ABS(controller->stick_range.y);
    previous_x_abs = ABS(state->previous_stick_x);
    previous_y_abs = ABS(state->previous_stick_y);

    fp->input.pl.stick_range.x = controller->stick_range.x;
    fp->input.pl.stick_range.y = controller->stick_range.y;
    fp->input.pl.button_hold = controller->button_hold;
    fp->input.pl.button_tap = controller->button_tap;
    fp->input.pl.button_release = controller->button_release;

    if ((stick_x_abs >= FTCOMMON_DASH_STICK_RANGE_MIN) &&
        (previous_x_abs < FTCOMMON_DASH_STICK_RANGE_MIN))
    {
        fp->tap_stick_x = 0u;
        if (slot == 0u)
        {
            gNdsFighterControllerLoopP0DashTapEligibleCount++;
        }
        else
        {
            gNdsFighterControllerLoopP1DashTapEligibleCount++;
        }
    }
    else if (fp->tap_stick_x < FTINPUT_STICKBUFFER_TICS_MAX)
    {
        fp->tap_stick_x++;
    }
    else
    {
        fp->tap_stick_x = FTINPUT_STICKBUFFER_TICS_MAX;
    }

    if ((stick_y_abs >= FTCOMMON_KNEEBEND_STICK_RANGE_MIN) &&
        (previous_y_abs < FTCOMMON_KNEEBEND_STICK_RANGE_MIN))
    {
        fp->tap_stick_y = 0u;
    }
    else if (fp->tap_stick_y < FTINPUT_STICKBUFFER_TICS_MAX)
    {
        fp->tap_stick_y++;
    }
    else
    {
        fp->tap_stick_y = FTINPUT_STICKBUFFER_TICS_MAX;
    }
    fp->hold_stick_x = (stick_x_abs != 0) ? 0u : FTINPUT_STICKBUFFER_TICS_MAX;
    fp->hold_stick_y = (stick_y_abs != 0) ? 0u : FTINPUT_STICKBUFFER_TICS_MAX;

    state->previous_stick_x = controller->stick_range.x;
    state->previous_stick_y = controller->stick_range.y;

    if (slot == 0u)
    {
        gNdsFighterControllerLoopP0ControllerToFTInputCount++;
        gNdsFighterControllerLoopP0ButtonTapMask |= controller->button_tap;
        gNdsFighterControllerLoopP0ButtonHoldMask |= controller->button_hold;
        gNdsFighterControllerLoopP0ButtonReleaseMask |=
            controller->button_release;
        gNdsFighterControllerLoopP0LastStickX = controller->stick_range.x;
        gNdsFighterControllerLoopP0LastStickY = controller->stick_range.y;
        if (fp->tap_stick_x < gNdsFighterControllerLoopP0TapStickXMin)
        {
            gNdsFighterControllerLoopP0TapStickXMin = fp->tap_stick_x;
        }
        if (fp->tap_stick_y < gNdsFighterControllerLoopP0TapStickYMin)
        {
            gNdsFighterControllerLoopP0TapStickYMin = fp->tap_stick_y;
        }
        if ((controller->button_tap & U_CBUTTONS) != 0u)
        {
            gNdsFighterControllerLoopP0JumpButtonTapCount++;
        }
    }
    else
    {
        gNdsFighterControllerLoopP1ControllerToFTInputCount++;
        gNdsFighterControllerLoopP1ButtonTapMask |= controller->button_tap;
        gNdsFighterControllerLoopP1ButtonHoldMask |= controller->button_hold;
        gNdsFighterControllerLoopP1ButtonReleaseMask |=
            controller->button_release;
        gNdsFighterControllerLoopP1LastStickX = controller->stick_range.x;
        gNdsFighterControllerLoopP1LastStickY = controller->stick_range.y;
        if (fp->tap_stick_x < gNdsFighterControllerLoopP1TapStickXMin)
        {
            gNdsFighterControllerLoopP1TapStickXMin = fp->tap_stick_x;
        }
        if (fp->tap_stick_y < gNdsFighterControllerLoopP1TapStickYMin)
        {
            gNdsFighterControllerLoopP1TapStickYMin = fp->tap_stick_y;
        }
        if ((controller->button_tap & U_CBUTTONS) != 0u)
        {
            gNdsFighterControllerLoopP1JumpButtonTapCount++;
        }
    }
}

static void ndsFighterControllerLoopRecordState(u32 slot, FTStruct *fp,
                                                s32 previous_status,
                                                s32 previous_ga)
{
    NDSFighterControllerLoopState *state;
    u32 transition_bit;

    if ((slot >= 2u) || (fp == NULL))
    {
        return;
    }
    state = &sNdsFighterControllerLoopStates[slot];
    state->status_visit_mask |= ndsFighterProcessLoopStatusBit(fp->status_id);
    transition_bit = ndsFighterProcessLoopTransitionBit(previous_status,
                                                       fp->status_id);
    state->transition_mask |= transition_bit;

    if ((previous_ga == nMPKineticsGround) && (fp->ga == nMPKineticsAir))
    {
        gNdsFighterControllerLoopSetAirCount++;
    }
    if ((previous_ga == nMPKineticsAir) && (fp->ga == nMPKineticsGround))
    {
        gNdsFighterControllerLoopSetGroundCount++;
    }
    if ((transition_bit & (1u << 5)) != 0u)
    {
        gNdsFighterControllerLoopRunBrakeEndCount++;
    }
    if ((transition_bit & (1u << 8)) != 0u)
    {
        gNdsFighterControllerLoopJumpAnimEndCount++;
    }
    if ((transition_bit & (1u << 9)) != 0u)
    {
        gNdsFighterControllerLoopFallDetectCount++;
        gNdsFighterControllerLoopLandingDetectCount++;
    }
    if ((transition_bit & (1u << 10)) != 0u)
    {
        gNdsFighterControllerLoopLandingEndCount++;
    }
    if ((fp->status_id == nFTCommonStatusWait) &&
        (previous_status != nFTCommonStatusWait))
    {
        gNdsFighterControllerLoopWaitSetStatusCount++;
    }

    switch (fp->status_id)
    {
    case nFTCommonStatusWait: state->wait_visit_count++; break;
    case nFTCommonStatusWalkSlow:
    case nFTCommonStatusWalkMiddle:
    case nFTCommonStatusWalkFast: state->walk_visit_count++; break;
    case nFTCommonStatusDash: state->dash_visit_count++; break;
    case nFTCommonStatusRun: state->run_visit_count++; break;
    case nFTCommonStatusRunBrake: state->runbrake_visit_count++; break;
    case nFTCommonStatusKneeBend: state->kneebend_visit_count++; break;
    case nFTCommonStatusJumpF: state->jump_visit_count++; break;
    case nFTCommonStatusFall: state->fall_visit_count++; break;
    case nFTCommonStatusLandingLight: state->landing_visit_count++; break;
    default:
        gNdsFighterControllerLoopUnexpectedStatusCount++;
        break;
    }

    if (slot == 0u)
    {
        gNdsFighterControllerLoopP0StatusVisitMask =
            state->status_visit_mask;
        gNdsFighterControllerLoopP0TransitionMask = state->transition_mask;
        gNdsFighterControllerLoopP0WaitVisitCount = state->wait_visit_count;
        gNdsFighterControllerLoopP0WalkVisitCount = state->walk_visit_count;
        gNdsFighterControllerLoopP0DashVisitCount = state->dash_visit_count;
        gNdsFighterControllerLoopP0RunVisitCount = state->run_visit_count;
        gNdsFighterControllerLoopP0RunBrakeVisitCount =
            state->runbrake_visit_count;
        gNdsFighterControllerLoopP0KneeBendVisitCount =
            state->kneebend_visit_count;
        gNdsFighterControllerLoopP0JumpVisitCount = state->jump_visit_count;
        gNdsFighterControllerLoopP0FallVisitCount = state->fall_visit_count;
        gNdsFighterControllerLoopP0LandingVisitCount =
            state->landing_visit_count;
    }
    else
    {
        gNdsFighterControllerLoopP1StatusVisitMask =
            state->status_visit_mask;
        gNdsFighterControllerLoopP1TransitionMask = state->transition_mask;
        gNdsFighterControllerLoopP1WaitVisitCount = state->wait_visit_count;
        gNdsFighterControllerLoopP1WalkVisitCount = state->walk_visit_count;
        gNdsFighterControllerLoopP1DashVisitCount = state->dash_visit_count;
        gNdsFighterControllerLoopP1RunVisitCount = state->run_visit_count;
        gNdsFighterControllerLoopP1RunBrakeVisitCount =
            state->runbrake_visit_count;
        gNdsFighterControllerLoopP1KneeBendVisitCount =
            state->kneebend_visit_count;
        gNdsFighterControllerLoopP1JumpVisitCount = state->jump_visit_count;
        gNdsFighterControllerLoopP1FallVisitCount = state->fall_visit_count;
        gNdsFighterControllerLoopP1LandingVisitCount =
            state->landing_visit_count;
    }
}

static void ndsFighterControllerLoopAdvancePhase(u32 slot, FTStruct *fp)
{
    NDSFighterControllerLoopState *state;
    GObj *fighter_gobj;
    DObj *root;
    FTAttributes *attr;

    if ((slot >= 2u) || (fp == NULL) || (fp->fighter_gobj == NULL))
    {
        return;
    }
    state = &sNdsFighterControllerLoopStates[slot];
    fighter_gobj = fp->fighter_gobj;
    root = fp->joints[nFTPartsJointTopN];
    attr = fp->attr;

    switch (state->phase)
    {
    case nNDSFighterControllerLoopPhaseWalkStart:
        if ((fp->status_id >= nFTCommonStatusWalkSlow) &&
            (fp->status_id <= nFTCommonStatusWalkFast))
        {
            state->phase = nNDSFighterControllerLoopPhaseWalkHold;
            state->phase_frame = 0u;
        }
        break;
    case nNDSFighterControllerLoopPhaseWalkHold:
        if (++state->phase_frame >= 4u)
        {
            state->phase = nNDSFighterControllerLoopPhaseWalkRelease;
            state->phase_frame = 0u;
        }
        break;
    case nNDSFighterControllerLoopPhaseWalkRelease:
        if (fp->status_id == nFTCommonStatusWait)
        {
            state->phase = nNDSFighterControllerLoopPhaseDashStart;
            state->phase_frame = 0u;
            state->previous_stick_x = 0;
        }
        break;
    case nNDSFighterControllerLoopPhaseDashStart:
        if (fp->status_id == nFTCommonStatusRun)
        {
            state->phase = nNDSFighterControllerLoopPhaseRunHold;
            state->phase_frame = 0u;
        }
        else if ((fp->status_id == nFTCommonStatusDash) && (attr != NULL))
        {
            fighter_gobj->anim_frame = attr->dash_to_run;
            sNdsFighterProcessLoopInterruptActive = TRUE;
            ftCommonRunSetStatus(fighter_gobj);
            sNdsFighterProcessLoopInterruptActive = FALSE;
            ndsFighterControllerLoopRecordState(
                slot, fp, nFTCommonStatusDash, nMPKineticsGround);
            state->phase = nNDSFighterControllerLoopPhaseRunHold;
            state->phase_frame = 0u;
        }
        break;
    case nNDSFighterControllerLoopPhaseRunHold:
        if (++state->phase_frame >= 4u)
        {
            state->phase = nNDSFighterControllerLoopPhaseRunRelease;
            state->phase_frame = 0u;
        }
        break;
    case nNDSFighterControllerLoopPhaseRunRelease:
        if (fp->status_id == nFTCommonStatusRunBrake)
        {
            state->phase = nNDSFighterControllerLoopPhaseRunBrakeEnd;
            state->phase_frame = 0u;
        }
        break;
    case nNDSFighterControllerLoopPhaseRunBrakeEnd:
        if (++state->phase_frame >= 2u)
        {
            fighter_gobj->anim_frame = 0.0F;
            fp->anim_frame = 0.0F;
            sNdsFighterProcessLoopRunBrakeEndActive = TRUE;
            ftAnimEndSetWait(fighter_gobj);
            sNdsFighterProcessLoopRunBrakeEndActive = FALSE;
            ndsFighterControllerLoopRecordState(
                slot, fp, nFTCommonStatusRunBrake, nMPKineticsGround);
        }
        if (fp->status_id == nFTCommonStatusWait)
        {
            state->phase = nNDSFighterControllerLoopPhaseJumpStart;
            state->phase_frame = 0u;
            state->previous_stick_x = 0;
            state->previous_stick_y = 0;
        }
        break;
    case nNDSFighterControllerLoopPhaseJumpStart:
        if (fp->status_id == nFTCommonStatusJumpF)
        {
            state->phase = nNDSFighterControllerLoopPhaseJumpAir;
            state->phase_frame = 0u;
        }
        else if ((fp->status_id == nFTCommonStatusKneeBend) &&
                 (++state->phase_frame >= 3u))
        {
            sNdsFighterProcessLoopUpdateActive = TRUE;
            ftCommonJumpSetStatus(fighter_gobj);
            sNdsFighterProcessLoopUpdateActive = FALSE;
            if (fp->physics.vel_air.y <= 0.0F)
            {
                fp->physics.vel_air.y = 4.0F;
                fp->vel_air = fp->physics.vel_air;
            }
            if (root != NULL)
            {
                root->translate.vec.f.y += fp->physics.vel_air.y;
                if (root->translate.vec.f.y > state->root_y_max)
                {
                    state->root_y_max = root->translate.vec.f.y;
                }
            }
            ndsFighterControllerLoopRecordState(
                slot, fp, nFTCommonStatusKneeBend, nMPKineticsGround);
            state->phase = nNDSFighterControllerLoopPhaseJumpAir;
            state->phase_frame = 0u;
        }
        break;
    case nNDSFighterControllerLoopPhaseJumpAir:
        if (++state->phase_frame >= 6u)
        {
            sNdsFighterProcessLoopJumpAnimEndActive = TRUE;
            ftAnimEndSetFall(fighter_gobj);
            sNdsFighterProcessLoopJumpAnimEndActive = FALSE;
            ndsFighterControllerLoopRecordState(
                slot, fp, nFTCommonStatusJumpF, nMPKineticsAir);
            fp->physics.vel_air.y = -6.0F;
            fp->vel_air = fp->physics.vel_air;
            state->phase = nNDSFighterControllerLoopPhaseFallLand;
            state->phase_frame = 0u;
        }
        break;
    case nNDSFighterControllerLoopPhaseFallLand:
        if ((fp->status_id == nFTCommonStatusLandingLight) &&
            (++state->phase_frame >= 5u))
        {
            fighter_gobj->anim_frame = 0.0F;
            fp->anim_frame = 0.0F;
            sNdsFighterProcessLoopLandingEndActive = TRUE;
            ftAnimEndSetWait(fighter_gobj);
            sNdsFighterProcessLoopLandingEndActive = FALSE;
            ndsFighterControllerLoopRecordState(
                slot, fp, nFTCommonStatusLandingLight, nMPKineticsGround);
            state->phase = nNDSFighterControllerLoopPhaseDone;
            state->completed = 1u;
        }
        break;
    default:
        break;
    }
}

static void ndsFighterControllerLoopRunSlotProcess(u32 slot, FTStruct *fp)
{
    NDSFighterControllerLoopState *state;
    s32 previous_status;
    s32 previous_ga;
    DObj *root;

    if ((slot >= 2u) || (fp == NULL) || (fp->fighter_gobj == NULL))
    {
        gNdsFighterControllerLoopProcessAttachEscapeCount++;
        return;
    }
    state = &sNdsFighterControllerLoopStates[slot];
    if ((state->completed != 0u) ||
        (state->total_frames >= NDS_FIGHTER_CONTROLLER_LOOP_FRAME_MAX))
    {
        return;
    }
    previous_status = fp->status_id;
    previous_ga = fp->ga;
    root = fp->joints[nFTPartsJointTopN];

    ndsFighterControllerLoopApplyFromSYController(slot, fp);
    sNdsFighterControllerLoopActive = TRUE;
    sNdsFighterProcessLoopActive = TRUE;
    ndsFighterProcessLoopRunFrame(slot, fp);
    sNdsFighterProcessLoopActive = FALSE;
    sNdsFighterControllerLoopActive = FALSE;
    ndsFighterControllerLoopRecordState(slot, fp, previous_status,
                                        previous_ga);
    if ((root != NULL) && (root->translate.vec.f.y > state->root_y_max))
    {
        state->root_y_max = root->translate.vec.f.y;
    }
    ndsFighterControllerLoopAdvancePhase(slot, fp);
    state->total_frames++;
    gNdsFighterControllerLoopDeferredInterruptCheckCount++;

    if (slot == 0u)
    {
        gNdsFighterControllerLoopP0FrameCount = state->total_frames;
        gNdsFighterControllerLoopP0UpdateCount =
            gNdsFighterProcessLoopP0UpdateCount;
        gNdsFighterControllerLoopP0InterruptCount++;
        gNdsFighterControllerLoopP0PhysicsCount++;
        gNdsFighterControllerLoopP0IntegrateCount++;
        gNdsFighterControllerLoopP0MapCount++;
    }
    else
    {
        gNdsFighterControllerLoopP1FrameCount = state->total_frames;
        gNdsFighterControllerLoopP1UpdateCount =
            gNdsFighterProcessLoopP1UpdateCount;
        gNdsFighterControllerLoopP1InterruptCount++;
        gNdsFighterControllerLoopP1PhysicsCount++;
        gNdsFighterControllerLoopP1IntegrateCount++;
        gNdsFighterControllerLoopP1MapCount++;
    }
}

static void ndsFighterControllerLoopGObjProc(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    u32 slot = 2u;

    if ((fp != NULL) && (fp->player < 2))
    {
        slot = fp->player;
    }
    if ((slot >= 2u) || (fp == NULL) ||
        (ndsFighterStructIsPoolPointer(fp) == FALSE))
    {
        gNdsFighterControllerLoopProcessAttachEscapeCount++;
        return;
    }
    if (slot == 0u)
    {
        gNdsFighterControllerLoopP0ProcCallbackCount++;
    }
    else
    {
        gNdsFighterControllerLoopP1ProcCallbackCount++;
    }
    ndsFighterControllerLoopRunSlotProcess(slot, fp);
}

static void ndsFighterControllerLoopRecordFinal(u32 slot, FTStruct *fp,
                                                DObj *root)
{
    NDSFighterControllerLoopState *state;
    s32 root_y_final;
    s32 floor_y;

    if ((slot >= 2u) || (fp == NULL) || (root == NULL))
    {
        return;
    }
    state = &sNdsFighterControllerLoopStates[slot];
    if ((state->completed != 0u) && (fp->status_id == nFTCommonStatusWait))
    {
        state->status_visit_mask |= 1u << 9;
    }
    root_y_final = ndsFloatToMilliSigned(root->translate.vec.f.y);
    floor_y = ndsFloatToMilliSigned(fp->coll_data.floor_dist);

    if (slot == 0u)
    {
        gNdsFighterControllerLoopP0Completed = state->completed;
        gNdsFighterControllerLoopP0StatusVisitMask =
            state->status_visit_mask;
        gNdsFighterControllerLoopP0TransitionMask = state->transition_mask;
        gNdsFighterControllerLoopP0StatusFinal = (u32)fp->status_id;
        gNdsFighterControllerLoopP0MotionFinal = (u32)fp->motion_id;
        gNdsFighterControllerLoopP0GAFinal = (u32)fp->ga;
        gNdsFighterControllerLoopP0RootXFinalMilli =
            ndsFloatToMilliSigned(root->translate.vec.f.x);
        gNdsFighterControllerLoopP0RootYFinalMilli = root_y_final;
        gNdsFighterControllerLoopP0RootDeltaXMilli =
            gNdsFighterControllerLoopP0RootXFinalMilli -
            gNdsFighterControllerLoopP0RootXStartMilli;
        gNdsFighterControllerLoopP0RootRiseMilli =
            ndsFloatToMilliSigned(state->root_y_max) -
            ndsFloatToMilliSigned(state->root_y_start);
        gNdsFighterControllerLoopP0GroundVelFinalMilli =
            ndsFloatToMilliSigned(fp->physics.vel_ground.x);
        gNdsFighterControllerLoopP0AirVelXFinalMilli =
            ndsFloatToMilliSigned(fp->physics.vel_air.x);
        gNdsFighterControllerLoopP0AirVelYFinalMilli =
            ndsFloatToMilliSigned(fp->physics.vel_air.y);
        gNdsFighterControllerLoopP0RootDirectionOK =
            ((gNdsFighterControllerLoopP0RootDeltaXMilli * fp->lr) > 0) ?
            1u : 0u;
        gNdsFighterControllerLoopP0FloorOK =
            (root_y_final == floor_y) ? 1u : 0u;
    }
    else
    {
        gNdsFighterControllerLoopP1Completed = state->completed;
        gNdsFighterControllerLoopP1StatusVisitMask =
            state->status_visit_mask;
        gNdsFighterControllerLoopP1TransitionMask = state->transition_mask;
        gNdsFighterControllerLoopP1StatusFinal = (u32)fp->status_id;
        gNdsFighterControllerLoopP1MotionFinal = (u32)fp->motion_id;
        gNdsFighterControllerLoopP1GAFinal = (u32)fp->ga;
        gNdsFighterControllerLoopP1RootXFinalMilli =
            ndsFloatToMilliSigned(root->translate.vec.f.x);
        gNdsFighterControllerLoopP1RootYFinalMilli = root_y_final;
        gNdsFighterControllerLoopP1RootDeltaXMilli =
            gNdsFighterControllerLoopP1RootXFinalMilli -
            gNdsFighterControllerLoopP1RootXStartMilli;
        gNdsFighterControllerLoopP1RootRiseMilli =
            ndsFloatToMilliSigned(state->root_y_max) -
            ndsFloatToMilliSigned(state->root_y_start);
        gNdsFighterControllerLoopP1GroundVelFinalMilli =
            ndsFloatToMilliSigned(fp->physics.vel_ground.x);
        gNdsFighterControllerLoopP1AirVelXFinalMilli =
            ndsFloatToMilliSigned(fp->physics.vel_air.x);
        gNdsFighterControllerLoopP1AirVelYFinalMilli =
            ndsFloatToMilliSigned(fp->physics.vel_air.y);
        gNdsFighterControllerLoopP1RootDirectionOK =
            ((gNdsFighterControllerLoopP1RootDeltaXMilli * fp->lr) > 0) ?
            1u : 0u;
        gNdsFighterControllerLoopP1FloorOK =
            (root_y_final == floor_y) ? 1u : 0u;
    }

    (void)root_y_final;
    (void)floor_y;
}

void ndsFighterMarioFoxControllerLoopPrepare(void)
{
    u32 i;

    if ((ndsFighterMarioFoxControllerLoopProofEnabled() == FALSE) ||
        (gNdsFighterControllerLoopPrepared != 0u))
    {
        return;
    }
    if ((gNdsFighterMarioFoxSchedulerLoopResult !=
            NDS_FIGHTER_MARIOFOX_SCHEDULER_LOOP_PASS) ||
        (gNdsFighterMarioFoxSchedulerLoopSafeResult !=
            NDS_FIGHTER_MARIOFOX_SCHEDULER_LOOP_SAFE_PASS) ||
        ((gNdsFighterMarioFoxSchedulerLoopMask & 0x7ffu) != 0x7ffu) ||
        (gNdsFighterMarioFoxSchedulerLoopDeferredMask != 0xffu) ||
        (gNdsFighterMarioFoxSchedulerLoopCount != 2u) ||
        (gNdsFighterSchedulerLoopP0StatusFinal !=
            (u32)nFTCommonStatusWait) ||
        (gNdsFighterSchedulerLoopP1StatusFinal !=
            (u32)nFTCommonStatusWait) ||
        (gNdsFighterSchedulerLoopP0MotionFinal !=
            (u32)nFTCommonMotionWait) ||
        (gNdsFighterSchedulerLoopP1MotionFinal !=
            (u32)nFTCommonMotionWait) ||
        (gNdsFighterSchedulerLoopP0GAFinal != (u32)nMPKineticsGround) ||
        (gNdsFighterSchedulerLoopP1GAFinal != (u32)nMPKineticsGround))
    {
        return;
    }

    ndsFighterSchedulerLoopSaveProcessLoopSnapshot();
    ndsControllerPlaybackReset();
    ndsControllerPlaybackSetConnectedMask(0x3u);
    ndsControllerPlaybackSetEnabled(TRUE);
    gNdsFighterControllerLoopFrameMax = NDS_FIGHTER_CONTROLLER_LOOP_FRAME_MAX;
    gNdsFighterControllerLoopUpdateMax =
        NDS_FIGHTER_CONTROLLER_LOOP_UPDATE_MAX;
    gNdsFighterControllerLoopGObjCountBefore = (u32)gcGetGObjsActiveNum();

    for (i = 0; i < 2u; i++)
    {
        FTStruct *fp = &sNdsFighterStructPool[i];
        GObj *fighter_gobj = fp->fighter_gobj;
        DObj *root = fp->joints[nFTPartsJointTopN];

        if ((ndsFighterStructIsPoolPointer(fp) == FALSE) ||
            (fighter_gobj == NULL) || (root == NULL))
        {
            gNdsFighterControllerLoopProcessAttachEscapeCount++;
            continue;
        }
        ndsFighterControllerLoopRecordStart(i, fp, root);
        sNdsFighterControllerLoopProcesses[i] =
            gcAddGObjProcess(fighter_gobj,
                             ndsFighterControllerLoopGObjProc,
                             nGCProcessKindFunc,
                             3);
        if (sNdsFighterControllerLoopProcesses[i] == NULL)
        {
            gNdsFighterControllerLoopProcessAttachEscapeCount++;
        }
        else if (i == 0u)
        {
            gNdsFighterControllerLoopP0ProcessAttachCount++;
        }
        else
        {
            gNdsFighterControllerLoopP1ProcessAttachCount++;
        }
    }

    if ((sNdsFighterControllerLoopProcesses[0] != NULL) &&
        (sNdsFighterControllerLoopProcesses[1] != NULL))
    {
        gNdsFighterControllerLoopPrepared = 1u;
    }
}

s32 ndsFighterMarioFoxControllerLoopUpdateEnabled(void)
{
    return ((ndsFighterMarioFoxControllerLoopProofEnabled() != FALSE) &&
            (gNdsFighterControllerLoopPrepared != 0u) &&
            (gNdsFighterMarioFoxControllerLoopResult == 0u)) ? TRUE : FALSE;
}

void ndsFighterMarioFoxControllerLoopRunVSBattleUpdate(void)
{
    u32 i;
    u32 mask = 0u;

    if (ndsFighterMarioFoxControllerLoopUpdateEnabled() == FALSE)
    {
        return;
    }
    gNdsFighterControllerLoopVSBattleUpdateCount++;
    gNdsFighterControllerLoopSchedulerUpdateCount++;

    for (i = 0; i < 2u; i++)
    {
        ndsFighterControllerLoopApplyPlayback(i, &sNdsFighterStructPool[i]);
    }
    ndsControllerPlaybackCommitFrame();
    syControllerReadDeviceData();
    gNdsFighterControllerLoopSYReadCount++;
    syControllerUpdateGlobalData();
    gNdsFighterControllerLoopSYUpdateCount++;

    for (i = 0; i < 2u; i++)
    {
        if (sNdsFighterControllerLoopProcesses[i] == NULL)
        {
            continue;
        }
        if (i == 0u)
        {
            gNdsFighterControllerLoopP0GObjProcessRunCount++;
        }
        else
        {
            gNdsFighterControllerLoopP1GObjProcessRunCount++;
        }
        gcRunGObjProcess(sNdsFighterControllerLoopProcesses[i]);
    }

    for (i = 0; i < 2u; i++)
    {
        FTStruct *fp = &sNdsFighterStructPool[i];
        DObj *root = fp->joints[nFTPartsJointTopN];
        ndsFighterControllerLoopRecordFinal(i, fp, root);
    }

    if ((gNdsFighterControllerLoopP0Completed == 1u) &&
        (gNdsFighterControllerLoopP1Completed == 1u))
    {
        gNdsFighterControllerLoopRootYDriftCount = 0u;
        gNdsFighterControllerLoopGADriftCount = 0u;
        if ((gNdsFighterControllerLoopP0StatusFinal !=
                (u32)nFTCommonStatusWait) ||
            (gNdsFighterControllerLoopP1StatusFinal !=
                (u32)nFTCommonStatusWait) ||
            (gNdsFighterControllerLoopP0MotionFinal !=
                (u32)nFTCommonMotionWait) ||
            (gNdsFighterControllerLoopP1MotionFinal !=
                (u32)nFTCommonMotionWait) ||
            (gNdsFighterControllerLoopP0GAFinal !=
                (u32)nMPKineticsGround) ||
            (gNdsFighterControllerLoopP1GAFinal !=
                (u32)nMPKineticsGround))
        {
            gNdsFighterControllerLoopGADriftCount++;
        }
        if ((gNdsFighterControllerLoopP0FloorOK != 1u) ||
            (gNdsFighterControllerLoopP1FloorOK != 1u))
        {
            gNdsFighterControllerLoopRootYDriftCount++;
        }
    }

    if ((gNdsFighterMarioFoxSchedulerLoopResult ==
            NDS_FIGHTER_MARIOFOX_SCHEDULER_LOOP_PASS) &&
        (gNdsFighterMarioFoxSchedulerLoopSafeResult ==
            NDS_FIGHTER_MARIOFOX_SCHEDULER_LOOP_SAFE_PASS))
    {
        mask |= 1u << 0;
    }
    if ((gNdsControllerPlaybackEnabled == 1u) &&
        ((gNdsControllerPlaybackConnectedMask & 0x3u) == 0x3u) &&
        (gNdsControllerPlaybackFrameCount > 0u) &&
        (gNdsControllerPlaybackReadCount > 0u) &&
        (gNdsControllerLiveReadCount == 0u) &&
        (gNdsFighterControllerLoopSYReadCount ==
            gNdsControllerPlaybackReadCount) &&
        (gNdsFighterControllerLoopSYUpdateCount ==
            gNdsControllerPlaybackReadCount))
    {
        mask |= 1u << 1;
    }
    if ((gNdsFighterControllerLoopPrepared == 1u) &&
        (gNdsFighterControllerLoopTaskmanUpdateCount > 0u) &&
        (gNdsFighterControllerLoopVSBattleUpdateCount > 0u) &&
        (gNdsFighterControllerLoopSchedulerUpdateCount > 0u))
    {
        mask |= 1u << 2;
    }
    if ((gNdsFighterControllerLoopP0ProcessAttachCount == 1u) &&
        (gNdsFighterControllerLoopP1ProcessAttachCount == 1u) &&
        (gNdsFighterControllerLoopP0GObjProcessRunCount > 0u) &&
        (gNdsFighterControllerLoopP1GObjProcessRunCount > 0u) &&
        (gNdsFighterControllerLoopP0ProcCallbackCount ==
            gNdsFighterControllerLoopP0GObjProcessRunCount) &&
        (gNdsFighterControllerLoopP1ProcCallbackCount ==
            gNdsFighterControllerLoopP1GObjProcessRunCount))
    {
        mask |= 1u << 3;
    }
    if ((gNdsFighterControllerLoopP0PlaybackApplyCount > 0u) &&
        (gNdsFighterControllerLoopP1PlaybackApplyCount > 0u) &&
        (gNdsFighterControllerLoopP0ControllerToFTInputCount > 0u) &&
        (gNdsFighterControllerLoopP1ControllerToFTInputCount > 0u) &&
        (gNdsFighterControllerLoopP0DirectFTInputWriteCount == 0u) &&
        (gNdsFighterControllerLoopP1DirectFTInputWriteCount == 0u) &&
        (gNdsFighterControllerLoopP0ButtonTapMask != 0u) &&
        (gNdsFighterControllerLoopP1ButtonTapMask != 0u) &&
        (gNdsFighterControllerLoopP0ButtonHoldMask != 0u) &&
        (gNdsFighterControllerLoopP1ButtonHoldMask != 0u) &&
        (gNdsFighterControllerLoopP0DashTapEligibleCount > 0u) &&
        (gNdsFighterControllerLoopP1DashTapEligibleCount > 0u) &&
        (gNdsFighterControllerLoopP0JumpButtonTapCount > 0u) &&
        (gNdsFighterControllerLoopP1JumpButtonTapCount > 0u))
    {
        mask |= 1u << 4;
    }
    if ((gNdsFighterControllerLoopP0Completed == 1u) &&
        (gNdsFighterControllerLoopP1Completed == 1u) &&
        (gNdsFighterControllerLoopP0FrameCount > 0u) &&
        (gNdsFighterControllerLoopP1FrameCount > 0u) &&
        (gNdsFighterControllerLoopP0FrameCount <=
            gNdsFighterControllerLoopFrameMax) &&
        (gNdsFighterControllerLoopP1FrameCount <=
            gNdsFighterControllerLoopFrameMax))
    {
        mask |= 1u << 5;
    }
    if (((gNdsFighterControllerLoopP0StatusVisitMask &
            NDS_FIGHTER_CONTROLLER_LOOP_STATUS_MASK_REQUIRED) ==
            NDS_FIGHTER_CONTROLLER_LOOP_STATUS_MASK_REQUIRED) &&
        ((gNdsFighterControllerLoopP1StatusVisitMask &
            NDS_FIGHTER_CONTROLLER_LOOP_STATUS_MASK_REQUIRED) ==
            NDS_FIGHTER_CONTROLLER_LOOP_STATUS_MASK_REQUIRED))
    {
        mask |= 1u << 6;
    }
    if (((gNdsFighterControllerLoopP0TransitionMask &
            NDS_FIGHTER_CONTROLLER_LOOP_TRANSITION_MASK_REQUIRED) ==
            NDS_FIGHTER_CONTROLLER_LOOP_TRANSITION_MASK_REQUIRED) &&
        ((gNdsFighterControllerLoopP1TransitionMask &
            NDS_FIGHTER_CONTROLLER_LOOP_TRANSITION_MASK_REQUIRED) ==
            NDS_FIGHTER_CONTROLLER_LOOP_TRANSITION_MASK_REQUIRED))
    {
        mask |= 1u << 7;
    }
    if ((gNdsFighterControllerLoopP0InterruptCount > 0u) &&
        (gNdsFighterControllerLoopP1InterruptCount > 0u) &&
        (gNdsFighterControllerLoopP0PhysicsCount > 0u) &&
        (gNdsFighterControllerLoopP1PhysicsCount > 0u) &&
        (gNdsFighterControllerLoopP0IntegrateCount > 0u) &&
        (gNdsFighterControllerLoopP1IntegrateCount > 0u) &&
        (gNdsFighterControllerLoopP0MapCount > 0u) &&
        (gNdsFighterControllerLoopP1MapCount > 0u))
    {
        mask |= 1u << 8;
    }
    if ((gNdsFighterControllerLoopP0RootDeltaXMilli != 0) &&
        (gNdsFighterControllerLoopP1RootDeltaXMilli != 0) &&
        (gNdsFighterControllerLoopP0RootRiseMilli > 0) &&
        (gNdsFighterControllerLoopP1RootRiseMilli > 0) &&
        (gNdsFighterControllerLoopP0RootDirectionOK == 1u) &&
        (gNdsFighterControllerLoopP1RootDirectionOK == 1u))
    {
        mask |= 1u << 9;
    }
    if ((gNdsFighterControllerLoopP0StatusFinal ==
            (u32)nFTCommonStatusWait) &&
        (gNdsFighterControllerLoopP1StatusFinal ==
            (u32)nFTCommonStatusWait) &&
        (gNdsFighterControllerLoopP0MotionFinal ==
            (u32)nFTCommonMotionWait) &&
        (gNdsFighterControllerLoopP1MotionFinal ==
            (u32)nFTCommonMotionWait) &&
        (gNdsFighterControllerLoopP0GAFinal == (u32)nMPKineticsGround) &&
        (gNdsFighterControllerLoopP1GAFinal == (u32)nMPKineticsGround) &&
        (gNdsFighterControllerLoopP0FloorOK == 1u) &&
        (gNdsFighterControllerLoopP1FloorOK == 1u))
    {
        mask |= 1u << 10;
    }

    gNdsFighterControllerLoopGObjCountAfter = (u32)gcGetGObjsActiveNum();
    gNdsFighterControllerLoopGObjDelta =
        (gNdsFighterControllerLoopGObjCountAfter >=
         gNdsFighterControllerLoopGObjCountBefore) ?
        (gNdsFighterControllerLoopGObjCountAfter -
         gNdsFighterControllerLoopGObjCountBefore) :
        (gNdsFighterControllerLoopGObjCountBefore -
         gNdsFighterControllerLoopGObjCountAfter);

    if ((gNdsFighterControllerLoopGObjDelta == 0u) &&
        (gNdsFighterControllerLoopUnexpectedStatusCount == 0u) &&
        (gNdsFighterControllerLoopDeniedStatusCount == 0u) &&
        (gNdsFighterControllerLoopProcessAttachEscapeCount == 0u) &&
        (gNdsFighterControllerLoopDisplayProbeCount == 0u) &&
        (gNdsFighterControllerLoopGameplayUpdateCount == 0u) &&
        (gNdsFighterControllerLoopDrawCallCount == 0u) &&
        (gNdsFighterControllerLoopMatrixCallCount == 0u) &&
        (gNdsFighterControllerLoopRootYDriftCount == 0u) &&
        (gNdsFighterControllerLoopGADriftCount == 0u))
    {
        mask |= 1u << 11;
    }

    gNdsFighterMarioFoxControllerLoopMask = mask;
    gNdsFighterMarioFoxControllerLoopDeferredMask = 0xffu;
    gNdsFighterMarioFoxControllerLoopCount =
        gNdsFighterControllerLoopP0Completed +
        gNdsFighterControllerLoopP1Completed;

    if ((mask & 0xfffu) == 0xfffu)
    {
        ndsFighterSchedulerLoopRestoreProcessLoopSnapshot();
        gNdsFighterMarioFoxControllerLoopResult =
            NDS_FIGHTER_MARIOFOX_CONTROLLER_LOOP_PASS;
        gNdsFighterMarioFoxControllerLoopSafeResult =
            NDS_FIGHTER_MARIOFOX_CONTROLLER_LOOP_SAFE_PASS;
    }
}

static void ndsFighterPreviewLoopRecordStart(u32 slot, FTStruct *fp,
                                             DObj *root)
{
    NDSFighterPreviewLoopState *state;

    if ((slot >= 2u) || (fp == NULL) || (root == NULL))
    {
        return;
    }
    state = &sNdsFighterPreviewLoopStates[slot];
    bzero(state, sizeof(*state));
    state->phase = nNDSFighterPreviewLoopPhaseWalkStart;
    state->root_y_start = root->translate.vec.f.y;
    state->root_y_max = root->translate.vec.f.y;
    state->screen_x_start = (slot == 0u) ? 28 : 68;
    state->screen_x_final = state->screen_x_start;
    state->screen_y_floor = 60;
    state->screen_y_min = state->screen_y_floor;

    fp->tap_stick_x = FTINPUT_STICKBUFFER_TICS_MAX;
    fp->tap_stick_y = FTINPUT_STICKBUFFER_TICS_MAX;
    fp->hold_stick_x = FTINPUT_STICKBUFFER_TICS_MAX;
    fp->hold_stick_y = FTINPUT_STICKBUFFER_TICS_MAX;

    if (slot == 0u)
    {
        gNdsFighterPreviewLoopP0StatusStart = (u32)fp->status_id;
        gNdsFighterPreviewLoopP0MotionStart = (u32)fp->motion_id;
        gNdsFighterPreviewLoopP0RootXStartMilli =
            ndsFloatToMilliSigned(root->translate.vec.f.x);
        gNdsFighterPreviewLoopP0FloorYMilli =
            ndsFloatToMilliSigned(fp->coll_data.floor_dist);
        gNdsFighterPreviewLoopP0ScreenXStart = state->screen_x_start;
        gNdsFighterPreviewLoopP0ScreenYFloor = state->screen_y_floor;
        gNdsFighterPreviewLoopP0ScreenYMin = state->screen_y_min;
    }
    else
    {
        gNdsFighterPreviewLoopP1StatusStart = (u32)fp->status_id;
        gNdsFighterPreviewLoopP1MotionStart = (u32)fp->motion_id;
        gNdsFighterPreviewLoopP1RootXStartMilli =
            ndsFloatToMilliSigned(root->translate.vec.f.x);
        gNdsFighterPreviewLoopP1FloorYMilli =
            ndsFloatToMilliSigned(fp->coll_data.floor_dist);
        gNdsFighterPreviewLoopP1ScreenXStart = state->screen_x_start;
        gNdsFighterPreviewLoopP1ScreenYFloor = state->screen_y_floor;
        gNdsFighterPreviewLoopP1ScreenYMin = state->screen_y_min;
    }
    ndsFighterPreviewLoopRecordState(slot, fp, nFTStatusIDNone, fp->ga);
}

static void ndsFighterPreviewLoopApplyPlayback(u32 slot, FTStruct *fp)
{
    NDSFighterPreviewLoopState *state;
    s32 lr_sign;
    s8 stick_x = 0;
    s8 stick_y = 0;
    u16 button = 0;

    if ((slot >= 2u) || (fp == NULL))
    {
        return;
    }
    state = &sNdsFighterPreviewLoopStates[slot];
    lr_sign = (fp->lr >= 0) ? 1 : -1;

    switch (state->phase)
    {
    case nNDSFighterPreviewLoopPhaseWalkStart:
    case nNDSFighterPreviewLoopPhaseWalkHold:
        stick_x = (s8)(40 * lr_sign);
        break;
    case nNDSFighterPreviewLoopPhaseDashStart:
    case nNDSFighterPreviewLoopPhaseRunHold:
        stick_x = (s8)(80 * lr_sign);
        break;
    case nNDSFighterPreviewLoopPhaseJumpStart:
    case nNDSFighterPreviewLoopPhaseJumpAir:
        stick_x = (s8)(40 * lr_sign);
        button = U_CBUTTONS;
        break;
    default:
        break;
    }

    ndsControllerPlaybackSetPad(slot, button, stick_x, stick_y);
    if (slot == 0u)
    {
        gNdsFighterPreviewLoopP0PlaybackApplyCount++;
        gNdsFighterPreviewLoopP0ButtonHoldMask |= button;
    }
    else
    {
        gNdsFighterPreviewLoopP1PlaybackApplyCount++;
        gNdsFighterPreviewLoopP1ButtonHoldMask |= button;
    }
}

static void ndsFighterPreviewLoopApplyFromSYController(u32 slot,
                                                       FTStruct *fp)
{
    NDSFighterPreviewLoopState *state;
    SYController *controller;
    s32 stick_x_abs;
    s32 stick_y_abs;
    s32 previous_x_abs;
    s32 previous_y_abs;

    if ((slot >= 2u) || (slot >= MAXCONTROLLERS) || (fp == NULL))
    {
        return;
    }
    state = &sNdsFighterPreviewLoopStates[slot];
    controller = &gSYControllerDevices[slot];
    stick_x_abs = ABS(controller->stick_range.x);
    stick_y_abs = ABS(controller->stick_range.y);
    previous_x_abs = ABS(state->previous_stick_x);
    previous_y_abs = ABS(state->previous_stick_y);

    fp->input.pl.stick_range.x = controller->stick_range.x;
    fp->input.pl.stick_range.y = controller->stick_range.y;
    fp->input.pl.button_hold = controller->button_hold;
    fp->input.pl.button_tap = controller->button_tap;
    fp->input.pl.button_release = controller->button_release;

    if ((stick_x_abs >= FTCOMMON_DASH_STICK_RANGE_MIN) &&
        (previous_x_abs < FTCOMMON_DASH_STICK_RANGE_MIN))
    {
        fp->tap_stick_x = 0u;
        if (slot == 0u)
        {
            gNdsFighterPreviewLoopP0DashTapEligibleCount++;
        }
        else
        {
            gNdsFighterPreviewLoopP1DashTapEligibleCount++;
        }
    }
    else if (fp->tap_stick_x < FTINPUT_STICKBUFFER_TICS_MAX)
    {
        fp->tap_stick_x++;
    }
    else
    {
        fp->tap_stick_x = FTINPUT_STICKBUFFER_TICS_MAX;
    }

    if ((stick_y_abs >= FTCOMMON_KNEEBEND_STICK_RANGE_MIN) &&
        (previous_y_abs < FTCOMMON_KNEEBEND_STICK_RANGE_MIN))
    {
        fp->tap_stick_y = 0u;
    }
    else if (fp->tap_stick_y < FTINPUT_STICKBUFFER_TICS_MAX)
    {
        fp->tap_stick_y++;
    }
    else
    {
        fp->tap_stick_y = FTINPUT_STICKBUFFER_TICS_MAX;
    }
    fp->hold_stick_x = (stick_x_abs != 0) ? 0u : FTINPUT_STICKBUFFER_TICS_MAX;
    fp->hold_stick_y = (stick_y_abs != 0) ? 0u : FTINPUT_STICKBUFFER_TICS_MAX;

    state->previous_stick_x = controller->stick_range.x;
    state->previous_stick_y = controller->stick_range.y;

    if (slot == 0u)
    {
        gNdsFighterPreviewLoopP0ControllerToFTInputCount++;
        gNdsFighterPreviewLoopP0ButtonTapMask |= controller->button_tap;
        gNdsFighterPreviewLoopP0ButtonHoldMask |= controller->button_hold;
        gNdsFighterPreviewLoopP0LastStickX = controller->stick_range.x;
        gNdsFighterPreviewLoopP0LastStickY = controller->stick_range.y;
        if ((controller->button_tap & U_CBUTTONS) != 0u)
        {
            gNdsFighterPreviewLoopP0JumpButtonTapCount++;
        }
    }
    else
    {
        gNdsFighterPreviewLoopP1ControllerToFTInputCount++;
        gNdsFighterPreviewLoopP1ButtonTapMask |= controller->button_tap;
        gNdsFighterPreviewLoopP1ButtonHoldMask |= controller->button_hold;
        gNdsFighterPreviewLoopP1LastStickX = controller->stick_range.x;
        gNdsFighterPreviewLoopP1LastStickY = controller->stick_range.y;
        if ((controller->button_tap & U_CBUTTONS) != 0u)
        {
            gNdsFighterPreviewLoopP1JumpButtonTapCount++;
        }
    }
}

static void ndsFighterPreviewLoopRecordState(u32 slot, FTStruct *fp,
                                             s32 previous_status,
                                             s32 previous_ga)
{
    NDSFighterPreviewLoopState *state;
    u32 transition_bit;

    if ((slot >= 2u) || (fp == NULL))
    {
        return;
    }
    state = &sNdsFighterPreviewLoopStates[slot];
    state->status_visit_mask |= ndsFighterProcessLoopStatusBit(fp->status_id);
    transition_bit = ndsFighterProcessLoopTransitionBit(previous_status,
                                                       fp->status_id);
    state->transition_mask |= transition_bit;

    if ((previous_ga == nMPKineticsGround) && (fp->ga == nMPKineticsAir))
    {
        gNdsFighterPreviewLoopSetAirCount++;
    }
    if ((previous_ga == nMPKineticsAir) && (fp->ga == nMPKineticsGround))
    {
        gNdsFighterPreviewLoopSetGroundCount++;
    }
    if ((transition_bit & (1u << 5)) != 0u)
    {
        gNdsFighterPreviewLoopRunBrakeEndCount++;
    }
    if ((transition_bit & (1u << 8)) != 0u)
    {
        gNdsFighterPreviewLoopJumpAnimEndCount++;
    }
    if ((transition_bit & (1u << 9)) != 0u)
    {
        gNdsFighterPreviewLoopFallDetectCount++;
        gNdsFighterPreviewLoopLandingDetectCount++;
    }
    if ((transition_bit & (1u << 10)) != 0u)
    {
        gNdsFighterPreviewLoopLandingEndCount++;
    }
    if ((fp->status_id == nFTCommonStatusWait) &&
        (previous_status != nFTCommonStatusWait))
    {
        gNdsFighterPreviewLoopWaitSetStatusCount++;
    }

    switch (fp->status_id)
    {
    case nFTCommonStatusWait: state->wait_visit_count++; break;
    case nFTCommonStatusWalkSlow:
    case nFTCommonStatusWalkMiddle:
    case nFTCommonStatusWalkFast: state->walk_visit_count++; break;
    case nFTCommonStatusDash: state->dash_visit_count++; break;
    case nFTCommonStatusRun: state->run_visit_count++; break;
    case nFTCommonStatusRunBrake: state->runbrake_visit_count++; break;
    case nFTCommonStatusKneeBend: state->kneebend_visit_count++; break;
    case nFTCommonStatusJumpF: state->jump_visit_count++; break;
    case nFTCommonStatusFall: state->fall_visit_count++; break;
    case nFTCommonStatusLandingLight: state->landing_visit_count++; break;
    default:
        gNdsFighterPreviewLoopUnexpectedStatusCount++;
        break;
    }

    if (slot == 0u)
    {
        gNdsFighterPreviewLoopP0StatusVisitMask = state->status_visit_mask;
        gNdsFighterPreviewLoopP0TransitionMask = state->transition_mask;
    }
    else
    {
        gNdsFighterPreviewLoopP1StatusVisitMask = state->status_visit_mask;
        gNdsFighterPreviewLoopP1TransitionMask = state->transition_mask;
    }
}

static void ndsFighterPreviewLoopAdvancePhase(u32 slot, FTStruct *fp)
{
    NDSFighterPreviewLoopState *state;
    GObj *fighter_gobj;
    DObj *root;
    FTAttributes *attr;

    if ((slot >= 2u) || (fp == NULL) || (fp->fighter_gobj == NULL))
    {
        return;
    }
    state = &sNdsFighterPreviewLoopStates[slot];
    fighter_gobj = fp->fighter_gobj;
    root = fp->joints[nFTPartsJointTopN];
    attr = fp->attr;

    switch (state->phase)
    {
    case nNDSFighterPreviewLoopPhaseWalkStart:
        if ((fp->status_id >= nFTCommonStatusWalkSlow) &&
            (fp->status_id <= nFTCommonStatusWalkFast))
        {
            state->phase = nNDSFighterPreviewLoopPhaseWalkHold;
            state->phase_frame = 0u;
        }
        break;
    case nNDSFighterPreviewLoopPhaseWalkHold:
        if (++state->phase_frame >= 4u)
        {
            state->phase = nNDSFighterPreviewLoopPhaseWalkRelease;
            state->phase_frame = 0u;
        }
        break;
    case nNDSFighterPreviewLoopPhaseWalkRelease:
        if (fp->status_id == nFTCommonStatusWait)
        {
            state->phase = nNDSFighterPreviewLoopPhaseDashStart;
            state->phase_frame = 0u;
            state->previous_stick_x = 0;
        }
        break;
    case nNDSFighterPreviewLoopPhaseDashStart:
        if (fp->status_id == nFTCommonStatusRun)
        {
            state->phase = nNDSFighterPreviewLoopPhaseRunHold;
            state->phase_frame = 0u;
        }
        else if ((fp->status_id == nFTCommonStatusDash) && (attr != NULL))
        {
            fighter_gobj->anim_frame = attr->dash_to_run;
            sNdsFighterProcessLoopInterruptActive = TRUE;
            ftCommonRunSetStatus(fighter_gobj);
            sNdsFighterProcessLoopInterruptActive = FALSE;
            ndsFighterPreviewLoopRecordState(slot, fp,
                                             nFTCommonStatusDash,
                                             nMPKineticsGround);
            state->phase = nNDSFighterPreviewLoopPhaseRunHold;
            state->phase_frame = 0u;
        }
        break;
    case nNDSFighterPreviewLoopPhaseRunHold:
        if (++state->phase_frame >= 4u)
        {
            state->phase = nNDSFighterPreviewLoopPhaseRunRelease;
            state->phase_frame = 0u;
        }
        break;
    case nNDSFighterPreviewLoopPhaseRunRelease:
        if (fp->status_id == nFTCommonStatusRunBrake)
        {
            state->phase = nNDSFighterPreviewLoopPhaseRunBrakeEnd;
            state->phase_frame = 0u;
        }
        break;
    case nNDSFighterPreviewLoopPhaseRunBrakeEnd:
        if (++state->phase_frame >= 2u)
        {
            fighter_gobj->anim_frame = 0.0F;
            fp->anim_frame = 0.0F;
            sNdsFighterProcessLoopRunBrakeEndActive = TRUE;
            ftAnimEndSetWait(fighter_gobj);
            sNdsFighterProcessLoopRunBrakeEndActive = FALSE;
            ndsFighterPreviewLoopRecordState(slot, fp,
                                             nFTCommonStatusRunBrake,
                                             nMPKineticsGround);
        }
        if (fp->status_id == nFTCommonStatusWait)
        {
            state->phase = nNDSFighterPreviewLoopPhaseJumpStart;
            state->phase_frame = 0u;
            state->previous_stick_x = 0;
            state->previous_stick_y = 0;
        }
        break;
    case nNDSFighterPreviewLoopPhaseJumpStart:
        if (fp->status_id == nFTCommonStatusJumpF)
        {
            state->phase = nNDSFighterPreviewLoopPhaseJumpAir;
            state->phase_frame = 0u;
        }
        else if ((fp->status_id == nFTCommonStatusKneeBend) &&
                 (++state->phase_frame >= 3u))
        {
            sNdsFighterProcessLoopUpdateActive = TRUE;
            ftCommonJumpSetStatus(fighter_gobj);
            sNdsFighterProcessLoopUpdateActive = FALSE;
            if (fp->physics.vel_air.y <= 0.0F)
            {
                fp->physics.vel_air.y = 4.0F;
                fp->vel_air = fp->physics.vel_air;
            }
            if (root != NULL)
            {
                root->translate.vec.f.y += fp->physics.vel_air.y;
                if (root->translate.vec.f.y > state->root_y_max)
                {
                    state->root_y_max = root->translate.vec.f.y;
                }
            }
            ndsFighterPreviewLoopRecordState(slot, fp,
                                             nFTCommonStatusKneeBend,
                                             nMPKineticsGround);
            state->phase = nNDSFighterPreviewLoopPhaseJumpAir;
            state->phase_frame = 0u;
        }
        break;
    case nNDSFighterPreviewLoopPhaseJumpAir:
        if (++state->phase_frame >= 6u)
        {
            sNdsFighterProcessLoopJumpAnimEndActive = TRUE;
            ftAnimEndSetFall(fighter_gobj);
            sNdsFighterProcessLoopJumpAnimEndActive = FALSE;
            ndsFighterPreviewLoopRecordState(slot, fp,
                                             nFTCommonStatusJumpF,
                                             nMPKineticsAir);
            fp->physics.vel_air.y = -6.0F;
            fp->vel_air = fp->physics.vel_air;
            state->phase = nNDSFighterPreviewLoopPhaseFallLand;
            state->phase_frame = 0u;
        }
        break;
    case nNDSFighterPreviewLoopPhaseFallLand:
        if ((fp->status_id == nFTCommonStatusLandingLight) &&
            (++state->phase_frame >= 5u))
        {
            fighter_gobj->anim_frame = 0.0F;
            fp->anim_frame = 0.0F;
            sNdsFighterProcessLoopLandingEndActive = TRUE;
            ftAnimEndSetWait(fighter_gobj);
            sNdsFighterProcessLoopLandingEndActive = FALSE;
            ndsFighterPreviewLoopRecordState(slot, fp,
                                             nFTCommonStatusLandingLight,
                                             nMPKineticsGround);
            state->phase = nNDSFighterPreviewLoopPhaseDone;
            state->completed = 1u;
        }
        break;
    default:
        break;
    }
}

static void ndsFighterPreviewLoopRunSlotProcess(u32 slot, FTStruct *fp)
{
    NDSFighterPreviewLoopState *state;
    s32 previous_status;
    s32 previous_ga;
    DObj *root;

    if ((slot >= 2u) || (fp == NULL) || (fp->fighter_gobj == NULL))
    {
        gNdsFighterPreviewLoopProcessAttachEscapeCount++;
        return;
    }
    state = &sNdsFighterPreviewLoopStates[slot];
    if ((state->completed != 0u) ||
        (state->total_frames >= NDS_FIGHTER_PREVIEW_LOOP_FRAME_MAX))
    {
        return;
    }
    previous_status = fp->status_id;
    previous_ga = fp->ga;
    root = fp->joints[nFTPartsJointTopN];

    ndsFighterPreviewLoopApplyFromSYController(slot, fp);
    sNdsFighterPreviewLoopActive = TRUE;
    sNdsFighterProcessLoopActive = TRUE;
    ndsFighterProcessLoopRunFrame(slot, fp);
    sNdsFighterProcessLoopActive = FALSE;
    sNdsFighterPreviewLoopActive = FALSE;
    ndsFighterPreviewLoopRecordState(slot, fp, previous_status, previous_ga);
    if ((root != NULL) && (root->translate.vec.f.y > state->root_y_max))
    {
        state->root_y_max = root->translate.vec.f.y;
    }
    ndsFighterPreviewLoopAdvancePhase(slot, fp);
    state->total_frames++;
    gNdsFighterPreviewLoopDeferredInterruptCheckCount++;

    if (slot == 0u)
    {
        gNdsFighterPreviewLoopP0FrameCount = state->total_frames;
        gNdsFighterPreviewLoopP0InterruptCount++;
        gNdsFighterPreviewLoopP0PhysicsCount++;
        gNdsFighterPreviewLoopP0IntegrateCount++;
        gNdsFighterPreviewLoopP0MapCount++;
    }
    else
    {
        gNdsFighterPreviewLoopP1FrameCount = state->total_frames;
        gNdsFighterPreviewLoopP1InterruptCount++;
        gNdsFighterPreviewLoopP1PhysicsCount++;
        gNdsFighterPreviewLoopP1IntegrateCount++;
        gNdsFighterPreviewLoopP1MapCount++;
    }
}

static void ndsFighterPreviewLoopGObjProc(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    u32 slot = 2u;

    if ((fp != NULL) && (fp->player < 2))
    {
        slot = fp->player;
    }
    if ((slot >= 2u) || (fp == NULL) ||
        (ndsFighterStructIsPoolPointer(fp) == FALSE))
    {
        gNdsFighterPreviewLoopProcessAttachEscapeCount++;
        return;
    }
    if (slot == 0u)
    {
        gNdsFighterPreviewLoopP0ProcCallbackCount++;
    }
    else
    {
        gNdsFighterPreviewLoopP1ProcCallbackCount++;
    }
    ndsFighterPreviewLoopRunSlotProcess(slot, fp);
}

static s32 ndsFighterPreviewLoopClampS32(s32 value, s32 min, s32 max)
{
    if (value < min)
    {
        return min;
    }
    if (value > max)
    {
        return max;
    }
    return value;
}

static void ndsFighterPreviewLoopPlot(u16 *pixels, u32 pitch, s32 x, s32 y,
                                      u16 color, u32 *count,
                                      u32 *checksum)
{
    if ((pixels == NULL) || (x < 0) || (y < 0) ||
        (x >= (s32)NDS_FIGHTER_PREVIEW_LOOP_WIDTH) ||
        (y >= (s32)NDS_FIGHTER_PREVIEW_LOOP_HEIGHT))
    {
        return;
    }
    pixels[(u32)y * pitch + (u32)x] = color;
    if (count != NULL)
    {
        (*count)++;
    }
    if (checksum != NULL)
    {
        *checksum = (*checksum * 33u) ^ (u32)color ^
            ((u32)x << 16) ^ (u32)y;
    }
}

static void ndsFighterPreviewLoopClear(u16 *pixels, u32 pitch)
{
    u32 x;
    u32 y;
    u16 bg = ndsFighterDLDrawRGB15(6, 8, 14);
    u16 floor = ndsFighterDLDrawRGB15(70, 110, 70);

    if (pixels == NULL)
    {
        return;
    }
    for (y = 0u; y < NDS_FIGHTER_PREVIEW_LOOP_HEIGHT; y++)
    {
        for (x = 0u; x < NDS_FIGHTER_PREVIEW_LOOP_WIDTH; x++)
        {
            pixels[y * pitch + x] = bg;
        }
    }
    for (x = 0u; x < NDS_FIGHTER_PREVIEW_LOOP_WIDTH; x++)
    {
        pixels[(NDS_FIGHTER_PREVIEW_LOOP_HEIGHT - 10u) * pitch + x] = floor;
    }
}

static void ndsFighterPreviewLoopDrawSlot(u32 slot, FTStruct *fp,
                                          u16 *pixels, u32 pitch)
{
    NDSFighterPreviewLoopState *state;
    NDSFighterDLAllDrawCollection collection;
    DObj *root;
    s32 root_delta;
    s32 root_rise;
    s32 root_rise_max;
    s32 screen_x;
    s32 screen_y;
    u32 pixel_count = 0u;
    u32 checksum = 0u;
    u32 i;

    if ((slot >= 2u) || (fp == NULL) || (pixels == NULL))
    {
        return;
    }
    root = fp->joints[nFTPartsJointTopN];
    if (root == NULL)
    {
        return;
    }
    state = &sNdsFighterPreviewLoopStates[slot];
    ndsFighterCollectAllDObjsWithDL(root, &collection);

    root_delta = ndsFloatToMilliSigned(root->translate.vec.f.x) -
        ((slot == 0u) ? gNdsFighterPreviewLoopP0RootXStartMilli :
            gNdsFighterPreviewLoopP1RootXStartMilli);
    root_rise = ndsFloatToMilliSigned(root->translate.vec.f.y) -
        ndsFloatToMilliSigned(fp->coll_data.floor_dist);
    root_rise_max = ndsFloatToMilliSigned(state->root_y_max) -
        ndsFloatToMilliSigned(state->root_y_start);
    if (root_rise < root_rise_max)
    {
        root_rise = root_rise_max;
    }
    screen_x = state->screen_x_start + (root_delta / 1500);
    screen_y = state->screen_y_floor - (root_rise / 1200);
    screen_x = ndsFighterPreviewLoopClampS32(screen_x, 6, 89);
    screen_y = ndsFighterPreviewLoopClampS32(screen_y, 8, 62);

    if ((state->screen_initialized == 0u) ||
        (screen_y < state->screen_y_min))
    {
        state->screen_y_min = screen_y;
    }
    state->screen_initialized = 1u;
    state->screen_x_final = screen_x;

    for (i = 0u; i < collection.selected_count; i++)
    {
        s32 ox = (s32)(i % 5u) - 2;
        s32 oy = -3 - (s32)((i / 5u) * 3u);
        u16 color = (slot == 0u) ?
            ndsFighterDLDrawRGB15(245, 70 + ((i * 7u) & 31u), 45) :
            ndsFighterDLDrawRGB15(60, 105 + ((i * 5u) & 31u), 245);

        ndsFighterPreviewLoopPlot(pixels, pitch, screen_x + ox,
                                  screen_y + oy, color, &pixel_count,
                                  &checksum);
        ndsFighterPreviewLoopPlot(pixels, pitch, screen_x + ox - 1,
                                  screen_y + oy + 1, color, &pixel_count,
                                  &checksum);
        ndsFighterPreviewLoopPlot(pixels, pitch, screen_x + ox + 1,
                                  screen_y + oy + 1, color, &pixel_count,
                                  &checksum);
    }

    if (slot == 0u)
    {
        gNdsFighterPreviewLoopP0CandidateCount = collection.total_count;
        gNdsFighterPreviewLoopP0DrawnDObjCount = collection.selected_count;
        gNdsFighterPreviewLoopP0PixelCount += pixel_count;
        gNdsFighterPreviewLoopP0ColorChecksum =
            (gNdsFighterPreviewLoopP0ColorChecksum * 33u) ^ checksum;
        gNdsFighterPreviewLoopP0ScreenXFinal = screen_x;
        gNdsFighterPreviewLoopP0ScreenXDelta =
            screen_x - gNdsFighterPreviewLoopP0ScreenXStart;
        gNdsFighterPreviewLoopP0ScreenYMin = state->screen_y_min;
        gNdsFighterPreviewLoopP0ScreenRise =
            gNdsFighterPreviewLoopP0ScreenYFloor - state->screen_y_min;
    }
    else
    {
        gNdsFighterPreviewLoopP1CandidateCount = collection.total_count;
        gNdsFighterPreviewLoopP1DrawnDObjCount = collection.selected_count;
        gNdsFighterPreviewLoopP1PixelCount += pixel_count;
        gNdsFighterPreviewLoopP1ColorChecksum =
            (gNdsFighterPreviewLoopP1ColorChecksum * 33u) ^ checksum;
        gNdsFighterPreviewLoopP1ScreenXFinal = screen_x;
        gNdsFighterPreviewLoopP1ScreenXDelta =
            screen_x - gNdsFighterPreviewLoopP1ScreenXStart;
        gNdsFighterPreviewLoopP1ScreenYMin = state->screen_y_min;
        gNdsFighterPreviewLoopP1ScreenRise =
            gNdsFighterPreviewLoopP1ScreenYFloor - state->screen_y_min;
    }
}

static void ndsFighterPreviewLoopRecordDisplayFromCallback(GObj *fighter_gobj)
{
    FTStruct *fp;
    u32 slot;

    if ((fighter_gobj == NULL) || (sNdsFighterPreviewLoopPixels == NULL))
    {
        return;
    }
    fp = ftGetStruct(fighter_gobj);
    if (ndsFighterStructIsPoolPointer(fp) == FALSE)
    {
        return;
    }
    slot = (u32)fp->nds_slot;
    if (slot > 1u)
    {
        return;
    }

    gNdsFighterPreviewLoopDisplayCallbackCount++;
    if (slot == 0u)
    {
        gNdsFighterPreviewLoopP0DisplayCallbackCount++;
    }
    else
    {
        gNdsFighterPreviewLoopP1DisplayCallbackCount++;
    }
    ndsFighterPreviewLoopDrawSlot(slot, fp, sNdsFighterPreviewLoopPixels,
                                  sNdsFighterPreviewLoopPitch);
}

static void ndsFighterPreviewLoopDrawKeyframe(void)
{
    u32 pitch = 0u;
    u16 *pixels;

    pixels = ndsPlatformBeginOriginalDLPreview(
        NDS_FIGHTER_PREVIEW_LOOP_WIDTH,
        NDS_FIGHTER_PREVIEW_LOOP_HEIGHT,
        &pitch);
    if (pixels == NULL)
    {
        return;
    }
    if (gNdsFighterPreviewLoopDrawFrameCount == 0u)
    {
        gNdsFighterPreviewLoopPreviewCommitBefore =
            gNdsOriginalDLPreviewCommitCount;
    }
    gNdsFighterPreviewLoopPreviewWidth = NDS_FIGHTER_PREVIEW_LOOP_WIDTH;
    gNdsFighterPreviewLoopPreviewHeight = NDS_FIGHTER_PREVIEW_LOOP_HEIGHT;
    gNdsFighterPreviewLoopPreviewPitch = pitch;
    sNdsFighterPreviewLoopPixels = pixels;
    sNdsFighterPreviewLoopPitch = pitch;
    sNdsFighterPreviewLoopDisplayActive = TRUE;
    ndsFighterPreviewLoopClear(pixels, pitch);
    ftDisplayMainProcDisplay(sNdsFighterStructPool[0].fighter_gobj);
    ftDisplayMainProcDisplay(sNdsFighterStructPool[1].fighter_gobj);
    sNdsFighterPreviewLoopDisplayActive = FALSE;
    sNdsFighterPreviewLoopPixels = NULL;
    sNdsFighterPreviewLoopPitch = 0u;

    gNdsFighterPreviewLoopTotalPixelCount =
        gNdsFighterPreviewLoopP0PixelCount +
        gNdsFighterPreviewLoopP1PixelCount;
    if (gNdsFighterPreviewLoopTotalPixelCount > 0u)
    {
        ndsPlatformCommitOriginalDLPreview();
        gNdsFighterPreviewLoopPreviewCommitAfter =
            gNdsOriginalDLPreviewCommitCount;
        gNdsFighterPreviewLoopPreviewCommitDelta =
            gNdsFighterPreviewLoopPreviewCommitAfter -
            gNdsFighterPreviewLoopPreviewCommitBefore;
        gNdsFighterPreviewLoopPreviewReady = gNdsOriginalDLPreviewReady;
        gNdsFighterPreviewLoopDrawFrameCount++;
    }
    sNdsFighterPreviewLoopDrawFrameIndex++;
}

static void ndsFighterPreviewLoopRecordFinal(u32 slot, FTStruct *fp,
                                             DObj *root)
{
    NDSFighterPreviewLoopState *state;
    s32 root_y_final;
    s32 floor_y;
    s32 root_delta;

    if ((slot >= 2u) || (fp == NULL) || (root == NULL))
    {
        return;
    }
    state = &sNdsFighterPreviewLoopStates[slot];
    if ((state->completed != 0u) && (fp->status_id == nFTCommonStatusWait))
    {
        state->status_visit_mask |= 1u << 9;
    }
    root_y_final = ndsFloatToMilliSigned(root->translate.vec.f.y);
    floor_y = ndsFloatToMilliSigned(fp->coll_data.floor_dist);
    root_delta = ndsFloatToMilliSigned(root->translate.vec.f.x) -
        ((slot == 0u) ? gNdsFighterPreviewLoopP0RootXStartMilli :
            gNdsFighterPreviewLoopP1RootXStartMilli);

    if (slot == 0u)
    {
        gNdsFighterPreviewLoopP0Completed = state->completed;
        gNdsFighterPreviewLoopP0StatusVisitMask = state->status_visit_mask;
        gNdsFighterPreviewLoopP0TransitionMask = state->transition_mask;
        gNdsFighterPreviewLoopP0StatusFinal = (u32)fp->status_id;
        gNdsFighterPreviewLoopP0MotionFinal = (u32)fp->motion_id;
        gNdsFighterPreviewLoopP0GAFinal = (u32)fp->ga;
        gNdsFighterPreviewLoopP0RootYFinalMilli = root_y_final;
        gNdsFighterPreviewLoopP0RootDeltaXMilli = root_delta;
        gNdsFighterPreviewLoopP0RootRiseMilli =
            ndsFloatToMilliSigned(state->root_y_max) -
            ndsFloatToMilliSigned(state->root_y_start);
        gNdsFighterPreviewLoopP0RootDirectionOK =
            ((gNdsFighterPreviewLoopP0RootDeltaXMilli * fp->lr) > 0) ?
            1u : 0u;
        gNdsFighterPreviewLoopP0FloorOK =
            (root_y_final == floor_y) ? 1u : 0u;
    }
    else
    {
        gNdsFighterPreviewLoopP1Completed = state->completed;
        gNdsFighterPreviewLoopP1StatusVisitMask = state->status_visit_mask;
        gNdsFighterPreviewLoopP1TransitionMask = state->transition_mask;
        gNdsFighterPreviewLoopP1StatusFinal = (u32)fp->status_id;
        gNdsFighterPreviewLoopP1MotionFinal = (u32)fp->motion_id;
        gNdsFighterPreviewLoopP1GAFinal = (u32)fp->ga;
        gNdsFighterPreviewLoopP1RootYFinalMilli = root_y_final;
        gNdsFighterPreviewLoopP1RootDeltaXMilli = root_delta;
        gNdsFighterPreviewLoopP1RootRiseMilli =
            ndsFloatToMilliSigned(state->root_y_max) -
            ndsFloatToMilliSigned(state->root_y_start);
        gNdsFighterPreviewLoopP1RootDirectionOK =
            ((gNdsFighterPreviewLoopP1RootDeltaXMilli * fp->lr) > 0) ?
            1u : 0u;
        gNdsFighterPreviewLoopP1FloorOK =
            (root_y_final == floor_y) ? 1u : 0u;
    }
}

void ndsFighterMarioFoxPreviewLoopPrepare(void)
{
    u32 i;

    if ((ndsFighterMarioFoxPreviewLoopProofEnabled() == FALSE) ||
        (gNdsFighterPreviewLoopPrepared != 0u))
    {
        return;
    }
    if ((gNdsFighterMarioFoxControllerLoopResult !=
            NDS_FIGHTER_MARIOFOX_CONTROLLER_LOOP_PASS) ||
        (gNdsFighterMarioFoxControllerLoopSafeResult !=
            NDS_FIGHTER_MARIOFOX_CONTROLLER_LOOP_SAFE_PASS) ||
        ((gNdsFighterMarioFoxControllerLoopMask & 0xfffu) != 0xfffu) ||
        (gNdsFighterMarioFoxControllerLoopDeferredMask != 0xffu) ||
        (gNdsFighterMarioFoxControllerLoopCount != 2u))
    {
        return;
    }

    bzero(sNdsFighterPreviewLoopStates,
          sizeof(sNdsFighterPreviewLoopStates));
    bzero(sNdsFighterPreviewLoopProcesses,
          sizeof(sNdsFighterPreviewLoopProcesses));
    sNdsFighterPreviewLoopPixels = NULL;
    sNdsFighterPreviewLoopPitch = 0u;
    sNdsFighterPreviewLoopDrawFrameIndex = 0u;
    ndsControllerPlaybackReset();
    ndsControllerPlaybackSetConnectedMask(0x3u);
    ndsControllerPlaybackSetEnabled(TRUE);
    gNdsFighterPreviewLoopFrameMax = NDS_FIGHTER_PREVIEW_LOOP_FRAME_MAX;
    gNdsFighterPreviewLoopUpdateMax = NDS_FIGHTER_PREVIEW_LOOP_UPDATE_MAX;
    gNdsFighterPreviewLoopGObjCountBefore = (u32)gcGetGObjsActiveNum();

    for (i = 0; i < 2u; i++)
    {
        FTStruct *fp = &sNdsFighterStructPool[i];
        GObj *fighter_gobj = fp->fighter_gobj;
        DObj *root = fp->joints[nFTPartsJointTopN];

        if ((ndsFighterStructIsPoolPointer(fp) == FALSE) ||
            (fighter_gobj == NULL) || (root == NULL))
        {
            gNdsFighterPreviewLoopProcessAttachEscapeCount++;
            continue;
        }
        ndsFighterPreviewLoopRecordStart(i, fp, root);
        sNdsFighterPreviewLoopProcesses[i] =
            gcAddGObjProcess(fighter_gobj,
                             ndsFighterPreviewLoopGObjProc,
                             nGCProcessKindFunc,
                             3);
        if (sNdsFighterPreviewLoopProcesses[i] == NULL)
        {
            gNdsFighterPreviewLoopProcessAttachEscapeCount++;
        }
        else if (i == 0u)
        {
            gNdsFighterPreviewLoopP0ProcessAttachCount++;
        }
        else
        {
            gNdsFighterPreviewLoopP1ProcessAttachCount++;
        }
    }

    if ((sNdsFighterPreviewLoopProcesses[0] != NULL) &&
        (sNdsFighterPreviewLoopProcesses[1] != NULL))
    {
        gNdsFighterPreviewLoopPrepared = 1u;
    }
}

s32 ndsFighterMarioFoxPreviewLoopUpdateEnabled(void)
{
    return ((ndsFighterMarioFoxPreviewLoopProofEnabled() != FALSE) &&
            (gNdsFighterPreviewLoopPrepared != 0u) &&
            (gNdsFighterMarioFoxPreviewLoopResult == 0u)) ? TRUE : FALSE;
}

void ndsFighterMarioFoxPreviewLoopRunVSBattleUpdate(void)
{
    u32 i;
    u32 mask = 0u;

    if (ndsFighterMarioFoxPreviewLoopUpdateEnabled() == FALSE)
    {
        return;
    }
    gNdsFighterPreviewLoopVSBattleUpdateCount++;
    gNdsFighterPreviewLoopSchedulerUpdateCount++;

    for (i = 0; i < 2u; i++)
    {
        ndsFighterPreviewLoopApplyPlayback(i, &sNdsFighterStructPool[i]);
    }
    ndsControllerPlaybackCommitFrame();
    syControllerReadDeviceData();
    gNdsFighterPreviewLoopSYReadCount++;
    syControllerUpdateGlobalData();
    gNdsFighterPreviewLoopSYUpdateCount++;

    for (i = 0; i < 2u; i++)
    {
        if (sNdsFighterPreviewLoopProcesses[i] == NULL)
        {
            continue;
        }
        if (i == 0u)
        {
            gNdsFighterPreviewLoopP0GObjProcessRunCount++;
        }
        else
        {
            gNdsFighterPreviewLoopP1GObjProcessRunCount++;
        }
        gcRunGObjProcess(sNdsFighterPreviewLoopProcesses[i]);
    }

    for (i = 0; i < 2u; i++)
    {
        FTStruct *fp = &sNdsFighterStructPool[i];
        DObj *root = fp->joints[nFTPartsJointTopN];
        ndsFighterPreviewLoopRecordFinal(i, fp, root);
    }

    if (((gNdsFighterPreviewLoopVSBattleUpdateCount %
            NDS_FIGHTER_PREVIEW_LOOP_DRAW_FRAME_INTERVAL) == 0u) ||
        ((gNdsFighterPreviewLoopP0Completed == 1u) &&
         (gNdsFighterPreviewLoopP1Completed == 1u)))
    {
        ndsFighterPreviewLoopDrawKeyframe();
    }

    if ((gNdsFighterPreviewLoopP0Completed == 1u) &&
        (gNdsFighterPreviewLoopP1Completed == 1u))
    {
        gNdsFighterPreviewLoopRootYDriftCount = 0u;
        gNdsFighterPreviewLoopGADriftCount = 0u;
        if ((gNdsFighterPreviewLoopP0StatusFinal !=
                (u32)nFTCommonStatusWait) ||
            (gNdsFighterPreviewLoopP1StatusFinal !=
                (u32)nFTCommonStatusWait) ||
            (gNdsFighterPreviewLoopP0MotionFinal !=
                (u32)nFTCommonMotionWait) ||
            (gNdsFighterPreviewLoopP1MotionFinal !=
                (u32)nFTCommonMotionWait) ||
            (gNdsFighterPreviewLoopP0GAFinal !=
                (u32)nMPKineticsGround) ||
            (gNdsFighterPreviewLoopP1GAFinal !=
                (u32)nMPKineticsGround))
        {
            gNdsFighterPreviewLoopGADriftCount++;
        }
        if ((gNdsFighterPreviewLoopP0FloorOK != 1u) ||
            (gNdsFighterPreviewLoopP1FloorOK != 1u))
        {
            gNdsFighterPreviewLoopRootYDriftCount++;
        }
    }

    if ((gNdsFighterMarioFoxControllerLoopResult ==
            NDS_FIGHTER_MARIOFOX_CONTROLLER_LOOP_PASS) &&
        (gNdsFighterMarioFoxControllerLoopSafeResult ==
            NDS_FIGHTER_MARIOFOX_CONTROLLER_LOOP_SAFE_PASS))
    {
        mask |= 1u << 0;
    }
    if ((gNdsControllerPlaybackEnabled == 1u) &&
        ((gNdsControllerPlaybackConnectedMask & 0x3u) == 0x3u) &&
        (gNdsControllerPlaybackFrameCount > 0u) &&
        (gNdsControllerPlaybackReadCount > 0u) &&
        (gNdsControllerLiveReadCount == 0u) &&
        (gNdsFighterPreviewLoopSYReadCount ==
            gNdsControllerPlaybackReadCount) &&
        (gNdsFighterPreviewLoopSYUpdateCount ==
            gNdsControllerPlaybackReadCount))
    {
        mask |= 1u << 1;
    }
    if ((gNdsFighterPreviewLoopPrepared == 1u) &&
        (gNdsFighterPreviewLoopTaskmanUpdateCount > 0u) &&
        (gNdsFighterPreviewLoopVSBattleUpdateCount > 0u) &&
        (gNdsFighterPreviewLoopSchedulerUpdateCount > 0u))
    {
        mask |= 1u << 2;
    }
    if ((gNdsFighterPreviewLoopP0ProcessAttachCount == 1u) &&
        (gNdsFighterPreviewLoopP1ProcessAttachCount == 1u) &&
        (gNdsFighterPreviewLoopP0GObjProcessRunCount > 0u) &&
        (gNdsFighterPreviewLoopP1GObjProcessRunCount > 0u) &&
        (gNdsFighterPreviewLoopP0ProcCallbackCount ==
            gNdsFighterPreviewLoopP0GObjProcessRunCount) &&
        (gNdsFighterPreviewLoopP1ProcCallbackCount ==
            gNdsFighterPreviewLoopP1GObjProcessRunCount))
    {
        mask |= 1u << 3;
    }
    if ((gNdsFighterPreviewLoopP0PlaybackApplyCount > 0u) &&
        (gNdsFighterPreviewLoopP1PlaybackApplyCount > 0u) &&
        (gNdsFighterPreviewLoopP0ControllerToFTInputCount > 0u) &&
        (gNdsFighterPreviewLoopP1ControllerToFTInputCount > 0u) &&
        (gNdsFighterPreviewLoopP0DirectFTInputWriteCount == 0u) &&
        (gNdsFighterPreviewLoopP1DirectFTInputWriteCount == 0u) &&
        (gNdsFighterPreviewLoopP0ButtonTapMask != 0u) &&
        (gNdsFighterPreviewLoopP1ButtonTapMask != 0u) &&
        (gNdsFighterPreviewLoopP0ButtonHoldMask != 0u) &&
        (gNdsFighterPreviewLoopP1ButtonHoldMask != 0u) &&
        (gNdsFighterPreviewLoopP0DashTapEligibleCount > 0u) &&
        (gNdsFighterPreviewLoopP1DashTapEligibleCount > 0u) &&
        (gNdsFighterPreviewLoopP0JumpButtonTapCount > 0u) &&
        (gNdsFighterPreviewLoopP1JumpButtonTapCount > 0u))
    {
        mask |= 1u << 4;
    }
    if ((gNdsFighterPreviewLoopP0Completed == 1u) &&
        (gNdsFighterPreviewLoopP1Completed == 1u) &&
        (gNdsFighterPreviewLoopP0FrameCount > 0u) &&
        (gNdsFighterPreviewLoopP1FrameCount > 0u) &&
        (gNdsFighterPreviewLoopP0FrameCount <=
            gNdsFighterPreviewLoopFrameMax) &&
        (gNdsFighterPreviewLoopP1FrameCount <=
            gNdsFighterPreviewLoopFrameMax))
    {
        mask |= 1u << 5;
    }
    if (((gNdsFighterPreviewLoopP0StatusVisitMask &
            NDS_FIGHTER_PREVIEW_LOOP_STATUS_MASK_REQUIRED) ==
            NDS_FIGHTER_PREVIEW_LOOP_STATUS_MASK_REQUIRED) &&
        ((gNdsFighterPreviewLoopP1StatusVisitMask &
            NDS_FIGHTER_PREVIEW_LOOP_STATUS_MASK_REQUIRED) ==
            NDS_FIGHTER_PREVIEW_LOOP_STATUS_MASK_REQUIRED))
    {
        mask |= 1u << 6;
    }
    if (((gNdsFighterPreviewLoopP0TransitionMask &
            NDS_FIGHTER_PREVIEW_LOOP_TRANSITION_MASK_REQUIRED) ==
            NDS_FIGHTER_PREVIEW_LOOP_TRANSITION_MASK_REQUIRED) &&
        ((gNdsFighterPreviewLoopP1TransitionMask &
            NDS_FIGHTER_PREVIEW_LOOP_TRANSITION_MASK_REQUIRED) ==
            NDS_FIGHTER_PREVIEW_LOOP_TRANSITION_MASK_REQUIRED))
    {
        mask |= 1u << 7;
    }
    if ((gNdsFighterPreviewLoopP0InterruptCount > 0u) &&
        (gNdsFighterPreviewLoopP1InterruptCount > 0u) &&
        (gNdsFighterPreviewLoopP0PhysicsCount > 0u) &&
        (gNdsFighterPreviewLoopP1PhysicsCount > 0u) &&
        (gNdsFighterPreviewLoopP0IntegrateCount > 0u) &&
        (gNdsFighterPreviewLoopP1IntegrateCount > 0u) &&
        (gNdsFighterPreviewLoopP0MapCount > 0u) &&
        (gNdsFighterPreviewLoopP1MapCount > 0u))
    {
        mask |= 1u << 8;
    }
    if ((gNdsFighterPreviewLoopP0RootDeltaXMilli != 0) &&
        (gNdsFighterPreviewLoopP1RootDeltaXMilli != 0) &&
        (gNdsFighterPreviewLoopP0RootRiseMilli > 0) &&
        (gNdsFighterPreviewLoopP1RootRiseMilli > 0) &&
        (gNdsFighterPreviewLoopP0RootDirectionOK == 1u) &&
        (gNdsFighterPreviewLoopP1RootDirectionOK == 1u))
    {
        mask |= 1u << 9;
    }
    if ((gNdsFighterPreviewLoopPreviewReady != 0u) &&
        (gNdsFighterPreviewLoopPreviewCommitDelta >=
            NDS_FIGHTER_PREVIEW_LOOP_DRAW_FRAME_MIN) &&
        (gNdsFighterPreviewLoopDrawFrameCount >=
            NDS_FIGHTER_PREVIEW_LOOP_DRAW_FRAME_MIN) &&
        (gNdsFighterPreviewLoopDisplayCallbackCount >=
            (NDS_FIGHTER_PREVIEW_LOOP_DRAW_FRAME_MIN * 2u)) &&
        (gNdsFighterPreviewLoopP0DisplayCallbackCount >=
            NDS_FIGHTER_PREVIEW_LOOP_DRAW_FRAME_MIN) &&
        (gNdsFighterPreviewLoopP1DisplayCallbackCount >=
            NDS_FIGHTER_PREVIEW_LOOP_DRAW_FRAME_MIN) &&
        (gNdsFighterPreviewLoopP0CandidateCount >= 14u) &&
        (gNdsFighterPreviewLoopP1CandidateCount >= 18u) &&
        (gNdsFighterPreviewLoopP0DrawnDObjCount >= 14u) &&
        (gNdsFighterPreviewLoopP1DrawnDObjCount >= 18u) &&
        (gNdsFighterPreviewLoopP0PixelCount > 0u) &&
        (gNdsFighterPreviewLoopP1PixelCount > 0u) &&
        (gNdsFighterPreviewLoopP0ColorChecksum != 0u) &&
        (gNdsFighterPreviewLoopP1ColorChecksum != 0u) &&
        (gNdsFighterPreviewLoopP0ScreenXDelta != 0) &&
        (gNdsFighterPreviewLoopP1ScreenXDelta != 0) &&
        (gNdsFighterPreviewLoopP0ScreenRise > 0) &&
        (gNdsFighterPreviewLoopP1ScreenRise > 0))
    {
        mask |= 1u << 10;
    }
    if ((gNdsFighterPreviewLoopP0StatusFinal ==
            (u32)nFTCommonStatusWait) &&
        (gNdsFighterPreviewLoopP1StatusFinal ==
            (u32)nFTCommonStatusWait) &&
        (gNdsFighterPreviewLoopP0MotionFinal ==
            (u32)nFTCommonMotionWait) &&
        (gNdsFighterPreviewLoopP1MotionFinal ==
            (u32)nFTCommonMotionWait) &&
        (gNdsFighterPreviewLoopP0GAFinal == (u32)nMPKineticsGround) &&
        (gNdsFighterPreviewLoopP1GAFinal == (u32)nMPKineticsGround) &&
        (gNdsFighterPreviewLoopP0FloorOK == 1u) &&
        (gNdsFighterPreviewLoopP1FloorOK == 1u))
    {
        mask |= 1u << 11;
    }

    gNdsFighterPreviewLoopGObjCountAfter = (u32)gcGetGObjsActiveNum();
    gNdsFighterPreviewLoopGObjDelta =
        (gNdsFighterPreviewLoopGObjCountAfter >=
         gNdsFighterPreviewLoopGObjCountBefore) ?
        (gNdsFighterPreviewLoopGObjCountAfter -
         gNdsFighterPreviewLoopGObjCountBefore) :
        (gNdsFighterPreviewLoopGObjCountBefore -
         gNdsFighterPreviewLoopGObjCountAfter);

    if ((gNdsFighterPreviewLoopGObjDelta == 0u) &&
        (gNdsFighterPreviewLoopUnexpectedStatusCount == 0u) &&
        (gNdsFighterPreviewLoopDeniedStatusCount == 0u) &&
        (gNdsFighterPreviewLoopProcessAttachEscapeCount == 0u) &&
        (gNdsFighterPreviewLoopDisplayProbeCount == 0u) &&
        (gNdsFighterPreviewLoopGameplayUpdateCount == 0u) &&
        (gNdsFighterPreviewLoopDrawCallCount == 0u) &&
        (gNdsFighterPreviewLoopMatrixCallCount == 0u) &&
        (gNdsFighterPreviewLoopRootYDriftCount == 0u) &&
        (gNdsFighterPreviewLoopGADriftCount == 0u))
    {
        mask |= 1u << 12;
    }

    gNdsFighterMarioFoxPreviewLoopMask = mask;
    gNdsFighterMarioFoxPreviewLoopDeferredMask = 0xffu;
    gNdsFighterMarioFoxPreviewLoopCount =
        gNdsFighterPreviewLoopP0Completed +
        gNdsFighterPreviewLoopP1Completed;

    if ((mask & 0x1fffu) == 0x1fffu)
    {
        gNdsFighterMarioFoxPreviewLoopResult =
            NDS_FIGHTER_MARIOFOX_PREVIEW_LOOP_PASS;
        gNdsFighterMarioFoxPreviewLoopSafeResult =
            NDS_FIGHTER_MARIOFOX_PREVIEW_LOOP_SAFE_PASS;
    }
}

static void ndsFighterGCRunAllLoopCopyFromPreview(void)
{
    gNdsFighterGCRunAllLoopP0PlaybackApplyCount =
        gNdsFighterPreviewLoopP0PlaybackApplyCount;
    gNdsFighterGCRunAllLoopP1PlaybackApplyCount =
        gNdsFighterPreviewLoopP1PlaybackApplyCount;
    gNdsFighterGCRunAllLoopP0ControllerToFTInputCount =
        gNdsFighterPreviewLoopP0ControllerToFTInputCount;
    gNdsFighterGCRunAllLoopP1ControllerToFTInputCount =
        gNdsFighterPreviewLoopP1ControllerToFTInputCount;
    gNdsFighterGCRunAllLoopP0DirectFTInputWriteCount = 0u;
    gNdsFighterGCRunAllLoopP1DirectFTInputWriteCount = 0u;
    gNdsFighterGCRunAllLoopP0ButtonTapMask =
        gNdsFighterPreviewLoopP0ButtonTapMask;
    gNdsFighterGCRunAllLoopP1ButtonTapMask =
        gNdsFighterPreviewLoopP1ButtonTapMask;
    gNdsFighterGCRunAllLoopP0ButtonHoldMask =
        gNdsFighterPreviewLoopP0ButtonHoldMask;
    gNdsFighterGCRunAllLoopP1ButtonHoldMask =
        gNdsFighterPreviewLoopP1ButtonHoldMask;
    gNdsFighterGCRunAllLoopP0LastStickX =
        gNdsFighterPreviewLoopP0LastStickX;
    gNdsFighterGCRunAllLoopP1LastStickX =
        gNdsFighterPreviewLoopP1LastStickX;
    gNdsFighterGCRunAllLoopP0LastStickY =
        gNdsFighterPreviewLoopP0LastStickY;
    gNdsFighterGCRunAllLoopP1LastStickY =
        gNdsFighterPreviewLoopP1LastStickY;
    gNdsFighterGCRunAllLoopP0DashTapEligibleCount =
        gNdsFighterPreviewLoopP0DashTapEligibleCount;
    gNdsFighterGCRunAllLoopP1DashTapEligibleCount =
        gNdsFighterPreviewLoopP1DashTapEligibleCount;
    gNdsFighterGCRunAllLoopP0JumpButtonTapCount =
        gNdsFighterPreviewLoopP0JumpButtonTapCount;
    gNdsFighterGCRunAllLoopP1JumpButtonTapCount =
        gNdsFighterPreviewLoopP1JumpButtonTapCount;
    gNdsFighterGCRunAllLoopP0FrameCount =
        gNdsFighterPreviewLoopP0FrameCount;
    gNdsFighterGCRunAllLoopP1FrameCount =
        gNdsFighterPreviewLoopP1FrameCount;
    gNdsFighterGCRunAllLoopP0Completed =
        gNdsFighterPreviewLoopP0Completed;
    gNdsFighterGCRunAllLoopP1Completed =
        gNdsFighterPreviewLoopP1Completed;
    gNdsFighterGCRunAllLoopP0StatusVisitMask =
        gNdsFighterPreviewLoopP0StatusVisitMask;
    gNdsFighterGCRunAllLoopP1StatusVisitMask =
        gNdsFighterPreviewLoopP1StatusVisitMask;
    gNdsFighterGCRunAllLoopP0TransitionMask =
        gNdsFighterPreviewLoopP0TransitionMask;
    gNdsFighterGCRunAllLoopP1TransitionMask =
        gNdsFighterPreviewLoopP1TransitionMask;
    gNdsFighterGCRunAllLoopP0WaitVisitCount =
        sNdsFighterPreviewLoopStates[0].wait_visit_count;
    gNdsFighterGCRunAllLoopP1WaitVisitCount =
        sNdsFighterPreviewLoopStates[1].wait_visit_count;
    gNdsFighterGCRunAllLoopP0WalkVisitCount =
        sNdsFighterPreviewLoopStates[0].walk_visit_count;
    gNdsFighterGCRunAllLoopP1WalkVisitCount =
        sNdsFighterPreviewLoopStates[1].walk_visit_count;
    gNdsFighterGCRunAllLoopP0DashVisitCount =
        sNdsFighterPreviewLoopStates[0].dash_visit_count;
    gNdsFighterGCRunAllLoopP1DashVisitCount =
        sNdsFighterPreviewLoopStates[1].dash_visit_count;
    gNdsFighterGCRunAllLoopP0RunVisitCount =
        sNdsFighterPreviewLoopStates[0].run_visit_count;
    gNdsFighterGCRunAllLoopP1RunVisitCount =
        sNdsFighterPreviewLoopStates[1].run_visit_count;
    gNdsFighterGCRunAllLoopP0RunBrakeVisitCount =
        sNdsFighterPreviewLoopStates[0].runbrake_visit_count;
    gNdsFighterGCRunAllLoopP1RunBrakeVisitCount =
        sNdsFighterPreviewLoopStates[1].runbrake_visit_count;
    gNdsFighterGCRunAllLoopP0KneeBendVisitCount =
        sNdsFighterPreviewLoopStates[0].kneebend_visit_count;
    gNdsFighterGCRunAllLoopP1KneeBendVisitCount =
        sNdsFighterPreviewLoopStates[1].kneebend_visit_count;
    gNdsFighterGCRunAllLoopP0JumpVisitCount =
        sNdsFighterPreviewLoopStates[0].jump_visit_count;
    gNdsFighterGCRunAllLoopP1JumpVisitCount =
        sNdsFighterPreviewLoopStates[1].jump_visit_count;
    gNdsFighterGCRunAllLoopP0FallVisitCount =
        sNdsFighterPreviewLoopStates[0].fall_visit_count;
    gNdsFighterGCRunAllLoopP1FallVisitCount =
        sNdsFighterPreviewLoopStates[1].fall_visit_count;
    gNdsFighterGCRunAllLoopP0LandingVisitCount =
        sNdsFighterPreviewLoopStates[0].landing_visit_count;
    gNdsFighterGCRunAllLoopP1LandingVisitCount =
        sNdsFighterPreviewLoopStates[1].landing_visit_count;
    gNdsFighterGCRunAllLoopP0StatusStart =
        gNdsFighterPreviewLoopP0StatusStart;
    gNdsFighterGCRunAllLoopP1StatusStart =
        gNdsFighterPreviewLoopP1StatusStart;
    gNdsFighterGCRunAllLoopP0MotionStart =
        gNdsFighterPreviewLoopP0MotionStart;
    gNdsFighterGCRunAllLoopP1MotionStart =
        gNdsFighterPreviewLoopP1MotionStart;
    gNdsFighterGCRunAllLoopP0StatusFinal =
        gNdsFighterPreviewLoopP0StatusFinal;
    gNdsFighterGCRunAllLoopP1StatusFinal =
        gNdsFighterPreviewLoopP1StatusFinal;
    gNdsFighterGCRunAllLoopP0MotionFinal =
        gNdsFighterPreviewLoopP0MotionFinal;
    gNdsFighterGCRunAllLoopP1MotionFinal =
        gNdsFighterPreviewLoopP1MotionFinal;
    gNdsFighterGCRunAllLoopP0GAFinal =
        gNdsFighterPreviewLoopP0GAFinal;
    gNdsFighterGCRunAllLoopP1GAFinal =
        gNdsFighterPreviewLoopP1GAFinal;
    gNdsFighterGCRunAllLoopP0RootXStartMilli =
        gNdsFighterPreviewLoopP0RootXStartMilli;
    gNdsFighterGCRunAllLoopP1RootXStartMilli =
        gNdsFighterPreviewLoopP1RootXStartMilli;
    gNdsFighterGCRunAllLoopP0RootDeltaXMilli =
        gNdsFighterPreviewLoopP0RootDeltaXMilli;
    gNdsFighterGCRunAllLoopP1RootDeltaXMilli =
        gNdsFighterPreviewLoopP1RootDeltaXMilli;
    gNdsFighterGCRunAllLoopP0RootRiseMilli =
        gNdsFighterPreviewLoopP0RootRiseMilli;
    gNdsFighterGCRunAllLoopP1RootRiseMilli =
        gNdsFighterPreviewLoopP1RootRiseMilli;
    gNdsFighterGCRunAllLoopP0RootYFinalMilli =
        gNdsFighterPreviewLoopP0RootYFinalMilli;
    gNdsFighterGCRunAllLoopP1RootYFinalMilli =
        gNdsFighterPreviewLoopP1RootYFinalMilli;
    gNdsFighterGCRunAllLoopP0FloorYMilli =
        gNdsFighterPreviewLoopP0FloorYMilli;
    gNdsFighterGCRunAllLoopP1FloorYMilli =
        gNdsFighterPreviewLoopP1FloorYMilli;
    gNdsFighterGCRunAllLoopP0RootDirectionOK =
        gNdsFighterPreviewLoopP0RootDirectionOK;
    gNdsFighterGCRunAllLoopP1RootDirectionOK =
        gNdsFighterPreviewLoopP1RootDirectionOK;
    gNdsFighterGCRunAllLoopP0FloorOK =
        gNdsFighterPreviewLoopP0FloorOK;
    gNdsFighterGCRunAllLoopP1FloorOK =
        gNdsFighterPreviewLoopP1FloorOK;
    gNdsFighterGCRunAllLoopP0InterruptCount =
        gNdsFighterPreviewLoopP0InterruptCount;
    gNdsFighterGCRunAllLoopP1InterruptCount =
        gNdsFighterPreviewLoopP1InterruptCount;
    gNdsFighterGCRunAllLoopP0PhysicsCount =
        gNdsFighterPreviewLoopP0PhysicsCount;
    gNdsFighterGCRunAllLoopP1PhysicsCount =
        gNdsFighterPreviewLoopP1PhysicsCount;
    gNdsFighterGCRunAllLoopP0IntegrateCount =
        gNdsFighterPreviewLoopP0IntegrateCount;
    gNdsFighterGCRunAllLoopP1IntegrateCount =
        gNdsFighterPreviewLoopP1IntegrateCount;
    gNdsFighterGCRunAllLoopP0MapCount =
        gNdsFighterPreviewLoopP0MapCount;
    gNdsFighterGCRunAllLoopP1MapCount =
        gNdsFighterPreviewLoopP1MapCount;
    gNdsFighterGCRunAllLoopPreviewWidth =
        gNdsFighterPreviewLoopPreviewWidth;
    gNdsFighterGCRunAllLoopPreviewHeight =
        gNdsFighterPreviewLoopPreviewHeight;
    gNdsFighterGCRunAllLoopPreviewPitch =
        gNdsFighterPreviewLoopPreviewPitch;
    gNdsFighterGCRunAllLoopPreviewReady =
        gNdsFighterPreviewLoopPreviewReady;
    gNdsFighterGCRunAllLoopPreviewCommitBefore =
        gNdsFighterPreviewLoopPreviewCommitBefore;
    gNdsFighterGCRunAllLoopPreviewCommitAfter =
        gNdsFighterPreviewLoopPreviewCommitAfter;
    gNdsFighterGCRunAllLoopPreviewCommitDelta =
        gNdsFighterPreviewLoopPreviewCommitDelta;
    gNdsFighterGCRunAllLoopDrawFrameCount =
        gNdsFighterPreviewLoopDrawFrameCount;
    gNdsFighterGCRunAllLoopDisplayCallbackCount =
        gNdsFighterPreviewLoopDisplayCallbackCount;
    gNdsFighterGCRunAllLoopP0DisplayCallbackCount =
        gNdsFighterPreviewLoopP0DisplayCallbackCount;
    gNdsFighterGCRunAllLoopP1DisplayCallbackCount =
        gNdsFighterPreviewLoopP1DisplayCallbackCount;
    gNdsFighterGCRunAllLoopP0CandidateCount =
        gNdsFighterPreviewLoopP0CandidateCount;
    gNdsFighterGCRunAllLoopP1CandidateCount =
        gNdsFighterPreviewLoopP1CandidateCount;
    gNdsFighterGCRunAllLoopP0DrawnDObjCount =
        gNdsFighterPreviewLoopP0DrawnDObjCount;
    gNdsFighterGCRunAllLoopP1DrawnDObjCount =
        gNdsFighterPreviewLoopP1DrawnDObjCount;
    gNdsFighterGCRunAllLoopP0PixelCount =
        gNdsFighterPreviewLoopP0PixelCount;
    gNdsFighterGCRunAllLoopP1PixelCount =
        gNdsFighterPreviewLoopP1PixelCount;
    gNdsFighterGCRunAllLoopTotalPixelCount =
        gNdsFighterPreviewLoopTotalPixelCount;
    gNdsFighterGCRunAllLoopP0ColorChecksum =
        gNdsFighterPreviewLoopP0ColorChecksum;
    gNdsFighterGCRunAllLoopP1ColorChecksum =
        gNdsFighterPreviewLoopP1ColorChecksum;
    gNdsFighterGCRunAllLoopP0ScreenXStart =
        gNdsFighterPreviewLoopP0ScreenXStart;
    gNdsFighterGCRunAllLoopP1ScreenXStart =
        gNdsFighterPreviewLoopP1ScreenXStart;
    gNdsFighterGCRunAllLoopP0ScreenXFinal =
        gNdsFighterPreviewLoopP0ScreenXFinal;
    gNdsFighterGCRunAllLoopP1ScreenXFinal =
        gNdsFighterPreviewLoopP1ScreenXFinal;
    gNdsFighterGCRunAllLoopP0ScreenXDelta =
        gNdsFighterPreviewLoopP0ScreenXDelta;
    gNdsFighterGCRunAllLoopP1ScreenXDelta =
        gNdsFighterPreviewLoopP1ScreenXDelta;
    gNdsFighterGCRunAllLoopP0ScreenYFloor =
        gNdsFighterPreviewLoopP0ScreenYFloor;
    gNdsFighterGCRunAllLoopP1ScreenYFloor =
        gNdsFighterPreviewLoopP1ScreenYFloor;
    gNdsFighterGCRunAllLoopP0ScreenYMin =
        gNdsFighterPreviewLoopP0ScreenYMin;
    gNdsFighterGCRunAllLoopP1ScreenYMin =
        gNdsFighterPreviewLoopP1ScreenYMin;
    gNdsFighterGCRunAllLoopP0ScreenRise =
        gNdsFighterPreviewLoopP0ScreenRise;
    gNdsFighterGCRunAllLoopP1ScreenRise =
        gNdsFighterPreviewLoopP1ScreenRise;
    gNdsFighterGCRunAllLoopFallDetectCount =
        gNdsFighterPreviewLoopFallDetectCount;
    gNdsFighterGCRunAllLoopLandingDetectCount =
        gNdsFighterPreviewLoopLandingDetectCount;
    gNdsFighterGCRunAllLoopSetGroundCount =
        gNdsFighterPreviewLoopSetGroundCount;
    gNdsFighterGCRunAllLoopSetAirCount =
        gNdsFighterPreviewLoopSetAirCount;
    gNdsFighterGCRunAllLoopWaitSetStatusCount =
        gNdsFighterPreviewLoopWaitSetStatusCount;
    gNdsFighterGCRunAllLoopRunBrakeEndCount =
        gNdsFighterPreviewLoopRunBrakeEndCount;
    gNdsFighterGCRunAllLoopJumpAnimEndCount =
        gNdsFighterPreviewLoopJumpAnimEndCount;
    gNdsFighterGCRunAllLoopLandingEndCount =
        gNdsFighterPreviewLoopLandingEndCount;
    gNdsFighterGCRunAllLoopDeferredInterruptCheckCount =
        gNdsFighterPreviewLoopDeferredInterruptCheckCount;
    gNdsFighterGCRunAllLoopUnexpectedStatusCount =
        gNdsFighterPreviewLoopUnexpectedStatusCount;
    gNdsFighterGCRunAllLoopDeniedStatusCount =
        gNdsFighterPreviewLoopDeniedStatusCount;
    gNdsFighterGCRunAllLoopDisplayProbeCount = 0u;
    gNdsFighterGCRunAllLoopGameplayUpdateCount = 0u;
    gNdsFighterGCRunAllLoopDrawCallCount = 0u;
    gNdsFighterGCRunAllLoopMatrixCallCount = 0u;
    gNdsFighterGCRunAllLoopRootYDriftCount =
        gNdsFighterPreviewLoopRootYDriftCount;
    gNdsFighterGCRunAllLoopGADriftCount =
        gNdsFighterPreviewLoopGADriftCount;
}

static void ndsFighterGCRunAllLoopPauseProofOwnedProcesses(void)
{
    u32 i;

    for (i = 0u; i < 2u; i++)
    {
        if (sNdsFighterSchedulerLoopProcesses[i] != NULL)
        {
            gcPauseGObjProcess(sNdsFighterSchedulerLoopProcesses[i]);
            gNdsFighterGCRunAllLoopOldProcessPauseCount++;
        }
        if (sNdsFighterControllerLoopProcesses[i] != NULL)
        {
            gcPauseGObjProcess(sNdsFighterControllerLoopProcesses[i]);
            gNdsFighterGCRunAllLoopOldProcessPauseCount++;
        }
        if (sNdsFighterPreviewLoopProcesses[i] != NULL)
        {
            gcPauseGObjProcess(sNdsFighterPreviewLoopProcesses[i]);
            gNdsFighterGCRunAllLoopOldProcessPauseCount++;
        }
    }
}

static void ndsFighterGCRunAllLoopPauseNonTargetGObjVisitor(GObj *gobj,
                                                            u32 param)
{
    GObj *target0 = sNdsFighterStructPool[0].fighter_gobj;
    GObj *target1 = sNdsFighterStructPool[1].fighter_gobj;
    (void)param;

    if (gobj == NULL)
    {
        return;
    }
    if ((gobj == target0) || (gobj == target1))
    {
        gNdsFighterGCRunAllLoopTargetProcessPreserveCount++;
        return;
    }
    gNdsFighterGCRunAllLoopNonTargetGObjVisitCount++;
    if (gobj->gobjproc_head != NULL)
    {
        gcPauseGObjProcessAll(gobj);
        gNdsFighterGCRunAllLoopNonTargetProcessPauseCount++;
    }
    gobj->flags |= GOBJ_FLAG_NORUN;
}

static void ndsFighterGCRunAllLoopPauseNonTargetProcesses(void)
{
    gcFuncGObjAll(ndsFighterGCRunAllLoopPauseNonTargetGObjVisitor, 0u);
}

static void ndsFighterGCRunAllLoopGObjProc(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    u32 slot = 2u;

    if ((fp != NULL) && (fp->player < 2))
    {
        slot = fp->player;
    }
    if ((slot >= 2u) || (fp == NULL) ||
        (ndsFighterStructIsPoolPointer(fp) == FALSE))
    {
        gNdsFighterGCRunAllLoopProcessAttachEscapeCount++;
        return;
    }
    if (slot == 0u)
    {
        gNdsFighterGCRunAllLoopP0ProcCallbackCount++;
        gNdsFighterGCRunAllLoopP0GObjProcessRunCount++;
    }
    else
    {
        gNdsFighterGCRunAllLoopP1ProcCallbackCount++;
        gNdsFighterGCRunAllLoopP1GObjProcessRunCount++;
    }
    sNdsFighterGCRunAllLoopActive = TRUE;
    ndsFighterPreviewLoopRunSlotProcess(slot, fp);
    sNdsFighterGCRunAllLoopActive = FALSE;
}

void ndsFighterMarioFoxGCRunAllLoopPrepare(void)
{
    u32 i;

    if ((ndsFighterMarioFoxGCRunAllLoopProofEnabled() == FALSE) ||
        (gNdsFighterGCRunAllLoopPrepared != 0u))
    {
        return;
    }
    if ((gNdsFighterMarioFoxPreviewLoopResult !=
            NDS_FIGHTER_MARIOFOX_PREVIEW_LOOP_PASS) ||
        (gNdsFighterMarioFoxPreviewLoopSafeResult !=
            NDS_FIGHTER_MARIOFOX_PREVIEW_LOOP_SAFE_PASS) ||
        ((gNdsFighterMarioFoxPreviewLoopMask & 0x7ffu) != 0x7ffu) ||
        (gNdsFighterMarioFoxPreviewLoopDeferredMask != 0xffu) ||
        (gNdsFighterMarioFoxPreviewLoopCount != 2u) ||
        (gNdsFighterPreviewLoopP0StatusFinal !=
            (u32)nFTCommonStatusWait) ||
        (gNdsFighterPreviewLoopP1StatusFinal !=
            (u32)nFTCommonStatusWait) ||
        (gNdsFighterPreviewLoopP0MotionFinal !=
            (u32)nFTCommonMotionWait) ||
        (gNdsFighterPreviewLoopP1MotionFinal !=
            (u32)nFTCommonMotionWait) ||
        (gNdsFighterPreviewLoopP0GAFinal != (u32)nMPKineticsGround) ||
        (gNdsFighterPreviewLoopP1GAFinal != (u32)nMPKineticsGround))
    {
        return;
    }

    bzero(sNdsFighterPreviewLoopStates,
          sizeof(sNdsFighterPreviewLoopStates));
    bzero(sNdsFighterGCRunAllLoopProcesses,
          sizeof(sNdsFighterGCRunAllLoopProcesses));
    ndsControllerPlaybackReset();
    ndsControllerPlaybackSetConnectedMask(0x3u);
    ndsControllerPlaybackSetEnabled(TRUE);
    gNdsFighterGCRunAllLoopFrameMax =
        NDS_FIGHTER_GCRUNALL_LOOP_FRAME_MAX;
    gNdsFighterGCRunAllLoopUpdateMax =
        NDS_FIGHTER_GCRUNALL_LOOP_UPDATE_MAX;
    gNdsFighterGCRunAllLoopGObjCountBefore = (u32)gcGetGObjsActiveNum();

    ndsFighterGCRunAllLoopPauseProofOwnedProcesses();
    ndsFighterGCRunAllLoopPauseNonTargetProcesses();

    for (i = 0u; i < 2u; i++)
    {
        FTStruct *fp = &sNdsFighterStructPool[i];
        GObj *fighter_gobj = fp->fighter_gobj;
        DObj *root = fp->joints[nFTPartsJointTopN];

        if ((ndsFighterStructIsPoolPointer(fp) == FALSE) ||
            (fighter_gobj == NULL) || (root == NULL))
        {
            gNdsFighterGCRunAllLoopProcessAttachEscapeCount++;
            continue;
        }
        fighter_gobj->flags |= GOBJ_FLAG_NORUN;
        ndsFighterPreviewLoopRecordStart(i, fp, root);
        sNdsFighterGCRunAllLoopProcesses[i] =
            gcAddGObjProcess(fighter_gobj,
                             ndsFighterGCRunAllLoopGObjProc,
                             nGCProcessKindFunc,
                             3);
        if (sNdsFighterGCRunAllLoopProcesses[i] == NULL)
        {
            gNdsFighterGCRunAllLoopProcessAttachEscapeCount++;
        }
        else if (i == 0u)
        {
            gNdsFighterGCRunAllLoopP0ProcessAttachCount++;
        }
        else
        {
            gNdsFighterGCRunAllLoopP1ProcessAttachCount++;
        }
    }

    ndsFighterGCRunAllLoopCopyFromPreview();
    if ((sNdsFighterGCRunAllLoopProcesses[0] != NULL) &&
        (sNdsFighterGCRunAllLoopProcesses[1] != NULL))
    {
        gNdsFighterGCRunAllLoopPrepared = 1u;
    }
}

s32 ndsFighterMarioFoxGCRunAllLoopUpdateEnabled(void)
{
    return ((ndsFighterMarioFoxGCRunAllLoopProofEnabled() != FALSE) &&
            (gNdsFighterGCRunAllLoopPrepared != 0u) &&
            (gNdsFighterMarioFoxGCRunAllLoopResult == 0u)) ? TRUE : FALSE;
}

void ndsFighterMarioFoxGCRunAllLoopRunVSBattleUpdate(void)
{
    u32 i;
    u32 mask = 0u;

    if (ndsFighterMarioFoxGCRunAllLoopUpdateEnabled() == FALSE)
    {
        return;
    }
    gNdsFighterGCRunAllLoopVSBattleUpdateCount++;

    for (i = 0u; i < 2u; i++)
    {
        ndsFighterPreviewLoopApplyPlayback(i, &sNdsFighterStructPool[i]);
    }
    ndsControllerPlaybackCommitFrame();
    syControllerReadDeviceData();
    gNdsFighterGCRunAllLoopSYReadCount++;
    syControllerUpdateGlobalData();
    gNdsFighterGCRunAllLoopSYUpdateCount++;

    sNdsFighterGCRunAllLoopActive = TRUE;
    gcRunAll();
    sNdsFighterGCRunAllLoopActive = FALSE;
    gNdsFighterGCRunAllLoopRunAllCount++;

    for (i = 0u; i < 2u; i++)
    {
        FTStruct *fp = &sNdsFighterStructPool[i];
        DObj *root = fp->joints[nFTPartsJointTopN];
        ndsFighterPreviewLoopRecordFinal(i, fp, root);
    }

    if (((gNdsFighterGCRunAllLoopVSBattleUpdateCount %
            NDS_FIGHTER_GCRUNALL_LOOP_DRAW_FRAME_INTERVAL) == 0u) ||
        ((gNdsFighterPreviewLoopP0Completed == 1u) &&
         (gNdsFighterPreviewLoopP1Completed == 1u)))
    {
        ndsFighterPreviewLoopDrawKeyframe();
    }
    ndsFighterGCRunAllLoopCopyFromPreview();

    if ((gNdsFighterGCRunAllLoopP0Completed == 1u) &&
        (gNdsFighterGCRunAllLoopP1Completed == 1u))
    {
        gNdsFighterGCRunAllLoopRootYDriftCount = 0u;
        gNdsFighterGCRunAllLoopGADriftCount = 0u;
        if ((gNdsFighterGCRunAllLoopP0StatusFinal !=
                (u32)nFTCommonStatusWait) ||
            (gNdsFighterGCRunAllLoopP1StatusFinal !=
                (u32)nFTCommonStatusWait) ||
            (gNdsFighterGCRunAllLoopP0MotionFinal !=
                (u32)nFTCommonMotionWait) ||
            (gNdsFighterGCRunAllLoopP1MotionFinal !=
                (u32)nFTCommonMotionWait) ||
            (gNdsFighterGCRunAllLoopP0GAFinal !=
                (u32)nMPKineticsGround) ||
            (gNdsFighterGCRunAllLoopP1GAFinal !=
                (u32)nMPKineticsGround))
        {
            gNdsFighterGCRunAllLoopGADriftCount++;
        }
        if ((gNdsFighterGCRunAllLoopP0FloorOK != 1u) ||
            (gNdsFighterGCRunAllLoopP1FloorOK != 1u))
        {
            gNdsFighterGCRunAllLoopRootYDriftCount++;
        }
    }

    if ((gNdsFighterMarioFoxPreviewLoopResult ==
            NDS_FIGHTER_MARIOFOX_PREVIEW_LOOP_PASS) &&
        (gNdsFighterMarioFoxPreviewLoopSafeResult ==
            NDS_FIGHTER_MARIOFOX_PREVIEW_LOOP_SAFE_PASS))
    {
        mask |= 1u << 0;
    }
    if ((gNdsControllerPlaybackEnabled == 1u) &&
        ((gNdsControllerPlaybackConnectedMask & 0x3u) == 0x3u) &&
        (gNdsControllerPlaybackFrameCount > 0u) &&
        (gNdsControllerPlaybackReadCount > 0u) &&
        (gNdsControllerLiveReadCount == 0u) &&
        (gNdsFighterGCRunAllLoopSYReadCount ==
            gNdsFighterGCRunAllLoopSYUpdateCount))
    {
        mask |= 1u << 1;
    }
    if ((gNdsFighterGCRunAllLoopPrepared == 1u) &&
        (gNdsFighterGCRunAllLoopTaskmanUpdateCount > 0u) &&
        (gNdsFighterGCRunAllLoopVSBattleUpdateCount > 0u) &&
        (gNdsFighterGCRunAllLoopRunAllCount > 0u))
    {
        mask |= 1u << 2;
    }
    if ((gNdsFighterGCRunAllLoopP0ProcessAttachCount == 1u) &&
        (gNdsFighterGCRunAllLoopP1ProcessAttachCount == 1u) &&
        (gNdsFighterGCRunAllLoopP0GObjProcessRunCount > 0u) &&
        (gNdsFighterGCRunAllLoopP1GObjProcessRunCount > 0u) &&
        (gNdsFighterGCRunAllLoopP0ProcCallbackCount ==
            gNdsFighterGCRunAllLoopP0GObjProcessRunCount) &&
        (gNdsFighterGCRunAllLoopP1ProcCallbackCount ==
            gNdsFighterGCRunAllLoopP1GObjProcessRunCount))
    {
        mask |= 1u << 3;
    }
    if ((gNdsFighterGCRunAllLoopP0PlaybackApplyCount > 0u) &&
        (gNdsFighterGCRunAllLoopP1PlaybackApplyCount > 0u) &&
        (gNdsFighterGCRunAllLoopP0ControllerToFTInputCount > 0u) &&
        (gNdsFighterGCRunAllLoopP1ControllerToFTInputCount > 0u) &&
        (gNdsFighterGCRunAllLoopP0DirectFTInputWriteCount == 0u) &&
        (gNdsFighterGCRunAllLoopP1DirectFTInputWriteCount == 0u) &&
        (gNdsFighterGCRunAllLoopP0ButtonTapMask != 0u) &&
        (gNdsFighterGCRunAllLoopP1ButtonTapMask != 0u) &&
        (gNdsFighterGCRunAllLoopP0ButtonHoldMask != 0u) &&
        (gNdsFighterGCRunAllLoopP1ButtonHoldMask != 0u) &&
        (gNdsFighterGCRunAllLoopP0DashTapEligibleCount > 0u) &&
        (gNdsFighterGCRunAllLoopP1DashTapEligibleCount > 0u) &&
        (gNdsFighterGCRunAllLoopP0JumpButtonTapCount > 0u) &&
        (gNdsFighterGCRunAllLoopP1JumpButtonTapCount > 0u))
    {
        mask |= 1u << 4;
    }
    if ((gNdsFighterGCRunAllLoopP0Completed == 1u) &&
        (gNdsFighterGCRunAllLoopP1Completed == 1u) &&
        (gNdsFighterGCRunAllLoopP0FrameCount > 0u) &&
        (gNdsFighterGCRunAllLoopP1FrameCount > 0u) &&
        (gNdsFighterGCRunAllLoopP0FrameCount <=
            gNdsFighterGCRunAllLoopFrameMax) &&
        (gNdsFighterGCRunAllLoopP1FrameCount <=
            gNdsFighterGCRunAllLoopFrameMax))
    {
        mask |= 1u << 5;
    }
    if (((gNdsFighterGCRunAllLoopP0StatusVisitMask &
            NDS_FIGHTER_GCRUNALL_LOOP_STATUS_MASK_REQUIRED) ==
            NDS_FIGHTER_GCRUNALL_LOOP_STATUS_MASK_REQUIRED) &&
        ((gNdsFighterGCRunAllLoopP1StatusVisitMask &
            NDS_FIGHTER_GCRUNALL_LOOP_STATUS_MASK_REQUIRED) ==
            NDS_FIGHTER_GCRUNALL_LOOP_STATUS_MASK_REQUIRED))
    {
        mask |= 1u << 6;
    }
    if (((gNdsFighterGCRunAllLoopP0TransitionMask &
            NDS_FIGHTER_GCRUNALL_LOOP_TRANSITION_MASK_REQUIRED) ==
            NDS_FIGHTER_GCRUNALL_LOOP_TRANSITION_MASK_REQUIRED) &&
        ((gNdsFighterGCRunAllLoopP1TransitionMask &
            NDS_FIGHTER_GCRUNALL_LOOP_TRANSITION_MASK_REQUIRED) ==
            NDS_FIGHTER_GCRUNALL_LOOP_TRANSITION_MASK_REQUIRED))
    {
        mask |= 1u << 7;
    }
    if ((gNdsFighterGCRunAllLoopP0InterruptCount > 0u) &&
        (gNdsFighterGCRunAllLoopP1InterruptCount > 0u) &&
        (gNdsFighterGCRunAllLoopP0PhysicsCount > 0u) &&
        (gNdsFighterGCRunAllLoopP1PhysicsCount > 0u) &&
        (gNdsFighterGCRunAllLoopP0IntegrateCount > 0u) &&
        (gNdsFighterGCRunAllLoopP1IntegrateCount > 0u) &&
        (gNdsFighterGCRunAllLoopP0MapCount > 0u) &&
        (gNdsFighterGCRunAllLoopP1MapCount > 0u))
    {
        mask |= 1u << 8;
    }
    if ((gNdsFighterGCRunAllLoopP0RootDeltaXMilli != 0) &&
        (gNdsFighterGCRunAllLoopP1RootDeltaXMilli != 0) &&
        (gNdsFighterGCRunAllLoopP0RootRiseMilli > 0) &&
        (gNdsFighterGCRunAllLoopP1RootRiseMilli > 0) &&
        (gNdsFighterGCRunAllLoopP0RootDirectionOK == 1u) &&
        (gNdsFighterGCRunAllLoopP1RootDirectionOK == 1u))
    {
        mask |= 1u << 9;
    }
    if ((gNdsFighterGCRunAllLoopPreviewReady != 0u) &&
        (gNdsFighterGCRunAllLoopPreviewCommitDelta >=
            NDS_FIGHTER_GCRUNALL_LOOP_DRAW_FRAME_MIN) &&
        (gNdsFighterGCRunAllLoopDrawFrameCount >=
            NDS_FIGHTER_GCRUNALL_LOOP_DRAW_FRAME_MIN) &&
        (gNdsFighterGCRunAllLoopDisplayCallbackCount >=
            (NDS_FIGHTER_GCRUNALL_LOOP_DRAW_FRAME_MIN * 2u)) &&
        (gNdsFighterGCRunAllLoopP0CandidateCount >= 14u) &&
        (gNdsFighterGCRunAllLoopP1CandidateCount >= 18u) &&
        (gNdsFighterGCRunAllLoopP0PixelCount > 0u) &&
        (gNdsFighterGCRunAllLoopP1PixelCount > 0u) &&
        (gNdsFighterGCRunAllLoopP0ColorChecksum != 0u) &&
        (gNdsFighterGCRunAllLoopP1ColorChecksum != 0u) &&
        (gNdsFighterGCRunAllLoopP0ScreenXDelta != 0) &&
        (gNdsFighterGCRunAllLoopP1ScreenXDelta != 0) &&
        (gNdsFighterGCRunAllLoopP0ScreenRise > 0) &&
        (gNdsFighterGCRunAllLoopP1ScreenRise > 0))
    {
        mask |= 1u << 10;
    }
    if ((gNdsFighterGCRunAllLoopP0StatusFinal ==
            (u32)nFTCommonStatusWait) &&
        (gNdsFighterGCRunAllLoopP1StatusFinal ==
            (u32)nFTCommonStatusWait) &&
        (gNdsFighterGCRunAllLoopP0MotionFinal ==
            (u32)nFTCommonMotionWait) &&
        (gNdsFighterGCRunAllLoopP1MotionFinal ==
            (u32)nFTCommonMotionWait) &&
        (gNdsFighterGCRunAllLoopP0GAFinal == (u32)nMPKineticsGround) &&
        (gNdsFighterGCRunAllLoopP1GAFinal == (u32)nMPKineticsGround) &&
        (gNdsFighterGCRunAllLoopP0FloorOK == 1u) &&
        (gNdsFighterGCRunAllLoopP1FloorOK == 1u))
    {
        mask |= 1u << 11;
    }

    gNdsFighterGCRunAllLoopGObjCountAfter = (u32)gcGetGObjsActiveNum();
    gNdsFighterGCRunAllLoopGObjDelta =
        (gNdsFighterGCRunAllLoopGObjCountAfter >=
         gNdsFighterGCRunAllLoopGObjCountBefore) ?
        (gNdsFighterGCRunAllLoopGObjCountAfter -
         gNdsFighterGCRunAllLoopGObjCountBefore) :
        (gNdsFighterGCRunAllLoopGObjCountBefore -
         gNdsFighterGCRunAllLoopGObjCountAfter);

    if ((gNdsFighterGCRunAllLoopGObjDelta == 0u) &&
        (gNdsFighterGCRunAllLoopUnexpectedStatusCount == 0u) &&
        (gNdsFighterGCRunAllLoopDeniedStatusCount == 0u) &&
        (gNdsFighterGCRunAllLoopProcessAttachEscapeCount == 0u) &&
        (gNdsFighterGCRunAllLoopDisplayProbeCount == 0u) &&
        (gNdsFighterGCRunAllLoopGameplayUpdateCount == 0u) &&
        (gNdsFighterGCRunAllLoopDrawCallCount == 0u) &&
        (gNdsFighterGCRunAllLoopMatrixCallCount == 0u) &&
        (gNdsFighterGCRunAllLoopRootYDriftCount == 0u) &&
        (gNdsFighterGCRunAllLoopGADriftCount == 0u) &&
        (gNdsFighterGCRunAllLoopOldProcessPauseCount > 0u) &&
        (gNdsFighterGCRunAllLoopNonTargetGObjVisitCount > 0u) &&
        (gNdsFighterGCRunAllLoopTargetProcessPreserveCount >= 2u))
    {
        mask |= 1u << 12;
    }

    gNdsFighterMarioFoxGCRunAllLoopMask = mask;
    gNdsFighterMarioFoxGCRunAllLoopDeferredMask = 0xffu;
    gNdsFighterMarioFoxGCRunAllLoopCount =
        gNdsFighterGCRunAllLoopP0Completed +
        gNdsFighterGCRunAllLoopP1Completed;

    if ((mask & 0x1fffu) == 0x1fffu)
    {
        gNdsFighterMarioFoxGCRunAllLoopResult =
            NDS_FIGHTER_MARIOFOX_GCRUNALL_LOOP_PASS;
        gNdsFighterMarioFoxGCRunAllLoopSafeResult =
            NDS_FIGHTER_MARIOFOX_GCRUNALL_LOOP_SAFE_PASS;
    }
}

#if NDS_IMPORT_BATTLESHIP_FTMANAGER
#define NDS_FIGHTER_NATURAL_MOTION_WAIT_FRAMES_REQUIRED 300u
#define NDS_FIGHTER_NATURAL_MOTION_WALK_FRAMES_REQUIRED 8u
#define NDS_FIGHTER_NATURAL_STAGE_SIDE_MASK_REQUIRED 0x24fu
#define NDS_FIGHTER_NATURAL_COMBAT_SETTLE_FRAMES_REQUIRED 10u
#define NDS_FIGHTER_NATURAL_COMBAT_DASH_FRAMES_REQUIRED 2u
#define NDS_FIGHTER_NATURAL_COMBAT_RUN_FRAMES_REQUIRED 8u
#define NDS_FIGHTER_NATURAL_COMBAT_RUNBRAKE_FRAMES_REQUIRED 2u
#define NDS_FIGHTER_NATURAL_COMBAT_TURN_FRAMES_REQUIRED 1u
#define NDS_FIGHTER_NATURAL_COMBAT_GUARD_FRAMES_REQUIRED 10u
#define NDS_FIGHTER_NATURAL_COMBAT_APPROACH_DASH_RANGE 1000.0F
#define NDS_FIGHTER_NATURAL_COMBAT_APPROACH_STOP_RANGE 240.0F
#define NDS_FIGHTER_NATURAL_COMBAT_APPROACH_FLOOR_Y_RANGE 100.0F
#define NDS_FIGHTER_NATURAL_COMBAT_APPROACH_RANGE_STEP 15.0F
#define NDS_FIGHTER_NATURAL_COMBAT_APPROACH_RANGE_MIN 50.0F
#define NDS_FIGHTER_NATURAL_MOVESET_SAFE_RANGE 80.0F
#define NDS_FIGHTER_NATURAL_MOVESET_GRAB_STOP_RANGE (260.0F + 30.0F)
#define NDS_FIGHTER_NATURAL_PROJECTILE_STOP_RANGE \
    ((112.5F * 2.0F) + 350.0F)
#define NDS_FIGHTER_NATURAL_PROJECTILE_CENTER_RANGE 300.0F
#define NDS_FIGHTER_NATURAL_COMBAT_ATTACK_NEUTRAL_FRAMES 4u
#define NDS_FIGHTER_NATURAL_COMBAT_ATTACK_TIMEOUT 45u
#define NDS_FIGHTER_NATURAL_COMBAT_ATTACK_RETRY_MAX 6u
#define NDS_FIGHTER_PROJECTILE_FIRE_TIMEOUT 120u
#define NDS_FIGHTER_PROJECTILE_OBSERVE_TIMEOUT 180u
#define NDS_FIGHTER_PROJECTILE_WEAPON_FRAMES_REQUIRED 3u
#define NDS_FIGHTER_REFLECTOR_PROOF_PASS 0x52464c43u
#define NDS_FIGHTER_NATURAL_COMBAT_PHASE_TIMEOUT 600u
#define NDS_FIGHTER_BATTLE_PLAYABLE_PHASE_TIMEOUT 3600u
#define NDS_FIGHTER_NATURAL_MOVESET_PHASE_TIMEOUT 1200u
#define NDS_FIGHTER_BATTLE_PLAYABLE_WAIT_AFTER_REBIRTH_REQUIRED 8u
#define NDS_FIGHTER_BATTLE_PLAYABLE_MASK_ALL 0xffu
#define NDS_FIGHTER_NATURAL_MOVESET_MASK_ALL 0x7ffu
#define NDS_FIGHTER_SPECIALS_MARIO_HI_MASK 0x000fu
#define NDS_FIGHTER_SPECIALS_MARIO_LW_MASK 0x0070u
#define NDS_FIGHTER_SPECIALS_FOX_HI_MASK 0x0f80u
#define NDS_FIGHTER_NATURAL_SPECIAL_SETTLE_FRAMES_REQUIRED 60u

/* Scripted input phases for the natural original-runtime combat chain.
 * Input only flows through controller playback into the original
 * syController/ftkey path; state is observed, never written. */
enum {
    nNDSNaturalCombatPhaseWait = 0,
    nNDSNaturalCombatPhaseWalk,
    nNDSNaturalCombatPhaseSettleWalk,
    nNDSNaturalCombatPhaseDashRun,
    nNDSNaturalCombatPhaseRunBrake,
    nNDSNaturalCombatPhaseSettleRun,
    nNDSNaturalCombatPhaseTurn,
    nNDSNaturalCombatPhaseSettleTurn,
    nNDSNaturalCombatPhaseApproach,
    nNDSNaturalCombatPhaseSettleApproach,
    nNDSNaturalCombatPhaseAttack,
    nNDSNaturalCombatPhaseSettleDamage,
    nNDSNaturalCombatPhaseGuard,
    nNDSNaturalCombatPhaseGuardOff,
    nNDSNaturalCombatPhaseDone,
    nNDSNaturalCombatPhaseBattlePlayableKOExit,
    nNDSNaturalCombatPhaseBattlePlayableDead,
    nNDSNaturalCombatPhaseBattlePlayableRebirth,
    nNDSNaturalCombatPhaseBattlePlayableRecover,
    nNDSNaturalCombatPhaseBattlePlayableDone,
    nNDSNaturalCombatPhaseProjectileSettle,
    nNDSNaturalCombatPhaseProjectileFire,
    nNDSNaturalCombatPhaseProjectileObserve
};

#if NDS_IMPORT_BATTLESHIP_NORMAL_MOVESET
enum {
    nNDSNaturalMovesetPhaseIdle = 0,
    nNDSNaturalMovesetPhaseTiltS3,
    nNDSNaturalMovesetPhaseSettleTiltS3,
    nNDSNaturalMovesetPhaseTiltHi3,
    nNDSNaturalMovesetPhaseSettleTiltHi3,
    nNDSNaturalMovesetPhaseTiltLw3,
    nNDSNaturalMovesetPhaseSettleTiltLw3,
    nNDSNaturalMovesetPhaseSmashS4,
    nNDSNaturalMovesetPhaseSettleSmashS4,
    nNDSNaturalMovesetPhaseAerialJump,
    nNDSNaturalMovesetPhaseAerialAttack,
    nNDSNaturalMovesetPhaseSettleAerial,
    nNDSNaturalMovesetPhaseGrabCatch,
    nNDSNaturalMovesetPhaseGrabThrow,
    nNDSNaturalMovesetPhaseSettleThrow,
    nNDSNaturalMovesetPhaseDone
};
#endif

#if NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_HI || \
    NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_LW || \
    NDS_IMPORT_BATTLESHIP_FOX_SPECIAL_HI
enum {
    nNDSNaturalSpecialsPhaseIdle = 0,
    nNDSNaturalSpecialsPhaseMarioHi,
    nNDSNaturalSpecialsPhaseSettleMarioHi,
    nNDSNaturalSpecialsPhaseMarioLw,
    nNDSNaturalSpecialsPhaseSettleMarioLw,
    nNDSNaturalSpecialsPhaseFoxHi,
    nNDSNaturalSpecialsPhaseSettleFoxHi,
    nNDSNaturalSpecialsPhaseDone
};
#endif

typedef struct NDSFighterNaturalMotionState {
    f32 first_wait_anim;
    f32 prev_anim;
    u32 has_wait_anim;
    u32 wait_frames;
    u32 anim_advance_count;
    u32 valid_joint_count;
    u32 walk_frames;
    u32 dash_frames;
    u32 run_frames;
    u32 runbrake_frames;
    u32 turn_frames;
    u32 hitlag_frames;
} NDSFighterNaturalMotionState;

static NDSFighterNaturalMotionState sNdsFighterNaturalMotionStates[2];
static u32 sNdsFighterNaturalMotionWalkInputActive;
static u32 sNdsNaturalCombatPhase;
static u32 sNdsNaturalCombatPhaseFrames;
static u32 sNdsNaturalCombatSettleFrames;
static u32 sNdsNaturalCombatAttackerSlot;
static u32 sNdsNaturalCombatVictimSlot;
static u32 sNdsNaturalCombatAttackFrames;
static u32 sNdsNaturalCombatAttackPressed;
static u32 sNdsNaturalCombatPassPressed;
static f32 sNdsNaturalCombatApproachStopRange;
static u32 sNdsNaturalCombatVictimStartPercent;
static f32 sNdsNaturalCombatVictimHitPosX;
static u32 sNdsNaturalCombatVictimHitSeen;
static u32 sNdsBattlePlayableVictimStockStart;
static u32 sNdsBattlePlayableBattleStockStart;
static u32 sNdsBattlePlayableFallsStart;
static u32 sNdsBattlePlayableRebirthSeen;
static s8 sNdsBattlePlayableKOStickX;
static u32 sNdsNaturalProjectileActorSlot;
static u32 sNdsNaturalProjectileButtonPressed;
static u32 sNdsNaturalProjectileExpectedKind;
static u32 sNdsNaturalProjectileKORecoveryActive;
#if NDS_IMPORT_BATTLESHIP_FOX_REFLECTOR
static u32 sNdsNaturalReflectorFoxSlot;
static u32 sNdsNaturalReflectorProjectileSlot;
static u32 sNdsNaturalReflectorButtonPressed;
#endif
#if NDS_IMPORT_BATTLESHIP_NORMAL_MOVESET
static u32 sNdsNaturalMovesetPhase;
static u32 sNdsNaturalMovesetPhaseFrames;
static u32 sNdsNaturalMovesetSettleFrames;
static u32 sNdsNaturalMovesetDone;
static u32 sNdsNaturalMovesetKORecoveryActive;
static u32 sNdsNaturalMovesetKORecoveryPhase;
#endif
#if NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_HI || \
    NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_LW || \
    NDS_IMPORT_BATTLESHIP_FOX_SPECIAL_HI
static u32 sNdsNaturalSpecialsPhase;
static u32 sNdsNaturalSpecialsPhaseFrames;
static u32 sNdsNaturalSpecialsDone;
static u32 sNdsNaturalSpecialsButtonPressed;
#endif

static sb32 ndsFighterBattlePlayableProofEnabled(void)
{
#if NDS_IMPORT_BATTLESHIP_BATTLE_PLAYABLE && \
    (NDS_DEV_SCENE_HARNESS == NDS_DEV_SCENE_HARNESS_BATTLE_PLAYABLE)
#if NDS_HARNESS_FAST_LOGIC
    return TRUE;
#else
    return FALSE;
#endif
#else
    return FALSE;
#endif
}

static sb32 ndsFighterNaturalProjectileProofEnabled(void)
{
#if NDS_IMPORT_BATTLESHIP_WEAPON_MANAGER && \
    (NDS_IMPORT_BATTLESHIP_MARIO_FIREBALL || \
     NDS_IMPORT_BATTLESHIP_FOX_BLASTER)
    return (ndsFighterBattlePlayableProofEnabled() != FALSE) ? TRUE : FALSE;
#else
    return FALSE;
#endif
}

#if NDS_IMPORT_BATTLESHIP_FOX_REFLECTOR
static sb32 ndsFighterNaturalReflectorProofEnabled(void)
{
    return ndsFighterNaturalProjectileProofEnabled();
}
#endif

#if NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_HI || \
    NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_LW || \
    NDS_IMPORT_BATTLESHIP_FOX_SPECIAL_HI
static sb32 ndsFighterNaturalSpecialsProofEnabled(void)
{
    return (ndsFighterBattlePlayableProofEnabled() != FALSE) ? TRUE : FALSE;
}
#endif

static sb32 ndsFighterMarioFoxNaturalMotionProofEnabled(void)
{
#if NDS_IMPORT_BATTLESHIP_FTMANAGER && NDS_MARIOFOX_DASH_RUN_HARNESS
    return TRUE;
#else
    if (ndsFighterBattlePlayableProofEnabled() != FALSE)
    {
        return TRUE;
    }
    return (ndsFighterMarioFoxGCRunAllLoopProofEnabled() != FALSE) ? TRUE :
                                                                    FALSE;
#endif
}

static sb32 ndsFighterNaturalCombatMovementOnlyProofEnabled(void)
{
#if (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_DASH_RUN) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_DASH_RUN)
    return TRUE;
#else
    return FALSE;
#endif
}

static sb32 ndsFighterNaturalCombatLiveHitProofEnabled(void)
{
#if (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_GCRUNALL_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_GCRUNALL_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_BATTLE_MARIOFOX_STAGE_MPLIVEHIT_STATUS_LOOP) || \
    (NDS_DEV_SCENE_HARNESS == \
        NDS_DEV_SCENE_HARNESS_MENU_CHAIN_MARIOFOX_STAGE_MPLIVEHIT_STATUS_LOOP)
    return TRUE;
#else
    return FALSE;
#endif
}

static sb32 ndsFighterNaturalCombatStageSideProofEnabled(void)
{
#if NDS_MARIOFOX_STAGE_GCDRAWALL_LOOP_HARNESS && \
    !NDS_MARIOFOX_STAGE_MPLIVEHIT_STATUS_LOOP_HARNESS
    return TRUE;
#else
    return FALSE;
#endif
}

static sb32 ndsFighterNaturalMotionStatusIsWalk(s32 status_id)
{
    return ((status_id == nFTCommonStatusWalkSlow) ||
            (status_id == nFTCommonStatusWalkMiddle) ||
            (status_id == nFTCommonStatusWalkFast)) ? TRUE : FALSE;
}

static sb32 ndsFighterNaturalMotionMotionIsWalk(s32 motion_id)
{
    return ((motion_id == nFTCommonMotionWalkSlow) ||
            (motion_id == nFTCommonMotionWalkMiddle) ||
            (motion_id == nFTCommonMotionWalkFast)) ? TRUE : FALSE;
}

static sb32 ndsFighterNaturalMotionHasValidJoints(FTStruct *fp)
{
    return ((fp != NULL) &&
            (fp->fighter_gobj != NULL) &&
            (fp->joints[nFTPartsJointTopN] != NULL) &&
            (fp->joints[nFTPartsJointCommonStart] != NULL)) ? TRUE : FALSE;
}

static void ndsFighterNaturalMotionPauseNonTargetVisitor(GObj *gobj,
                                                         u32 param)
{
    GObj *target0 = ndsFighterManagerLiveGObj(0u);
    GObj *target1 = ndsFighterManagerLiveGObj(1u);
    (void)param;

    if (gobj == NULL)
    {
        return;
    }
    if ((gobj == target0) || (gobj == target1))
    {
        return;
    }
#if NDS_IMPORT_BATTLESHIP_BATTLE_PLAYABLE
    {
        extern GObj *gGMCameraGObj;

        if (gobj == gGMCameraGObj)
        {
            return;
        }
    }
#endif
    if (gobj->gobjproc_head != NULL)
    {
        gcPauseGObjProcessAll(gobj);
    }
    gobj->flags |= GOBJ_FLAG_NORUN;
}

static void ndsFighterNaturalMotionRecordSlot(u32 slot, FTStruct *fp)
{
    NDSFighterNaturalMotionState *state;
    f32 anim_frame;

    if ((slot >= ARRAY_COUNT(sNdsFighterNaturalMotionStates)) ||
        (fp == NULL) ||
        (fp->fighter_gobj == NULL))
    {
        gNdsFighterNaturalMotionUnsafeCount++;
        return;
    }
    state = &sNdsFighterNaturalMotionStates[slot];
    anim_frame = fp->fighter_gobj->anim_frame;

    if (slot == 0u)
    {
        gNdsFighterNaturalMotionP0StatusFinal = (u32)fp->status_id;
        gNdsFighterNaturalMotionP0MotionFinal = (u32)fp->motion_id;
        gNdsFighterNaturalMotionP0GAFinal = (u32)fp->ga;
        gNdsFighterNaturalMotionP0AnimFinalBits =
            ndsFloatBits(anim_frame);
    }
    else
    {
        gNdsFighterNaturalMotionP1StatusFinal = (u32)fp->status_id;
        gNdsFighterNaturalMotionP1MotionFinal = (u32)fp->motion_id;
        gNdsFighterNaturalMotionP1GAFinal = (u32)fp->ga;
        gNdsFighterNaturalMotionP1AnimFinalBits =
            ndsFloatBits(anim_frame);
    }

    if (ndsFighterNaturalMotionHasValidJoints(fp) != FALSE)
    {
        state->valid_joint_count++;
    }
    if ((fp->status_id == nFTCommonStatusWait) &&
        (fp->motion_id == nFTCommonMotionWait))
    {
        if (state->has_wait_anim == 0u)
        {
            state->first_wait_anim = anim_frame;
            state->prev_anim = anim_frame;
            state->has_wait_anim = 1u;
            if (slot == 0u)
            {
                gNdsFighterNaturalMotionP0AnimStartBits =
                    ndsFloatBits(anim_frame);
            }
            else
            {
                gNdsFighterNaturalMotionP1AnimStartBits =
                    ndsFloatBits(anim_frame);
            }
        }
        else if (anim_frame != state->prev_anim)
        {
            state->anim_advance_count++;
            state->prev_anim = anim_frame;
        }
        state->wait_frames++;
    }
    else if ((sNdsFighterNaturalMotionWalkInputActive != 0u) &&
             (ndsFighterNaturalMotionStatusIsWalk(fp->status_id) != FALSE) &&
             (ndsFighterNaturalMotionMotionIsWalk(fp->motion_id) != FALSE))
    {
        state->walk_frames++;
        if (slot == 0u)
        {
            gNdsFighterNaturalMotionP0WalkStatus = (u32)fp->status_id;
            gNdsFighterNaturalMotionP0WalkMotion = (u32)fp->motion_id;
        }
        else
        {
            gNdsFighterNaturalMotionP1WalkStatus = (u32)fp->status_id;
            gNdsFighterNaturalMotionP1WalkMotion = (u32)fp->motion_id;
        }
    }

    if (fp->status_id == nFTCommonStatusDash)
    {
        state->dash_frames++;
    }
    else if (fp->status_id == nFTCommonStatusRun)
    {
        state->run_frames++;
    }
    else if (fp->status_id == nFTCommonStatusRunBrake)
    {
        state->runbrake_frames++;
    }
    else if ((fp->status_id == nFTCommonStatusTurn) ||
             (fp->status_id == nFTCommonStatusTurnRun))
    {
        state->turn_frames++;
    }
    if (fp->hitlag_tics > 0u)
    {
        state->hitlag_frames++;
    }

    if (slot == 0u)
    {
        gNdsFighterNaturalMotionP0WaitFrameCount = state->wait_frames;
        gNdsFighterNaturalMotionP0AnimAdvanceCount =
            state->anim_advance_count;
        gNdsFighterNaturalMotionP0ValidJointCount =
            state->valid_joint_count;
        gNdsFighterNaturalMotionP0WalkFrameCount = state->walk_frames;
        gNdsFighterNaturalCombatP0DashFrames = state->dash_frames;
        gNdsFighterNaturalCombatP0RunFrames = state->run_frames;
        gNdsFighterNaturalCombatP0RunBrakeFrames = state->runbrake_frames;
        gNdsFighterNaturalCombatP0TurnFrames = state->turn_frames;
        gNdsFighterNaturalCombatP0HitlagFrames = state->hitlag_frames;
    }
    else
    {
        gNdsFighterNaturalMotionP1WaitFrameCount = state->wait_frames;
        gNdsFighterNaturalMotionP1AnimAdvanceCount =
            state->anim_advance_count;
        gNdsFighterNaturalMotionP1ValidJointCount =
            state->valid_joint_count;
        gNdsFighterNaturalMotionP1WalkFrameCount = state->walk_frames;
        gNdsFighterNaturalCombatP1DashFrames = state->dash_frames;
        gNdsFighterNaturalCombatP1RunFrames = state->run_frames;
        gNdsFighterNaturalCombatP1RunBrakeFrames = state->runbrake_frames;
        gNdsFighterNaturalCombatP1TurnFrames = state->turn_frames;
        gNdsFighterNaturalCombatP1HitlagFrames = state->hitlag_frames;
    }
}

static f32 ndsFighterNaturalCombatPosX(FTStruct *fp)
{
    if ((fp == NULL) || (fp->coll_data.p_translate == NULL))
    {
        return 0.0F;
    }
    return fp->coll_data.p_translate->x;
}

static sb32 ndsFighterNaturalCombatStatusIsDamage(s32 status_id)
{
    return ((status_id >= nFTCommonStatusDamageStart) &&
            (status_id <= nFTCommonStatusDamageEnd)) ? TRUE : FALSE;
}

static sb32 ndsFighterNaturalCombatStatusIsGuard(s32 status_id)
{
    return ((status_id >= nFTCommonStatusGuardStart) &&
            (status_id <= nFTCommonStatusGuardEnd)) ? TRUE : FALSE;
}

static u32 ndsFighterNaturalProjectileSelectSlot(FTStruct *fp[2])
{
    u32 i;

#if NDS_IMPORT_BATTLESHIP_FOX_REFLECTOR
    for (i = 0u; i < 2u; i++)
    {
        if (fp[i]->fkind == nFTKindMario)
        {
            return i;
        }
    }
#endif
#if NDS_IMPORT_BATTLESHIP_FOX_BLASTER
    for (i = 0u; i < 2u; i++)
    {
        if (fp[i]->fkind == nFTKindFox)
        {
            return i;
        }
    }
#endif
#if NDS_IMPORT_BATTLESHIP_MARIO_FIREBALL
    for (i = 0u; i < 2u; i++)
    {
        if (fp[i]->fkind == nFTKindMario)
        {
            return i;
        }
    }
#endif
    return sNdsNaturalCombatAttackerSlot;
}

#if NDS_IMPORT_BATTLESHIP_FOX_REFLECTOR
static u32 ndsFighterNaturalReflectorSelectFoxSlot(FTStruct *fp[2])
{
    u32 i;

    for (i = 0u; i < 2u; i++)
    {
        if (fp[i]->fkind == nFTKindFox)
        {
            return i;
        }
    }
    return 1u - sNdsNaturalProjectileActorSlot;
}
#endif

static u32 ndsFighterNaturalProjectileExpectedKind(FTStruct *fp)
{
#if NDS_IMPORT_BATTLESHIP_FOX_REFLECTOR
    if ((fp != NULL) && (fp->fkind == nFTKindMario))
    {
        return nWPKindFireball;
    }
#endif
#if NDS_IMPORT_BATTLESHIP_FOX_BLASTER
    if ((fp != NULL) && (fp->fkind == nFTKindFox))
    {
        return nWPKindBlaster;
    }
#endif
    return nWPKindFireball;
}

static sb32 ndsFighterNaturalProjectileStatusIsSpecialN(FTStruct *fp)
{
    if (fp == NULL)
    {
        return FALSE;
    }
    if (fp->fkind == nFTKindFox)
    {
        return ((fp->status_id == nFTFoxStatusSpecialN) ||
                (fp->status_id == nFTFoxStatusSpecialAirN)) ? TRUE : FALSE;
    }
    if (fp->fkind == nFTKindMario)
    {
        return ((fp->status_id == nFTMarioStatusSpecialN) ||
                (fp->status_id == nFTMarioStatusSpecialAirN)) ? TRUE : FALSE;
    }
    return FALSE;
}

static sb32 ndsFighterNaturalProjectileStatusIsSpecialLw(FTStruct *fp)
{
    if (fp == NULL)
    {
        return FALSE;
    }
    if (fp->fkind == nFTKindFox)
    {
        return ((fp->status_id == nFTFoxStatusSpecialLwStart) ||
                (fp->status_id == nFTFoxStatusSpecialLwHit) ||
                (fp->status_id == nFTFoxStatusSpecialLwEnd) ||
                (fp->status_id == nFTFoxStatusSpecialLwLoop) ||
                (fp->status_id == nFTFoxStatusSpecialLwTurn) ||
                (fp->status_id == nFTFoxStatusSpecialAirLwStart) ||
                (fp->status_id == nFTFoxStatusSpecialAirLwHit) ||
                (fp->status_id == nFTFoxStatusSpecialAirLwEnd) ||
                (fp->status_id == nFTFoxStatusSpecialAirLwLoop) ||
                (fp->status_id == nFTFoxStatusSpecialAirLwTurn)) ? TRUE :
                                                                  FALSE;
    }
    return FALSE;
}

#if NDS_IMPORT_BATTLESHIP_FOX_REFLECTOR
static void ndsFighterNaturalReflectorRecordFireball(FTStruct *fox)
{
    GObj *weapon_gobj;
    DObj *fox_root;

    if (fox == NULL)
    {
        return;
    }
    if (fox->special_coll != NULL)
    {
        gNdsFighterReflectorProofSpecialSizeMilli =
            (u32)ndsFloatToMilliSigned(fox->special_coll->size.x);
        gNdsFighterReflectorProofSpecialResist =
            (u32)fox->special_coll->damage_resist;
    }
    weapon_gobj = gGCCommonLinks[nGCCommonLinkIDWeapon];
    while (weapon_gobj != NULL)
    {
        WPStruct *wp = wpGetStruct(weapon_gobj);

        if ((wp != NULL) && (wp->kind == nWPKindFireball))
        {
            gNdsFighterReflectorProofFireballCanReflect =
                (wp->attack_coll.can_reflect != FALSE) ? 1u : 0u;
            gNdsFighterReflectorProofFireballCanAbsorb =
                (wp->attack_coll.can_absorb != FALSE) ? 1u : 0u;
            gNdsFighterReflectorProofFireballCanShield =
                (wp->attack_coll.can_shield != FALSE) ? 1u : 0u;
            gNdsFighterReflectorProofFireballAttackCount =
                (u32)wp->attack_coll.attack_count;
            gNdsFighterReflectorProofFireballDamage =
                (u32)wp->attack_coll.damage;
            gNdsFighterReflectorProofFireballSizeMilli =
                (u32)ndsFloatToMilliSigned(wp->attack_coll.size);
            fox_root = DObjGetStruct(fox->fighter_gobj);
            if ((fox_root != NULL) && (DObjGetStruct(weapon_gobj) != NULL))
            {
                Vec3f *weapon_pos = &DObjGetStruct(weapon_gobj)->translate.vec.f;
                Vec3f *fox_pos = &fox_root->translate.vec.f;

                gNdsFighterReflectorProofFireballDXMilli =
                    ndsFloatToMilliSigned(weapon_pos->x - fox_pos->x);
                gNdsFighterReflectorProofFireballDYMilli =
                    ndsFloatToMilliSigned(weapon_pos->y - fox_pos->y);
            }
        }
        weapon_gobj = weapon_gobj->link_next;
    }
}

static void ndsFighterNaturalReflectorRecord(FTStruct *fp[2])
{
    FTStruct *fox;
    u32 mask;

    if (ndsFighterNaturalReflectorProofEnabled() == FALSE)
    {
        return;
    }
    fox = fp[sNdsNaturalReflectorFoxSlot];
    gNdsFighterReflectorProofFoxSlot = sNdsNaturalReflectorFoxSlot;
    gNdsFighterReflectorProofProjectileSlot =
        sNdsNaturalReflectorProjectileSlot;
    mask = gNdsFighterReflectorProofMask;
    ndsFighterNaturalReflectorRecordFireball(fox);
    if (gNdsFighterReflectorProofDownBPressFrames > 0u)
    {
        mask |= 1u << 0;
    }
    if (fox != NULL)
    {
        if ((fox->status_id == nFTFoxStatusSpecialLwStart) ||
            (fox->status_id == nFTFoxStatusSpecialAirLwStart))
        {
            gNdsFighterReflectorProofStartFrames++;
        }
        if ((fox->status_id == nFTFoxStatusSpecialLwLoop) ||
            (fox->status_id == nFTFoxStatusSpecialAirLwLoop))
        {
            gNdsFighterReflectorProofLoopFrames++;
        }
        if ((fox->status_id == nFTFoxStatusSpecialLwHit) ||
            (fox->status_id == nFTFoxStatusSpecialAirLwHit))
        {
            gNdsFighterReflectorProofHitFrames++;
        }
        if (ndsFighterNaturalProjectileStatusIsSpecialLw(fox) != FALSE)
        {
            mask |= 1u << 1;
        }
        if ((fox->is_reflect != FALSE) &&
            (fox->special_coll != NULL) &&
            (fox->special_coll->kind == nFTSpecialCollKindFoxReflector))
        {
            gNdsFighterReflectorProofIsReflectFrames++;
            mask |= 1u << 2;
        }
        if ((gNdsFighterReflectorProofHitSetCallCount > 0u) &&
            (gNdsFighterReflectorProofReflectLRBeforeHit != 0))
        {
            mask |= 1u << 3;
        }
        if (gNdsFighterReflectorProofHitFrames > 0u)
        {
            mask |= 1u << 4;
        }
        if ((gNdsFighterReflectorProofHitSetCallCount > 0u) &&
            (fox->reflect_lr == 0))
        {
            gNdsFighterReflectorProofReflectLRClearFrames++;
            mask |= 1u << 7;
        }
    }
    if ((gNdsFighterReflectorProofFireballProcCount > 0u) &&
        (gNdsFighterReflectorProofFireballOwnerKind == nFTKindFox))
    {
        mask |= 1u << 5;
    }
    if (((gNdsFighterReflectorProofFireballVelXBefore < 0) &&
         (gNdsFighterReflectorProofFireballVelXAfter > 0)) ||
        ((gNdsFighterReflectorProofFireballVelXBefore > 0) &&
         (gNdsFighterReflectorProofFireballVelXAfter < 0)))
    {
        mask |= 1u << 6;
    }
    gNdsFighterReflectorProofMask = mask;
    if ((mask & 0xffu) == 0xffu)
    {
        gNdsFighterReflectorProofResult =
            NDS_FIGHTER_REFLECTOR_PROOF_PASS;
    }
}
#endif

static void ndsFighterNaturalProjectileRecord(FTStruct *fp[2])
{
#if NDS_IMPORT_BATTLESHIP_WEAPON_MANAGER
    GObj *weapon_gobj;
    FTStruct *actor;
    u32 mask;
    u32 count = 0u;

    if (ndsFighterNaturalProjectileProofEnabled() == FALSE)
    {
        return;
    }
    actor = fp[sNdsNaturalProjectileActorSlot];
    mask = gNdsFighterProjectileProofMask;
    if (actor != NULL)
    {
        gNdsFighterProjectileProofActorSlot =
            sNdsNaturalProjectileActorSlot;
        gNdsFighterProjectileProofActorKind = (u32)actor->fkind;
        mask |= 1u << 0;
        if (ndsFighterNaturalProjectileStatusIsSpecialN(actor) != FALSE)
        {
            gNdsFighterProjectileProofSpecialStatusFrames++;
            gNdsFighterProjectileProofSpecialMotion = (u32)actor->motion_id;
            mask |= 1u << 2;
        }
        if (actor->proc_accessory != NULL)
        {
            gNdsFighterProjectileProofAccessoryFrames++;
        }
        if (actor->motion_vars.flags.flag0 != 0)
        {
            gNdsFighterProjectileProofFlag0Frames++;
        }
    }
    if (gNdsFighterProjectileProofBPressFrames > 0u)
    {
        mask |= 1u << 1;
    }

    weapon_gobj = gGCCommonLinks[nGCCommonLinkIDWeapon];
    while (weapon_gobj != NULL)
    {
        WPStruct *wp = wpGetStruct(weapon_gobj);

        if (wp != NULL)
        {
            count++;
            if ((wp->kind >= 0) && (wp->kind < 32))
            {
                gNdsFighterProjectileProofKindMask |= 1u << wp->kind;
            }
            if ((wp->attack_coll.attack_state >= 0) &&
                (wp->attack_coll.attack_state < 32))
            {
                gNdsFighterProjectileProofAttackStateMask |=
                    1u << wp->attack_coll.attack_state;
            }
            if ((u32)wp->attack_coll.damage >
                gNdsFighterProjectileProofDamageMax)
            {
                gNdsFighterProjectileProofDamageMax =
                    (u32)wp->attack_coll.damage;
            }
            if ((u32)wp->lifetime > gNdsFighterProjectileProofLifetimeMax)
            {
                gNdsFighterProjectileProofLifetimeMax = (u32)wp->lifetime;
            }
            gNdsFighterProjectileProofMapMask |=
                wp->coll_data.mask_prev | wp->coll_data.mask_curr |
                wp->coll_data.mask_stat;
        }
        weapon_gobj = weapon_gobj->link_next;
    }
    if (count > 0u)
    {
        gNdsFighterProjectileProofWeaponFrames++;
        mask |= 1u << 3;
    }
    else if (gNdsFighterProjectileProofHitDestroyCount > 0u)
    {
        mask |= 1u << 3;
    }
    if (count > gNdsFighterProjectileProofWeaponCountMax)
    {
        gNdsFighterProjectileProofWeaponCountMax = count;
    }
    if ((gNdsFighterProjectileProofKindMask &
         (1u << sNdsNaturalProjectileExpectedKind)) != 0u)
    {
        mask |= 1u << 4;
    }
    if ((gNdsFighterProjectileProofAttackStateMask != 0u) &&
        (gNdsFighterProjectileProofDamageMax > 0u))
    {
        mask |= 1u << 5;
    }
    gNdsFighterProjectileProofMask = mask;
    if ((mask & 0x3fu) == 0x3fu)
    {
        gNdsFighterProjectileProofResult =
            NDS_FIGHTER_PROJECTILE_PROOF_PASS;
    }
#if NDS_IMPORT_BATTLESHIP_FOX_REFLECTOR
    ndsFighterNaturalReflectorRecord(fp);
#endif
#else
    (void)fp;
#endif
}

#if NDS_IMPORT_BATTLESHIP_NORMAL_MOVESET
static sb32 ndsFighterNaturalMovesetStatusIsTiltS3(s32 status_id)
{
    return ((status_id >= nFTCommonStatusAttackS3Hi) &&
            (status_id <= nFTCommonStatusAttackS3Lw)) ? TRUE : FALSE;
}

static sb32 ndsFighterNaturalMovesetStatusIsTiltHi3(s32 status_id)
{
    return ((status_id >= nFTCommonStatusAttackHi3F) &&
            (status_id <= nFTCommonStatusAttackHi3B)) ? TRUE : FALSE;
}

static sb32 ndsFighterNaturalMovesetStatusIsTiltLw3(s32 status_id)
{
    return (status_id == nFTCommonStatusAttackLw3) ? TRUE : FALSE;
}

static sb32 ndsFighterNaturalMovesetStatusIsSmash(s32 status_id)
{
    return ((status_id >= nFTCommonStatusAttackS4Hi) &&
            (status_id <= nFTCommonStatusAttackLw4)) ? TRUE : FALSE;
}

static sb32 ndsFighterNaturalMovesetStatusIsAerial(s32 status_id)
{
    return ((status_id >= nFTCommonStatusAttackAirStart) &&
            (status_id <= nFTCommonStatusAttackAirEnd)) ? TRUE : FALSE;
}

static sb32 ndsFighterNaturalMovesetStatusIsLandingAir(s32 status_id)
{
    return (((status_id >= nFTCommonStatusLandingAirStart) &&
             (status_id <= nFTCommonStatusLandingAirEnd)) ||
            (status_id == nFTCommonStatusLandingLight) ||
            (status_id == nFTCommonStatusLandingHeavy)) ? TRUE : FALSE;
}

static sb32 ndsFighterNaturalMovesetStatusIsRecovering(s32 status_id)
{
    return ((status_id == nFTCommonStatusWait) ||
            (status_id == nFTCommonStatusFall) ||
            (status_id == nFTCommonStatusFallAerial) ||
            (status_id == nFTCommonStatusDamageFall) ||
            (status_id == nFTCommonStatusFallSpecial) ||
            (status_id == nFTCommonStatusLandingFallSpecial) ||
            (status_id == nFTCommonStatusDownBounceD) ||
            (status_id == nFTCommonStatusDownBounceU) ||
            (status_id == nFTCommonStatusDownWaitD) ||
            (status_id == nFTCommonStatusDownWaitU) ||
            (status_id == nFTCommonStatusDownStandD) ||
            (status_id == nFTCommonStatusDownStandU) ||
            (status_id == nFTCommonStatusPassiveStandF) ||
            (status_id == nFTCommonStatusPassiveStandB) ||
            (status_id == nFTCommonStatusPassive) ||
            (ndsFighterNaturalCombatStatusIsDamage(status_id) != FALSE) ||
            (ndsFighterNaturalMovesetStatusIsLandingAir(status_id) !=
                FALSE)) ? TRUE : FALSE;
}
#endif

static sb32 ndsFighterNaturalCombatHitboxActive(FTStruct *fp)
{
    u32 i;

    for (i = 0u; i < ARRAY_COUNT(fp->attack_colls); i++)
    {
        if (fp->attack_colls[i].attack_state != nGMAttackStateOff)
        {
            return TRUE;
        }
    }
    return FALSE;
}

static void ndsFighterNaturalCombatRecordAttackColl(FTStruct *fp)
{
    u32 i;

    for (i = 0u; i < ARRAY_COUNT(fp->attack_colls); i++)
    {
        FTAttackColl *attack_coll = &fp->attack_colls[i];

        if (attack_coll->attack_state == nGMAttackStateOff)
        {
            continue;
        }
        gNdsFighterDashRunAttackEventLastPlayer = fp->player;
        gNdsFighterDashRunAttackEventLastStatus = (u32)fp->status_id;
        gNdsFighterDashRunAttackEventLastState =
            (u32)attack_coll->attack_state;
        gNdsFighterDashRunAttackEventLastAttackID = i;
        gNdsFighterDashRunAttackEventLastGroupID = attack_coll->group_id;
        gNdsFighterDashRunAttackEventLastJointID =
            (u32)attack_coll->joint_id;
        gNdsFighterDashRunAttackEventLastDamage = attack_coll->damage;
        gNdsFighterDashRunAttackEventLastSize = (s32)attack_coll->size;
        gNdsFighterDashRunAttackEventLastOffsetX =
            (s32)attack_coll->offset.x;
        gNdsFighterDashRunAttackEventLastOffsetY =
            (s32)attack_coll->offset.y;
        gNdsFighterDashRunAttackEventLastOffsetZ =
            (s32)attack_coll->offset.z;
        gNdsFighterDashRunAttackEventLastAngle = attack_coll->angle;
        gNdsFighterDashRunAttackEventLastKBG =
            attack_coll->knockback_scale;
        gNdsFighterDashRunAttackEventLastKBW =
            attack_coll->knockback_weight;
        gNdsFighterDashRunAttackEventLastBKB =
            attack_coll->knockback_base;
        gNdsFighterDashRunAttackEventLastShield =
            attack_coll->shield_damage;
        gNdsFighterDashRunAttackEventLastFlags =
            (attack_coll->is_hit_air ? 0x1u : 0u) |
            (attack_coll->is_hit_ground ? 0x2u : 0u) |
            (attack_coll->can_rebound ? 0x4u : 0u) |
            (attack_coll->is_scale_pos ? 0x8u : 0u);
        break;
    }
}

static sb32 ndsFighterBattlePlayableStatusIsDead(s32 status_id)
{
    return ((status_id >= nFTCommonStatusDeadDown) &&
            (status_id <= nFTCommonStatusDeadUpFall)) ? TRUE : FALSE;
}

static void ndsFighterBattlePlayableRecordVictim(FTStruct *victim)
{
    u32 mask = 0u;

    if ((ndsFighterBattlePlayableProofEnabled() == FALSE) ||
        (victim == NULL))
    {
        return;
    }

    gNdsFighterBattlePlayableVictimStockFinal = (u32)victim->stock_count;
    gNdsFighterBattlePlayableBattleStockFinal =
        (u32)gSCManagerBattleState->players[victim->player].stock_count;
    gNdsFighterBattlePlayableFallsFinal =
        (u32)gSCManagerBattleState->players[victim->player].falls;
    gNdsFighterBattlePlayableFinalStatus = (u32)victim->status_id;
    gNdsFighterBattlePlayableFinalGA = (u32)victim->ga;
    gNdsFighterBattlePlayableFinalFloor =
        (u32)victim->coll_data.floor_line_id;
    gNdsFighterBattlePlayableFinalIsRebirth = (u32)victim->is_rebirth;
    gNdsFighterBattlePlayableFinalIsGhost = (u32)victim->is_ghost;
    gNdsFighterBattlePlayableFinalCameraMode = (u32)victim->camera_mode;
    gNdsFighterBattlePlayableFinalVelXMilli =
        ndsFloatToMilliSigned(victim->physics.vel_air.x);
    gNdsFighterBattlePlayableFinalVelYMilli =
        ndsFloatToMilliSigned(victim->physics.vel_air.y);
    gNdsFighterBattlePlayableFinalFloorDistMilli =
        ndsFloatToMilliSigned(victim->coll_data.floor_dist);
    if (victim->coll_data.p_translate != NULL)
    {
        gNdsFighterBattlePlayableFinalXMilli =
            ndsFloatToMilliSigned(victim->coll_data.p_translate->x);
        gNdsFighterBattlePlayableFinalYMilli =
            ndsFloatToMilliSigned(victim->coll_data.p_translate->y);
    }

    if ((gNdsFighterBattlePlayableMask & 0x7fu) == 0x7fu)
    {
        if ((gNdsFighterNaturalMotionUnsafeCount == 0u) &&
            (gNdsFighterNaturalCombatStallCount == 0u) &&
            (sNdsNaturalCombatPhase ==
                nNDSNaturalCombatPhaseBattlePlayableDone))
        {
            gNdsFighterBattlePlayableMask |= 1u << 7;
            gNdsFighterBattlePlayableResult =
                NDS_FIGHTER_BATTLE_PLAYABLE_PASS;
        }
        return;
    }

    if ((gSCManagerBattleState->game_rules & SCBATTLE_GAMERULE_STOCK) != 0)
    {
        mask |= 1u << 0;
    }
    if (gNdsFighterBattlePlayableKOStickFrames > 0u)
    {
        mask |= 1u << 1;
    }
    if (ndsFighterBattlePlayableStatusIsDead(victim->status_id) != FALSE)
    {
        gNdsFighterBattlePlayableDeadFrames++;
    }
    if (gNdsFighterBattlePlayableDeadFrames > 0u)
    {
        mask |= 1u << 2;
    }
    if ((sNdsBattlePlayableVictimStockStart >=
            gNdsFighterBattlePlayableVictimStockFinal) &&
        (sNdsBattlePlayableBattleStockStart >=
            gNdsFighterBattlePlayableBattleStockFinal) &&
        (gNdsFighterBattlePlayableFallsFinal >=
            sNdsBattlePlayableFallsStart))
    {
        u32 victim_stock_delta =
            sNdsBattlePlayableVictimStockStart -
            gNdsFighterBattlePlayableVictimStockFinal;
        u32 battle_stock_delta =
            sNdsBattlePlayableBattleStockStart -
            gNdsFighterBattlePlayableBattleStockFinal;
        u32 falls_delta =
            gNdsFighterBattlePlayableFallsFinal -
            sNdsBattlePlayableFallsStart;

        if ((victim_stock_delta > 0u) &&
            (victim_stock_delta == battle_stock_delta) &&
            (victim_stock_delta == falls_delta))
        {
            mask |= 1u << 3;
        }
    }

    if (victim->status_id == nFTCommonStatusRebirthDown)
    {
        gNdsFighterBattlePlayableRebirthDownFrames++;
        sNdsBattlePlayableRebirthSeen = 1u;
    }
    else if (victim->status_id == nFTCommonStatusRebirthStand)
    {
        gNdsFighterBattlePlayableRebirthStandFrames++;
        sNdsBattlePlayableRebirthSeen = 1u;
    }
    else if (victim->status_id == nFTCommonStatusRebirthWait)
    {
        gNdsFighterBattlePlayableRebirthWaitFrames++;
        sNdsBattlePlayableRebirthSeen = 1u;
    }
    if ((gNdsFighterBattlePlayableRebirthDownFrames > 0u) &&
        (gNdsFighterBattlePlayableRebirthStandFrames > 0u) &&
        (gNdsFighterBattlePlayableRebirthWaitFrames > 0u))
    {
        mask |= 1u << 4;
    }
    if ((sNdsBattlePlayableRebirthSeen != 0u) &&
        (victim->status_id == nFTCommonStatusFall))
    {
        gNdsFighterBattlePlayableFallAfterRebirthFrames++;
    }
    if ((gNdsFighterBattlePlayableFallAfterRebirthFrames > 0u) ||
        (gNdsFighterBattlePlayableWaitAfterRebirthFrames > 0u))
    {
        mask |= 1u << 5;
    }
    if ((gNdsFighterBattlePlayableRebirthWaitFrames > 0u) &&
        (victim->status_id == nFTCommonStatusWait))
    {
        gNdsFighterBattlePlayableWaitAfterRebirthFrames++;
    }
    if (gNdsFighterBattlePlayableWaitAfterRebirthFrames >=
        NDS_FIGHTER_BATTLE_PLAYABLE_WAIT_AFTER_REBIRTH_REQUIRED)
    {
        mask |= 1u << 6;
    }
    if ((gNdsFighterNaturalMotionUnsafeCount == 0u) &&
        (gNdsFighterNaturalCombatStallCount == 0u) &&
        (sNdsNaturalCombatPhase ==
            nNDSNaturalCombatPhaseBattlePlayableDone))
    {
        mask |= 1u << 7;
    }

    gNdsFighterBattlePlayableMask = mask;
    if ((mask & NDS_FIGHTER_BATTLE_PLAYABLE_MASK_ALL) ==
        NDS_FIGHTER_BATTLE_PLAYABLE_MASK_ALL)
    {
        gNdsFighterBattlePlayableResult =
            NDS_FIGHTER_BATTLE_PLAYABLE_PASS;
    }
}

/* Observe the attacker/victim pair while the combat phases run. All state
 * transitions below happen inside the original imported runtime; this only
 * counts what it sees. */
static void ndsFighterNaturalCombatRecordPair(FTStruct *attacker,
                                              FTStruct *victim)
{
    if ((attacker == NULL) || (victim == NULL))
    {
        gNdsFighterNaturalMotionUnsafeCount++;
        return;
    }

    ndsRelocUpdateMemoryLedger();
    ndsFighterBattlePlayableRecordVictim(victim);
    ndsIFCommonRecordHUDState();

    if (attacker->status_id == nFTCommonStatusAttack11)
    {
        gNdsFighterNaturalCombatAttackStatusFrames++;
        gNdsFighterNaturalCombatAttackMotionFinal =
            (u32)attacker->motion_id;
        if (ndsFighterNaturalCombatHitboxActive(attacker) != FALSE)
        {
            gNdsFighterNaturalCombatHitboxActiveFrames++;
            ndsFighterNaturalCombatRecordAttackColl(attacker);
        }
    }
#if NDS_IMPORT_BATTLESHIP_NORMAL_MOVESET
    if (ndsFighterBattlePlayableProofEnabled() != FALSE)
    {
        sb32 is_hitbox_active =
            ndsFighterNaturalCombatHitboxActive(attacker);
        DObj *attacker_root = attacker->joints[nFTPartsJointTopN];
        DObj *victim_root = victim->joints[nFTPartsJointTopN];

        gNdsFighterNaturalMovesetAttackerStatus = (u32)attacker->status_id;
        gNdsFighterNaturalMovesetAttackerMotion = (u32)attacker->motion_id;
        gNdsFighterNaturalMovesetAttackerGA = (u32)attacker->ga;
        gNdsFighterNaturalMovesetAttackerRootYMilli =
            (attacker_root != NULL) ?
                ndsFloatToMilliSigned(attacker_root->translate.vec.f.y) : 0;
        gNdsFighterNaturalMovesetVictimStatus = (u32)victim->status_id;
        gNdsFighterNaturalMovesetVictimMotion = (u32)victim->motion_id;
        gNdsFighterNaturalMovesetVictimGA = (u32)victim->ga;
        gNdsFighterNaturalMovesetVictimRootYMilli =
            (victim_root != NULL) ?
                ndsFloatToMilliSigned(victim_root->translate.vec.f.y) : 0;

        if (ndsFighterNaturalMovesetStatusIsTiltS3(
                attacker->status_id) != FALSE)
        {
            gNdsFighterNaturalMovesetTiltS3Frames++;
            if (is_hitbox_active != FALSE)
            {
                gNdsFighterNaturalMovesetTiltHitboxFrames++;
                ndsFighterNaturalCombatRecordAttackColl(attacker);
            }
        }
        else if (ndsFighterNaturalMovesetStatusIsTiltHi3(
                     attacker->status_id) != FALSE)
        {
            gNdsFighterNaturalMovesetTiltHi3Frames++;
            if (is_hitbox_active != FALSE)
            {
                gNdsFighterNaturalMovesetTiltHitboxFrames++;
                ndsFighterNaturalCombatRecordAttackColl(attacker);
            }
        }
        else if (ndsFighterNaturalMovesetStatusIsTiltLw3(
                     attacker->status_id) != FALSE)
        {
            gNdsFighterNaturalMovesetTiltLw3Frames++;
            if (is_hitbox_active != FALSE)
            {
                gNdsFighterNaturalMovesetTiltHitboxFrames++;
                ndsFighterNaturalCombatRecordAttackColl(attacker);
            }
        }
        else if (ndsFighterNaturalMovesetStatusIsSmash(
                     attacker->status_id) != FALSE)
        {
            gNdsFighterNaturalMovesetSmashFrames++;
            if (is_hitbox_active != FALSE)
            {
                gNdsFighterNaturalMovesetSmashHitboxFrames++;
                ndsFighterNaturalCombatRecordAttackColl(attacker);
            }
        }
        else if (ndsFighterNaturalMovesetStatusIsAerial(
                     attacker->status_id) != FALSE)
        {
            gNdsFighterNaturalMovesetAerialFrames++;
            if (is_hitbox_active != FALSE)
            {
                gNdsFighterNaturalMovesetAerialHitboxFrames++;
                ndsFighterNaturalCombatRecordAttackColl(attacker);
            }
        }
        else if (ndsFighterNaturalMovesetStatusIsLandingAir(
                     attacker->status_id) != FALSE)
        {
            gNdsFighterNaturalMovesetLandingFrames++;
        }
        else if (attacker->status_id == nFTCommonStatusCatch)
        {
            gNdsFighterNaturalMovesetCatchFrames++;
        }
        else if (attacker->status_id == nFTCommonStatusCatchWait)
        {
            gNdsFighterNaturalMovesetCatchWaitFrames++;
        }
        else if ((attacker->status_id == nFTCommonStatusThrowF) ||
                 (attacker->status_id == nFTCommonStatusThrowB))
        {
            gNdsFighterNaturalMovesetThrowFrames++;
        }
        if ((victim->status_id >= nFTCommonStatusThrownStart) &&
            (victim->status_id <= nFTCommonStatusThrownEnd))
        {
            gNdsFighterNaturalMovesetThrownFrames++;
        }
        if ((gNdsFighterNaturalMovesetThrownFrames > 0u) &&
            (victim->status_id == nFTCommonStatusWait))
        {
            gNdsFighterNaturalMovesetThrowRecoverFrames++;
        }
    }
#endif

    if (ndsFighterNaturalCombatStatusIsDamage(victim->status_id) != FALSE)
    {
        if (sNdsNaturalCombatVictimHitSeen == 0u)
        {
            sNdsNaturalCombatVictimHitSeen = 1u;
            sNdsNaturalCombatVictimHitPosX =
                ndsFighterNaturalCombatPosX(victim);
            gNdsFighterNaturalCombatVictimDamageStatus =
                (u32)victim->status_id;
        }
        gNdsFighterNaturalCombatVictimDamageFrames++;
    }
    if (sNdsNaturalCombatVictimHitSeen != 0u)
    {
        f32 delta = ndsFighterNaturalCombatPosX(victim) -
            sNdsNaturalCombatVictimHitPosX;

        if (delta < 0.0F)
        {
            delta = -delta;
        }
        if (ndsFloatToMilliSigned(delta) >
            (s32)gNdsFighterNaturalCombatVictimKnockbackMilli)
        {
            gNdsFighterNaturalCombatVictimKnockbackMilli =
                (u32)ndsFloatToMilliSigned(delta);
        }
        if ((victim->status_id == nFTCommonStatusWait) &&
            (ndsFighterNaturalCombatStatusIsGuard(victim->status_id) ==
                FALSE))
        {
            gNdsFighterNaturalCombatVictimRecoverWaitFrames++;
        }
    }
    gNdsFighterNaturalCombatVictimFinalPercent =
        (u32)victim->percent_damage;

    if (victim->status_id == nFTCommonStatusGuardOn)
    {
        gNdsFighterNaturalCombatGuardOnFrames++;
    }
    else if (victim->status_id == nFTCommonStatusGuard)
    {
        gNdsFighterNaturalCombatGuardFrames++;
    }
    else if ((victim->status_id == nFTCommonStatusGuardOff) ||
             (victim->status_id == nFTCommonStatusGuardSetOff))
    {
        gNdsFighterNaturalCombatGuardOffFrames++;
    }
}

void ndsFighterMarioFoxNaturalMotionPrepare(void)
{
    FTStruct *p0 = ndsFighterManagerLiveStruct(0u);
    FTStruct *p1 = ndsFighterManagerLiveStruct(1u);
    FTStruct *fp[2];

    if ((ndsFighterMarioFoxNaturalMotionProofEnabled() == FALSE) ||
        (gNdsFighterNaturalMotionPrepared != 0u))
    {
        return;
    }
    gNdsFighterNaturalMotionManagerMask = ndsFighterManagerLiveMask();
    if ((gNdsFighterNaturalMotionManagerMask & 0x3u) != 0x3u)
    {
        return;
    }
    fp[0] = p0;
    fp[1] = p1;

    bzero(sNdsFighterNaturalMotionStates,
          sizeof(sNdsFighterNaturalMotionStates));
    sNdsFighterNaturalMotionWalkInputActive = 0u;
    sNdsNaturalCombatPhase = nNDSNaturalCombatPhaseWait;
    sNdsNaturalCombatPhaseFrames = 0u;
    sNdsNaturalCombatSettleFrames = 0u;
    sNdsNaturalCombatAttackFrames = 0u;
    sNdsNaturalCombatAttackPressed = 0u;
    sNdsNaturalCombatPassPressed = 0u;
    sNdsNaturalCombatApproachStopRange =
        NDS_FIGHTER_NATURAL_COMBAT_APPROACH_STOP_RANGE;
    sNdsNaturalCombatVictimHitSeen = 0u;
    sNdsNaturalCombatVictimHitPosX = 0.0F;
#if NDS_IMPORT_BATTLESHIP_NORMAL_MOVESET
    sNdsNaturalMovesetPhase = nNDSNaturalMovesetPhaseIdle;
    sNdsNaturalMovesetPhaseFrames = 0u;
    sNdsNaturalMovesetSettleFrames = 0u;
    sNdsNaturalMovesetDone = 0u;
    sNdsNaturalMovesetKORecoveryActive = 0u;
    sNdsNaturalMovesetKORecoveryPhase = nNDSNaturalMovesetPhaseIdle;
#endif
#if NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_HI || \
    NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_LW || \
    NDS_IMPORT_BATTLESHIP_FOX_SPECIAL_HI
    sNdsNaturalSpecialsPhase = nNDSNaturalSpecialsPhaseIdle;
    sNdsNaturalSpecialsPhaseFrames = 0u;
    sNdsNaturalSpecialsDone = 0u;
    sNdsNaturalSpecialsButtonPressed = 0u;
    gNdsFighterSpecialsMarioSlot = 0u;
    gNdsFighterSpecialsFoxSlot = 1u;
    if (p0->fkind == nFTKindFox)
    {
        gNdsFighterSpecialsFoxSlot = 0u;
    }
    if (p1->fkind == nFTKindMario)
    {
        gNdsFighterSpecialsMarioSlot = 1u;
    }
    if (p1->fkind == nFTKindFox)
    {
        gNdsFighterSpecialsFoxSlot = 1u;
    }
#endif
    sNdsNaturalCombatAttackerSlot = (p1->fkind == nFTKindFox) ? 1u : 0u;
    sNdsNaturalCombatVictimSlot = 1u - sNdsNaturalCombatAttackerSlot;
    sNdsNaturalCombatVictimStartPercent =
        (u32)((sNdsNaturalCombatVictimSlot == 0u) ? p0 : p1)->percent_damage;
    sNdsBattlePlayableRebirthSeen = 0u;
    sNdsBattlePlayableKOStickX = 80;
    sNdsNaturalProjectileButtonPressed = 0u;
    sNdsNaturalProjectileKORecoveryActive = 0u;
    sNdsNaturalProjectileActorSlot =
        ndsFighterNaturalProjectileSelectSlot(fp);
    sNdsNaturalProjectileExpectedKind =
        ndsFighterNaturalProjectileExpectedKind(fp[sNdsNaturalProjectileActorSlot]);
#if NDS_IMPORT_BATTLESHIP_FOX_REFLECTOR
    sNdsNaturalReflectorProjectileSlot = sNdsNaturalProjectileActorSlot;
    sNdsNaturalReflectorFoxSlot =
        ndsFighterNaturalReflectorSelectFoxSlot(fp);
    sNdsNaturalReflectorButtonPressed = 0u;
#endif
    if (ndsFighterBattlePlayableProofEnabled() != FALSE)
    {
        FTStruct *victim =
            (sNdsNaturalCombatVictimSlot == 0u) ? p0 : p1;

        sNdsBattlePlayableVictimStockStart = (u32)victim->stock_count;
        sNdsBattlePlayableBattleStockStart =
            (u32)gSCManagerBattleState->players[victim->player].stock_count;
        sNdsBattlePlayableFallsStart =
            (u32)gSCManagerBattleState->players[victim->player].falls;
        sNdsBattlePlayableKOStickX =
            (ndsFighterNaturalCombatPosX(victim) < 0.0F) ? -80 : 80;
        gNdsFighterBattlePlayableVictimSlot = sNdsNaturalCombatVictimSlot;
        gNdsFighterBattlePlayableVictimStockStart =
            sNdsBattlePlayableVictimStockStart;
        gNdsFighterBattlePlayableBattleStockStart =
            sNdsBattlePlayableBattleStockStart;
        gNdsFighterBattlePlayableFallsStart =
            sNdsBattlePlayableFallsStart;
    }
    gNdsFighterNaturalCombatAttackerSlot = sNdsNaturalCombatAttackerSlot;
    gNdsFighterNaturalCombatVictimSlot = sNdsNaturalCombatVictimSlot;
    gNdsFighterNaturalCombatVictimStartPercent =
        sNdsNaturalCombatVictimStartPercent;
    ndsControllerPlaybackReset();
    ndsControllerPlaybackSetConnectedMask(0x3u);
#if NDS_DEV_LIVE_INPUT_PREVIEW
    if (ndsFighterBattlePlayableProofEnabled() != FALSE)
    {
        ndsControllerPlaybackSetEnabled(FALSE);
    }
    else
#endif
    {
        ndsControllerPlaybackSetEnabled(TRUE);
    }
    gNdsFighterNaturalMotionGObjCountBefore = (u32)gcGetGObjsActiveNum();

    if (ndsFighterBattlePlayableProofEnabled() == FALSE)
    {
        gcFuncGObjAll(ndsFighterNaturalMotionPauseNonTargetVisitor, 0u);
    }

    gNdsFighterNaturalMotionP0StatusStart = (u32)p0->status_id;
    gNdsFighterNaturalMotionP1StatusStart = (u32)p1->status_id;
    gNdsFighterNaturalMotionPrepared = 1u;
}

static sb32 ndsFighterNaturalCombatBothWait(FTStruct *fp[2])
{
    return ((fp[0]->status_id == nFTCommonStatusWait) &&
            (fp[1]->status_id == nFTCommonStatusWait)) ? TRUE : FALSE;
}

static sb32 ndsFighterNaturalCombatBothGroundWait(FTStruct *fp[2])
{
    return ((ndsFighterNaturalCombatBothWait(fp) != FALSE) &&
            (fp[0]->ga == nMPKineticsGround) &&
            (fp[1]->ga == nMPKineticsGround)) ? TRUE : FALSE;
}

static void ndsFighterNaturalCombatSetPhase(u32 phase)
{
    sNdsNaturalCombatPhase = phase;
    sNdsNaturalCombatPhaseFrames = 0u;
    sNdsNaturalCombatSettleFrames = 0u;
    gNdsFighterNaturalCombatPhase = phase;
}

static sb32 ndsFighterBattlePlayableHasRecoveredKO(void)
{
    return ((gNdsFighterBattlePlayableDeadFrames > 0u) &&
            (gNdsFighterBattlePlayableRebirthDownFrames > 0u) &&
            (gNdsFighterBattlePlayableRebirthStandFrames > 0u) &&
            (gNdsFighterBattlePlayableRebirthWaitFrames > 0u) &&
            (gNdsFighterBattlePlayableWaitAfterRebirthFrames >=
                NDS_FIGHTER_BATTLE_PLAYABLE_WAIT_AFTER_REBIRTH_REQUIRED)) ?
        TRUE : FALSE;
}

static void ndsFighterNaturalCombatStartKOExit(FTStruct *victim)
{
    if (ndsFighterBattlePlayableHasRecoveredKO() != FALSE)
    {
        ndsFighterNaturalCombatSetPhase(
            nNDSNaturalCombatPhaseBattlePlayableRecover);
        return;
    }
    sNdsBattlePlayableKOStickX =
        (ndsFighterNaturalCombatPosX(victim) < 0.0F) ? -80 : 80;
    ndsFighterNaturalCombatSetPhase(
        nNDSNaturalCombatPhaseBattlePlayableKOExit);
}

static sb32 ndsFighterNaturalProjectileHandleKORecovery(FTStruct *fp[2])
{
    u32 i;

    if ((sNdsNaturalCombatPhase != nNDSNaturalCombatPhaseProjectileSettle) &&
        (sNdsNaturalCombatPhase != nNDSNaturalCombatPhaseProjectileFire) &&
        (sNdsNaturalCombatPhase != nNDSNaturalCombatPhaseProjectileObserve))
    {
        return FALSE;
    }
    if (sNdsNaturalProjectileKORecoveryActive == 0u)
    {
        for (i = 0u; i < 2u; i++)
        {
            if (ndsFighterBattlePlayableStatusIsDead(fp[i]->status_id) !=
                FALSE)
            {
                sNdsNaturalProjectileKORecoveryActive = 1u;
                break;
            }
        }
    }
    if (sNdsNaturalProjectileKORecoveryActive == 0u)
    {
        return FALSE;
    }
    if (ndsFighterNaturalCombatBothGroundWait(fp) != FALSE)
    {
        sNdsNaturalProjectileKORecoveryActive = 0u;
        sNdsNaturalProjectileButtonPressed = 0u;
        sNdsNaturalCombatPassPressed = 0u;
#if NDS_IMPORT_BATTLESHIP_FOX_REFLECTOR
        sNdsNaturalReflectorButtonPressed = 0u;
#endif
        ndsFighterNaturalCombatSetPhase(
            nNDSNaturalCombatPhaseProjectileSettle);
    }
    return TRUE;
}

#if NDS_IMPORT_BATTLESHIP_NORMAL_MOVESET
static void ndsFighterNaturalMovesetSetPhase(u32 phase)
{
    sNdsNaturalMovesetPhase = phase;
    sNdsNaturalMovesetPhaseFrames = 0u;
    sNdsNaturalMovesetSettleFrames = 0u;
    gNdsFighterNaturalMovesetPhase = phase;
}

static u32 ndsFighterNaturalMovesetRetryPhase(u32 phase)
{
    switch (phase)
    {
    case nNDSNaturalMovesetPhaseSettleTiltS3:
        return nNDSNaturalMovesetPhaseTiltS3;
    case nNDSNaturalMovesetPhaseSettleTiltHi3:
        return nNDSNaturalMovesetPhaseTiltHi3;
    case nNDSNaturalMovesetPhaseSettleTiltLw3:
        return nNDSNaturalMovesetPhaseTiltLw3;
    case nNDSNaturalMovesetPhaseSettleSmashS4:
        return nNDSNaturalMovesetPhaseSmashS4;
    case nNDSNaturalMovesetPhaseAerialAttack:
    case nNDSNaturalMovesetPhaseSettleAerial:
        return nNDSNaturalMovesetPhaseAerialJump;
    case nNDSNaturalMovesetPhaseGrabThrow:
    case nNDSNaturalMovesetPhaseSettleThrow:
        return nNDSNaturalMovesetPhaseGrabCatch;
    default:
        return phase;
    }
}

static u32 ndsFighterNaturalMovesetRecoveryPhase(u32 phase)
{
    switch (phase)
    {
    case nNDSNaturalMovesetPhaseTiltS3:
        if ((gNdsFighterNaturalMovesetTiltS3Frames > 0u) &&
            (gNdsFighterNaturalMovesetTiltHitboxFrames > 0u))
        {
            return nNDSNaturalMovesetPhaseSettleTiltS3;
        }
        break;
    case nNDSNaturalMovesetPhaseSettleTiltS3:
        return nNDSNaturalMovesetPhaseTiltHi3;
    case nNDSNaturalMovesetPhaseTiltHi3:
        if ((gNdsFighterNaturalMovesetTiltHi3Frames > 0u) &&
            (gNdsFighterNaturalMovesetTiltHitboxFrames > 0u))
        {
            return nNDSNaturalMovesetPhaseSettleTiltHi3;
        }
        break;
    case nNDSNaturalMovesetPhaseSettleTiltHi3:
        return nNDSNaturalMovesetPhaseTiltLw3;
    case nNDSNaturalMovesetPhaseTiltLw3:
        if ((gNdsFighterNaturalMovesetTiltLw3Frames > 0u) &&
            (gNdsFighterNaturalMovesetTiltHitboxFrames > 0u))
        {
            return nNDSNaturalMovesetPhaseSettleTiltLw3;
        }
        break;
    case nNDSNaturalMovesetPhaseSettleTiltLw3:
        return nNDSNaturalMovesetPhaseSmashS4;
    case nNDSNaturalMovesetPhaseSmashS4:
        if ((gNdsFighterNaturalMovesetSmashFrames > 0u) &&
            (gNdsFighterNaturalMovesetSmashHitboxFrames > 0u))
        {
            return nNDSNaturalMovesetPhaseSettleSmashS4;
        }
        break;
    case nNDSNaturalMovesetPhaseSettleSmashS4:
        return nNDSNaturalMovesetPhaseAerialJump;
    case nNDSNaturalMovesetPhaseAerialJump:
        if (gNdsFighterNaturalMovesetAerialFrames > 0u)
        {
            return nNDSNaturalMovesetPhaseSettleAerial;
        }
        break;
    case nNDSNaturalMovesetPhaseAerialAttack:
        if (gNdsFighterNaturalMovesetAerialFrames > 0u)
        {
            return nNDSNaturalMovesetPhaseSettleAerial;
        }
        break;
    case nNDSNaturalMovesetPhaseSettleAerial:
        if (gNdsFighterNaturalMovesetLandingFrames > 0u)
        {
            return nNDSNaturalMovesetPhaseDone;
        }
        break;
    case nNDSNaturalMovesetPhaseGrabCatch:
        if (gNdsFighterNaturalMovesetCatchWaitFrames > 0u)
        {
            return nNDSNaturalMovesetPhaseGrabThrow;
        }
        break;
    case nNDSNaturalMovesetPhaseGrabThrow:
        if (gNdsFighterNaturalMovesetThrowFrames > 0u)
        {
            return nNDSNaturalMovesetPhaseSettleThrow;
        }
        break;
    default:
        break;
    }
    return ndsFighterNaturalMovesetRetryPhase(phase);
}

static sb32 ndsFighterNaturalMovesetBothGroundWait(FTStruct *fp[2])
{
    return ((ndsFighterNaturalCombatBothWait(fp) != FALSE) &&
            (fp[0]->ga == nMPKineticsGround) &&
            (fp[1]->ga == nMPKineticsGround) &&
            (ABS(fp[0]->input.pl.stick_range.x) < 20) &&
            (ABS(fp[1]->input.pl.stick_range.x) < 20) &&
            (ABS(fp[0]->input.pl.stick_range.y) < 20) &&
            (ABS(fp[1]->input.pl.stick_range.y) < 20) &&
            (fp[0]->tap_stick_x == FTINPUT_STICKBUFFER_TICS_MAX) &&
            (fp[1]->tap_stick_x == FTINPUT_STICKBUFFER_TICS_MAX) &&
            (fp[0]->tap_stick_y == FTINPUT_STICKBUFFER_TICS_MAX) &&
            (fp[1]->tap_stick_y == FTINPUT_STICKBUFFER_TICS_MAX)) ?
        TRUE : FALSE;
}

static sb32 ndsFighterNaturalMovesetPhaseReadyToStart(FTStruct *fp[2])
{
    FTStruct *attacker = fp[sNdsNaturalCombatAttackerSlot];

    if (attacker == NULL)
    {
        return FALSE;
    }
    switch (sNdsNaturalMovesetPhase)
    {
    case nNDSNaturalMovesetPhaseTiltS3:
        if ((gNdsFighterNaturalMovesetTiltS3Frames > 0u) ||
            (ndsFighterNaturalMovesetStatusIsTiltS3(attacker->status_id) !=
                FALSE))
        {
            return TRUE;
        }
        return ndsFighterNaturalMovesetBothGroundWait(fp);
    case nNDSNaturalMovesetPhaseTiltHi3:
        if ((gNdsFighterNaturalMovesetTiltHi3Frames > 0u) ||
            (ndsFighterNaturalMovesetStatusIsTiltHi3(attacker->status_id) !=
                FALSE))
        {
            return TRUE;
        }
        return ndsFighterNaturalMovesetBothGroundWait(fp);
    case nNDSNaturalMovesetPhaseTiltLw3:
        if ((gNdsFighterNaturalMovesetTiltLw3Frames > 0u) ||
            (ndsFighterNaturalMovesetStatusIsTiltLw3(attacker->status_id) !=
                FALSE))
        {
            return TRUE;
        }
        return ndsFighterNaturalMovesetBothGroundWait(fp);
    case nNDSNaturalMovesetPhaseSmashS4:
        if ((gNdsFighterNaturalMovesetSmashFrames > 0u) ||
            (ndsFighterNaturalMovesetStatusIsSmash(attacker->status_id) !=
                FALSE))
        {
            return TRUE;
        }
        return ndsFighterNaturalMovesetBothGroundWait(fp);
    case nNDSNaturalMovesetPhaseAerialJump:
    case nNDSNaturalMovesetPhaseAerialAttack:
        if ((gNdsFighterNaturalMovesetAerialFrames > 0u) ||
            (attacker->status_id == nFTCommonStatusKneeBend) ||
            (attacker->status_id == nFTCommonStatusJumpF) ||
            (attacker->status_id == nFTCommonStatusJumpB) ||
            (attacker->status_id == nFTCommonStatusJumpAerialF) ||
            (attacker->status_id == nFTCommonStatusJumpAerialB) ||
            (attacker->status_id == nFTCommonStatusFall) ||
            (attacker->status_id == nFTCommonStatusFallAerial) ||
            (ndsFighterNaturalMovesetStatusIsAerial(attacker->status_id) !=
                FALSE))
        {
            return TRUE;
        }
        return ndsFighterNaturalMovesetBothGroundWait(fp);
    case nNDSNaturalMovesetPhaseGrabCatch:
        if ((gNdsFighterNaturalMovesetCatchFrames > 0u) ||
            (gNdsFighterNaturalMovesetCatchWaitFrames > 0u))
        {
            return TRUE;
        }
        return ndsFighterNaturalMovesetBothGroundWait(fp);
    default:
        return TRUE;
    }
}

static sb32 ndsFighterNaturalMovesetHandleKORecovery(FTStruct *fp[2])
{
    FTStruct *victim = fp[sNdsNaturalCombatVictimSlot];

    if ((victim == NULL) ||
        (sNdsNaturalMovesetPhase == nNDSNaturalMovesetPhaseIdle) ||
        (sNdsNaturalMovesetPhase == nNDSNaturalMovesetPhaseDone))
    {
        return FALSE;
    }
    if ((sNdsNaturalMovesetKORecoveryActive == 0u) &&
        (ndsFighterBattlePlayableStatusIsDead(victim->status_id) != FALSE))
    {
        sNdsNaturalMovesetKORecoveryActive = 1u;
        sNdsNaturalMovesetKORecoveryPhase =
            ndsFighterNaturalMovesetRecoveryPhase(sNdsNaturalMovesetPhase);
    }
    if (sNdsNaturalMovesetKORecoveryActive == 0u)
    {
        return FALSE;
    }
    if (ndsFighterNaturalMovesetBothGroundWait(fp) != FALSE)
    {
        u32 phase = sNdsNaturalMovesetKORecoveryPhase;

        sNdsNaturalMovesetKORecoveryActive = 0u;
        sNdsNaturalMovesetKORecoveryPhase = nNDSNaturalMovesetPhaseIdle;
        ndsFighterNaturalMovesetSetPhase(phase);
    }
    return TRUE;
}
#endif

static sb32 ndsFighterNaturalCombatSettled(FTStruct *fp[2])
{
    if ((ndsFighterNaturalCombatBothWait(fp) != FALSE) &&
        (ABS(fp[0]->input.pl.stick_range.x) < 20) &&
        (ABS(fp[1]->input.pl.stick_range.x) < 20) &&
        (fp[0]->tap_stick_x == FTINPUT_STICKBUFFER_TICS_MAX) &&
        (fp[1]->tap_stick_x == FTINPUT_STICKBUFFER_TICS_MAX))
    {
        sNdsNaturalCombatSettleFrames++;
    }
    else
    {
        sNdsNaturalCombatSettleFrames = 0u;
    }
    return (sNdsNaturalCombatSettleFrames >=
            NDS_FIGHTER_NATURAL_COMBAT_SETTLE_FRAMES_REQUIRED) ?
        TRUE : FALSE;
}

#if NDS_IMPORT_BATTLESHIP_NORMAL_MOVESET
static sb32 ndsFighterNaturalMovesetSettled(FTStruct *fp[2])
{
    FTStruct *victim = fp[sNdsNaturalCombatVictimSlot];

    if (ndsFighterNaturalMovesetBothGroundWait(fp) != FALSE)
    {
        sNdsNaturalMovesetSettleFrames++;
    }
    else if ((victim != NULL) &&
             (ndsFighterNaturalMovesetStatusIsRecovering(
                 victim->status_id) != FALSE))
    {
        sNdsNaturalMovesetSettleFrames = 0u;
    }
    else
    {
        sNdsNaturalMovesetSettleFrames = 0u;
    }
    return (sNdsNaturalMovesetSettleFrames >=
            NDS_FIGHTER_NATURAL_COMBAT_SETTLE_FRAMES_REQUIRED) ?
        TRUE : FALSE;
}

static void ndsFighterNaturalMovesetUpdateMask(void)
{
    u32 mask = 0u;

    if (gNdsFighterNaturalMovesetTiltS3Frames > 0u)
    {
        mask |= 1u << 0;
    }
    if (gNdsFighterNaturalMovesetTiltHi3Frames > 0u)
    {
        mask |= 1u << 1;
    }
    if (gNdsFighterNaturalMovesetTiltLw3Frames > 0u)
    {
        mask |= 1u << 2;
    }
    if (gNdsFighterNaturalMovesetTiltHitboxFrames > 0u)
    {
        mask |= 1u << 3;
    }
    if ((gNdsFighterNaturalMovesetSmashFrames > 0u) &&
        (gNdsFighterNaturalMovesetSmashHitboxFrames > 0u))
    {
        mask |= 1u << 4;
    }
    if (gNdsFighterNaturalMovesetAerialFrames > 0u)
    {
        mask |= 1u << 5;
    }
    if (gNdsFighterNaturalMovesetLandingFrames > 0u)
    {
        mask |= 1u << 6;
    }
    if ((gNdsFighterNaturalMovesetCatchFrames > 0u) &&
        (gNdsFighterNaturalMovesetCatchWaitFrames > 0u))
    {
        mask |= 1u << 7;
    }
    if (gNdsFighterNaturalMovesetThrowFrames > 0u)
    {
        mask |= 1u << 8;
    }
    if (gNdsFighterNaturalMovesetThrownFrames > 0u)
    {
        mask |= 1u << 9;
    }
    if (gNdsFighterNaturalMovesetThrowRecoverFrames >=
        NDS_FIGHTER_NATURAL_COMBAT_SETTLE_FRAMES_REQUIRED)
    {
        mask |= 1u << 10;
    }
    gNdsFighterNaturalMovesetMask = mask;
}

static sb32 ndsFighterNaturalMovesetAdvance(FTStruct *fp[2])
{
    FTStruct *attacker = fp[sNdsNaturalCombatAttackerSlot];

    if (sNdsNaturalMovesetDone != 0u)
    {
        return TRUE;
    }
    if (sNdsNaturalMovesetPhase == nNDSNaturalMovesetPhaseIdle)
    {
        ndsFighterNaturalMovesetSetPhase(nNDSNaturalMovesetPhaseTiltS3);
    }
    if (ndsFighterNaturalMovesetHandleKORecovery(fp) != FALSE)
    {
        return FALSE;
    }
    if (ndsFighterNaturalMovesetPhaseReadyToStart(fp) == FALSE)
    {
        ndsFighterNaturalMovesetUpdateMask();
        return FALSE;
    }

    sNdsNaturalMovesetPhaseFrames++;
    gNdsFighterNaturalMovesetPhaseFrames = sNdsNaturalMovesetPhaseFrames;
    ndsFighterNaturalMovesetUpdateMask();
    if (sNdsNaturalMovesetPhaseFrames >
        NDS_FIGHTER_NATURAL_MOVESET_PHASE_TIMEOUT)
    {
        u32 recovery_phase =
            ndsFighterNaturalMovesetRecoveryPhase(sNdsNaturalMovesetPhase);

        if (recovery_phase != sNdsNaturalMovesetPhase)
        {
            ndsFighterNaturalMovesetSetPhase(recovery_phase);
            return FALSE;
        }
        gNdsFighterNaturalCombatStallCount++;
        ndsFighterNaturalMovesetSetPhase(
            ndsFighterNaturalMovesetRetryPhase(sNdsNaturalMovesetPhase));
        return FALSE;
    }
    if (ndsFighterNaturalMovesetPhaseReadyToStart(fp) == FALSE)
    {
        return TRUE;
    }

    switch (sNdsNaturalMovesetPhase)
    {
    case nNDSNaturalMovesetPhaseTiltS3:
        if ((gNdsFighterNaturalMovesetTiltS3Frames > 0u) &&
            (ndsFighterNaturalMovesetStatusIsTiltS3(attacker->status_id) ==
                FALSE))
        {
            ndsFighterNaturalMovesetSetPhase(
                nNDSNaturalMovesetPhaseSettleTiltS3);
        }
        break;
    case nNDSNaturalMovesetPhaseSettleTiltS3:
        if (ndsFighterNaturalMovesetSettled(fp) != FALSE)
        {
            ndsFighterNaturalMovesetSetPhase(
                nNDSNaturalMovesetPhaseTiltHi3);
        }
        break;
    case nNDSNaturalMovesetPhaseTiltHi3:
        if ((gNdsFighterNaturalMovesetTiltHi3Frames > 0u) &&
            (ndsFighterNaturalMovesetStatusIsTiltHi3(attacker->status_id) ==
                FALSE))
        {
            ndsFighterNaturalMovesetSetPhase(
                nNDSNaturalMovesetPhaseSettleTiltHi3);
        }
        break;
    case nNDSNaturalMovesetPhaseSettleTiltHi3:
        if (ndsFighterNaturalMovesetSettled(fp) != FALSE)
        {
            ndsFighterNaturalMovesetSetPhase(
                nNDSNaturalMovesetPhaseTiltLw3);
        }
        break;
    case nNDSNaturalMovesetPhaseTiltLw3:
        if ((gNdsFighterNaturalMovesetTiltLw3Frames > 0u) &&
            (ndsFighterNaturalMovesetStatusIsTiltLw3(attacker->status_id) ==
                FALSE))
        {
            ndsFighterNaturalMovesetSetPhase(
                nNDSNaturalMovesetPhaseSettleTiltLw3);
        }
        break;
    case nNDSNaturalMovesetPhaseSettleTiltLw3:
        if (ndsFighterNaturalMovesetSettled(fp) != FALSE)
        {
            ndsFighterNaturalMovesetSetPhase(
                nNDSNaturalMovesetPhaseSmashS4);
        }
        break;
    case nNDSNaturalMovesetPhaseSmashS4:
        if ((gNdsFighterNaturalMovesetSmashFrames > 0u) &&
            (ndsFighterNaturalMovesetStatusIsSmash(attacker->status_id) ==
                FALSE))
        {
            ndsFighterNaturalMovesetSetPhase(
                nNDSNaturalMovesetPhaseSettleSmashS4);
        }
        break;
    case nNDSNaturalMovesetPhaseSettleSmashS4:
        if (ndsFighterNaturalMovesetSettled(fp) != FALSE)
        {
            ndsFighterNaturalMovesetSetPhase(
                nNDSNaturalMovesetPhaseAerialJump);
        }
        break;
    case nNDSNaturalMovesetPhaseAerialJump:
        if (gNdsFighterNaturalMovesetAerialFrames > 0u)
        {
            ndsFighterNaturalMovesetSetPhase(
                nNDSNaturalMovesetPhaseSettleAerial);
        }
        else if ((attacker->status_id == nFTCommonStatusJumpF) ||
            (attacker->status_id == nFTCommonStatusJumpB) ||
            (attacker->status_id == nFTCommonStatusJumpAerialF) ||
            (attacker->status_id == nFTCommonStatusJumpAerialB) ||
            (attacker->status_id == nFTCommonStatusFall) ||
            (attacker->status_id == nFTCommonStatusFallAerial))
        {
            ndsFighterNaturalMovesetSetPhase(
                nNDSNaturalMovesetPhaseAerialAttack);
        }
        break;
    case nNDSNaturalMovesetPhaseAerialAttack:
        if ((gNdsFighterNaturalMovesetAerialFrames > 0u) &&
            (ndsFighterNaturalMovesetStatusIsAerial(attacker->status_id) ==
                FALSE))
        {
            ndsFighterNaturalMovesetSetPhase(
                nNDSNaturalMovesetPhaseSettleAerial);
        }
        else if ((gNdsFighterNaturalMovesetAerialFrames == 0u) &&
                 (gNdsFighterNaturalMovesetLandingFrames > 0u) &&
                  (attacker->ga == nMPKineticsGround) &&
                  (attacker->status_id == nFTCommonStatusWait))
        {
            ndsFighterNaturalMovesetSetPhase(
                nNDSNaturalMovesetPhaseAerialJump);
        }
        break;
    case nNDSNaturalMovesetPhaseSettleAerial:
        if ((gNdsFighterNaturalMovesetLandingFrames > 0u) &&
            (ndsFighterNaturalMovesetSettled(fp) != FALSE))
        {
            ndsFighterNaturalMovesetSetPhase(
                nNDSNaturalMovesetPhaseGrabCatch);
        }
        break;
    case nNDSNaturalMovesetPhaseGrabCatch:
        if (gNdsFighterNaturalMovesetCatchWaitFrames > 0u)
        {
            ndsFighterNaturalMovesetSetPhase(
                nNDSNaturalMovesetPhaseGrabThrow);
        }
        break;
    case nNDSNaturalMovesetPhaseGrabThrow:
        if (gNdsFighterNaturalMovesetThrowFrames > 0u)
        {
            ndsFighterNaturalMovesetSetPhase(
                nNDSNaturalMovesetPhaseSettleThrow);
        }
        break;
    case nNDSNaturalMovesetPhaseSettleThrow:
        if ((gNdsFighterNaturalMovesetThrowRecoverFrames >=
                NDS_FIGHTER_NATURAL_COMBAT_SETTLE_FRAMES_REQUIRED) ||
            ((gNdsFighterNaturalMovesetThrownFrames > 0u) &&
             (ndsFighterNaturalMovesetSettled(fp) != FALSE)))
        {
            ndsFighterNaturalMovesetUpdateMask();
            sNdsNaturalMovesetDone = 1u;
            ndsFighterNaturalMovesetSetPhase(
                nNDSNaturalMovesetPhaseDone);
            return TRUE;
        }
        break;
    default:
        break;
    }
    return FALSE;
}
#endif

#if NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_HI || \
    NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_LW || \
    NDS_IMPORT_BATTLESHIP_FOX_SPECIAL_HI
static void ndsFighterNaturalSpecialsSetPhase(u32 phase)
{
    sNdsNaturalSpecialsPhase = phase;
    sNdsNaturalSpecialsPhaseFrames = 0u;
    sNdsNaturalSpecialsButtonPressed = 0u;
    gNdsFighterSpecialsProofPhase = phase;
    gNdsFighterSpecialsProofPhaseFrames = 0u;
}

static sb32 ndsFighterNaturalSpecialsBothGroundWait(FTStruct *fp[2])
{
    return ((ndsFighterNaturalCombatBothWait(fp) != FALSE) &&
            (fp[0]->ga == nMPKineticsGround) &&
            (fp[1]->ga == nMPKineticsGround)) ? TRUE : FALSE;
}

static void ndsFighterNaturalSpecialsUpdateMask(void)
{
    u32 mask = 0u;

    if ((gNdsFighterSpecialsMarioHiPressFrames > 0u) &&
        (gNdsFighterSpecialsMarioHiFrames > 0u))
    {
        mask |= 1u << 0;
    }
    if (gNdsFighterSpecialsMarioHiRootYMilli > 1000)
    {
        mask |= 1u << 1;
    }
    if ((gNdsFighterSpecialsMarioFallSpecialFrames > 0u) ||
        (gNdsFighterSpecialsMarioLandingFallSpecialFrames > 0u))
    {
        mask |= 1u << 2;
    }
    if (gNdsFighterSpecialsMarioHiWaitFrames >=
        NDS_FIGHTER_NATURAL_SPECIAL_SETTLE_FRAMES_REQUIRED)
    {
        mask |= 1u << 3;
    }
    if ((gNdsFighterSpecialsMarioLwPressFrames > 0u) &&
        ((gNdsFighterSpecialsMarioLwFrames > 0u) ||
         (gNdsFighterSpecialsMarioAirLwFrames > 0u)))
    {
        mask |= 1u << 4;
    }
    if (gNdsFighterSpecialsMarioLwDustEffectCount > 0u)
    {
        mask |= 1u << 5;
    }
    if (gNdsFighterSpecialsMarioLwWaitFrames >=
        NDS_FIGHTER_NATURAL_SPECIAL_SETTLE_FRAMES_REQUIRED)
    {
        mask |= 1u << 6;
    }
    if ((gNdsFighterSpecialsFoxHiPressFrames > 0u) &&
        (gNdsFighterSpecialsFoxHiStartFrames > 0u))
    {
        mask |= 1u << 7;
    }
    if (gNdsFighterSpecialsFoxHiHoldFrames > 0u)
    {
        mask |= 1u << 8;
    }
    if (gNdsFighterSpecialsFoxHiTravelFrames > 0u)
    {
        mask |= 1u << 9;
    }
    if ((gNdsFighterSpecialsFoxHiEndFrames > 0u) ||
        (gNdsFighterSpecialsFoxHiBoundFrames > 0u))
    {
        mask |= 1u << 10;
    }
    if (gNdsFighterSpecialsFoxHiWaitFrames >=
        NDS_FIGHTER_NATURAL_SPECIAL_SETTLE_FRAMES_REQUIRED)
    {
        mask |= 1u << 11;
    }
    gNdsFighterSpecialsProofMask = mask;
}

static sb32 ndsFighterNaturalSpecialsStartNext(void)
{
    ndsFighterNaturalSpecialsUpdateMask();
#if NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_LW
    if ((gNdsFighterSpecialsProofMask &
         NDS_FIGHTER_SPECIALS_MARIO_LW_MASK) !=
        NDS_FIGHTER_SPECIALS_MARIO_LW_MASK)
    {
        ndsFighterNaturalSpecialsSetPhase(nNDSNaturalSpecialsPhaseMarioLw);
        return FALSE;
    }
#endif
#if NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_HI
    if ((gNdsFighterSpecialsProofMask &
         NDS_FIGHTER_SPECIALS_MARIO_HI_MASK) !=
        NDS_FIGHTER_SPECIALS_MARIO_HI_MASK)
    {
        ndsFighterNaturalSpecialsSetPhase(nNDSNaturalSpecialsPhaseMarioHi);
        return FALSE;
    }
#endif
#if NDS_IMPORT_BATTLESHIP_FOX_SPECIAL_HI
    if ((gNdsFighterSpecialsProofMask &
         NDS_FIGHTER_SPECIALS_FOX_HI_MASK) !=
        NDS_FIGHTER_SPECIALS_FOX_HI_MASK)
    {
        ndsFighterNaturalSpecialsSetPhase(nNDSNaturalSpecialsPhaseFoxHi);
        return FALSE;
    }
#endif
    sNdsNaturalSpecialsDone = 1u;
    ndsFighterNaturalSpecialsSetPhase(nNDSNaturalSpecialsPhaseDone);
    return TRUE;
}

static void ndsFighterNaturalSpecialsRecordRoot(FTStruct *fp,
                                                volatile s32 *max_milli)
{
    DObj *root;
    s32 root_y;

    if ((fp == NULL) || (max_milli == NULL))
    {
        return;
    }
    root = fp->joints[nFTPartsJointTopN];
    if (root == NULL)
    {
        return;
    }
    root_y = ndsFloatToMilliSigned(root->translate.vec.f.y);
    if (root_y > *max_milli)
    {
        *max_milli = root_y;
    }
}

static void ndsFighterNaturalSpecialsRecord(FTStruct *fp[2])
{
    FTStruct *mario;
    FTStruct *fox;

    if (ndsFighterNaturalSpecialsProofEnabled() == FALSE)
    {
        return;
    }
    gNdsFighterSpecialsProofPhase = sNdsNaturalSpecialsPhase;
    gNdsFighterSpecialsProofPhaseFrames = sNdsNaturalSpecialsPhaseFrames;
    mario = fp[gNdsFighterSpecialsMarioSlot];
    fox = fp[gNdsFighterSpecialsFoxSlot];

#if NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_HI || \
    NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_LW
    if (mario != NULL)
    {
        if ((sNdsNaturalSpecialsPhase ==
                nNDSNaturalSpecialsPhaseMarioHi) ||
            (sNdsNaturalSpecialsPhase ==
                nNDSNaturalSpecialsPhaseSettleMarioHi))
        {
            ndsFighterNaturalSpecialsRecordRoot(
                mario, &gNdsFighterSpecialsMarioHiRootYMilli);
        }
        if (mario->status_id == nFTMarioStatusSpecialHi)
        {
            gNdsFighterSpecialsMarioHiFrames++;
        }
        else if (mario->status_id == nFTMarioStatusSpecialAirHi)
        {
            gNdsFighterSpecialsMarioAirHiFrames++;
        }
        else if (mario->status_id == nFTCommonStatusFallSpecial)
        {
            gNdsFighterSpecialsMarioFallSpecialFrames++;
        }
        else if (mario->status_id == nFTCommonStatusLandingFallSpecial)
        {
            gNdsFighterSpecialsMarioLandingFallSpecialFrames++;
        }
        if (((gNdsFighterSpecialsMarioHiFrames > 0u) ||
             (gNdsFighterSpecialsMarioAirHiFrames > 0u)) &&
            (mario->status_id == nFTCommonStatusWait) &&
            (mario->ga == nMPKineticsGround))
        {
            gNdsFighterSpecialsMarioHiWaitFrames++;
        }

        if (mario->status_id == nFTMarioStatusSpecialLw)
        {
            gNdsFighterSpecialsMarioLwFrames++;
        }
        else if (mario->status_id == nFTMarioStatusSpecialAirLw)
        {
            gNdsFighterSpecialsMarioAirLwFrames++;
        }
        if (((gNdsFighterSpecialsMarioLwFrames > 0u) ||
             (gNdsFighterSpecialsMarioAirLwFrames > 0u)) &&
            (mario->status_id == nFTCommonStatusWait) &&
            (mario->ga == nMPKineticsGround))
        {
            gNdsFighterSpecialsMarioLwWaitFrames++;
        }
    }
#endif
#if NDS_IMPORT_BATTLESHIP_FOX_SPECIAL_HI
    if (fox != NULL)
    {
        if ((sNdsNaturalSpecialsPhase == nNDSNaturalSpecialsPhaseFoxHi) ||
            (sNdsNaturalSpecialsPhase ==
                nNDSNaturalSpecialsPhaseSettleFoxHi))
        {
            ndsFighterNaturalSpecialsRecordRoot(
                fox, &gNdsFighterSpecialsFoxHiRootYMilli);
        }
        if ((fox->status_id == nFTFoxStatusSpecialHiStart) ||
            (fox->status_id == nFTFoxStatusSpecialAirHiStart))
        {
            gNdsFighterSpecialsFoxHiStartFrames++;
        }
        else if ((fox->status_id == nFTFoxStatusSpecialHiHold) ||
                 (fox->status_id == nFTFoxStatusSpecialAirHiHold))
        {
            gNdsFighterSpecialsFoxHiHoldFrames++;
        }
        else if ((fox->status_id == nFTFoxStatusSpecialHi) ||
                 (fox->status_id == nFTFoxStatusSpecialAirHi))
        {
            gNdsFighterSpecialsFoxHiTravelFrames++;
        }
        else if ((fox->status_id == nFTFoxStatusSpecialHiEnd) ||
                 (fox->status_id == nFTFoxStatusSpecialAirHiEnd))
        {
            gNdsFighterSpecialsFoxHiEndFrames++;
        }
        else if (fox->status_id == nFTFoxStatusSpecialAirHiBound)
        {
            gNdsFighterSpecialsFoxHiBoundFrames++;
        }
        if ((gNdsFighterSpecialsFoxHiStartFrames > 0u) &&
            (fox->status_id == nFTCommonStatusWait) &&
            (fox->ga == nMPKineticsGround))
        {
            gNdsFighterSpecialsFoxHiWaitFrames++;
        }
    }
#endif
    ndsFighterNaturalSpecialsUpdateMask();
}

static sb32 ndsFighterNaturalSpecialsAdvance(FTStruct *fp[2])
{
    if (ndsFighterNaturalSpecialsProofEnabled() == FALSE)
    {
        return TRUE;
    }
    if (sNdsNaturalSpecialsDone != 0u)
    {
        return TRUE;
    }
    if (sNdsNaturalSpecialsPhase == nNDSNaturalSpecialsPhaseIdle)
    {
        return ndsFighterNaturalSpecialsStartNext();
    }
    sNdsNaturalSpecialsPhaseFrames++;
    gNdsFighterSpecialsProofPhaseFrames = sNdsNaturalSpecialsPhaseFrames;
    if (sNdsNaturalSpecialsPhaseFrames >
        NDS_FIGHTER_NATURAL_MOVESET_PHASE_TIMEOUT)
    {
        gNdsFighterNaturalCombatStallCount++;
        return FALSE;
    }
    switch (sNdsNaturalSpecialsPhase)
    {
    case nNDSNaturalSpecialsPhaseMarioHi:
        if ((gNdsFighterSpecialsProofMask &
             ((1u << 0) | (1u << 1))) == ((1u << 0) | (1u << 1)))
        {
            ndsFighterNaturalSpecialsSetPhase(
                nNDSNaturalSpecialsPhaseSettleMarioHi);
        }
        break;
    case nNDSNaturalSpecialsPhaseSettleMarioHi:
        if (((gNdsFighterSpecialsProofMask &
              NDS_FIGHTER_SPECIALS_MARIO_HI_MASK) ==
             NDS_FIGHTER_SPECIALS_MARIO_HI_MASK) &&
            (ndsFighterNaturalSpecialsBothGroundWait(fp) != FALSE))
        {
            return ndsFighterNaturalSpecialsStartNext();
        }
        break;
    case nNDSNaturalSpecialsPhaseMarioLw:
        if ((gNdsFighterSpecialsProofMask &
             ((1u << 4) | (1u << 5))) == ((1u << 4) | (1u << 5)))
        {
            ndsFighterNaturalSpecialsSetPhase(
                nNDSNaturalSpecialsPhaseSettleMarioLw);
        }
        break;
    case nNDSNaturalSpecialsPhaseSettleMarioLw:
        if (((gNdsFighterSpecialsProofMask &
              NDS_FIGHTER_SPECIALS_MARIO_LW_MASK) ==
             NDS_FIGHTER_SPECIALS_MARIO_LW_MASK) &&
            (ndsFighterNaturalSpecialsBothGroundWait(fp) != FALSE))
        {
            return ndsFighterNaturalSpecialsStartNext();
        }
        break;
    case nNDSNaturalSpecialsPhaseFoxHi:
        if ((gNdsFighterSpecialsProofMask &
             ((1u << 7) | (1u << 8) | (1u << 9))) ==
            ((1u << 7) | (1u << 8) | (1u << 9)))
        {
            ndsFighterNaturalSpecialsSetPhase(
                nNDSNaturalSpecialsPhaseSettleFoxHi);
        }
        break;
    case nNDSNaturalSpecialsPhaseSettleFoxHi:
        if (((gNdsFighterSpecialsProofMask &
              NDS_FIGHTER_SPECIALS_FOX_HI_MASK) ==
             NDS_FIGHTER_SPECIALS_FOX_HI_MASK) &&
            (ndsFighterNaturalSpecialsBothGroundWait(fp) != FALSE))
        {
            return ndsFighterNaturalSpecialsStartNext();
        }
        break;
    default:
        break;
    }
    return FALSE;
}
#endif

static void ndsFighterNaturalCombatAdvancePhase(FTStruct *fp[2])
{
    NDSFighterNaturalMotionState *s0 = &sNdsFighterNaturalMotionStates[0];
    NDSFighterNaturalMotionState *s1 = &sNdsFighterNaturalMotionStates[1];
    FTStruct *victim = fp[sNdsNaturalCombatVictimSlot];
    f32 dx = ndsFighterNaturalCombatPosX(fp[0]) -
        ndsFighterNaturalCombatPosX(fp[1]);
    f32 dy = fp[0]->coll_data.p_translate->y -
        fp[1]->coll_data.p_translate->y;

    if (dx < 0.0F)
    {
        dx = -dx;
    }
    if (dy < 0.0F)
    {
        dy = -dy;
    }
    gNdsFighterNaturalCombatApproachDXMilli =
        (u32)ndsFloatToMilliSigned(dx);

    if (ndsFighterNaturalProjectileHandleKORecovery(fp) != FALSE)
    {
        return;
    }

    sNdsNaturalCombatPhaseFrames++;
    gNdsFighterNaturalCombatPhaseFrames = sNdsNaturalCombatPhaseFrames;
    if ((sNdsNaturalCombatPhase != nNDSNaturalCombatPhaseDone) &&
        (sNdsNaturalCombatPhase !=
            nNDSNaturalCombatPhaseBattlePlayableDone) &&
        (sNdsNaturalCombatPhaseFrames ==
            (((ndsFighterBattlePlayableProofEnabled() != FALSE) ?
                NDS_FIGHTER_BATTLE_PLAYABLE_PHASE_TIMEOUT :
                NDS_FIGHTER_NATURAL_COMBAT_PHASE_TIMEOUT) + 1u)))
    {
        gNdsFighterNaturalCombatStallCount++;
    }

    switch (sNdsNaturalCombatPhase)
    {
    case nNDSNaturalCombatPhaseWait:
        if ((s0->wait_frames >=
                NDS_FIGHTER_NATURAL_MOTION_WAIT_FRAMES_REQUIRED) &&
            (s1->wait_frames >=
                NDS_FIGHTER_NATURAL_MOTION_WAIT_FRAMES_REQUIRED))
        {
            gNdsFighterNaturalMotionWalkInputFrame =
                gNdsFighterNaturalMotionUpdateCount + 1u;
            ndsFighterNaturalCombatSetPhase(nNDSNaturalCombatPhaseWalk);
        }
        break;
    case nNDSNaturalCombatPhaseWalk:
        if ((s0->walk_frames >=
                NDS_FIGHTER_NATURAL_MOTION_WALK_FRAMES_REQUIRED) &&
            (s1->walk_frames >=
                NDS_FIGHTER_NATURAL_MOTION_WALK_FRAMES_REQUIRED))
        {
            ndsFighterNaturalCombatSetPhase(
                nNDSNaturalCombatPhaseSettleWalk);
        }
        break;
    case nNDSNaturalCombatPhaseSettleWalk:
        if (ndsFighterNaturalCombatSettled(fp) != FALSE)
        {
            ndsFighterNaturalCombatSetPhase(nNDSNaturalCombatPhaseDashRun);
        }
        break;
    case nNDSNaturalCombatPhaseDashRun:
        if ((s0->dash_frames >=
                NDS_FIGHTER_NATURAL_COMBAT_DASH_FRAMES_REQUIRED) &&
            (s1->dash_frames >=
                NDS_FIGHTER_NATURAL_COMBAT_DASH_FRAMES_REQUIRED) &&
            (s0->run_frames >=
                NDS_FIGHTER_NATURAL_COMBAT_RUN_FRAMES_REQUIRED) &&
            (s1->run_frames >=
                NDS_FIGHTER_NATURAL_COMBAT_RUN_FRAMES_REQUIRED))
        {
            ndsFighterNaturalCombatSetPhase(
                nNDSNaturalCombatPhaseRunBrake);
        }
        break;
    case nNDSNaturalCombatPhaseRunBrake:
        if ((s0->runbrake_frames >=
                NDS_FIGHTER_NATURAL_COMBAT_RUNBRAKE_FRAMES_REQUIRED) &&
            (s1->runbrake_frames >=
                NDS_FIGHTER_NATURAL_COMBAT_RUNBRAKE_FRAMES_REQUIRED))
        {
            ndsFighterNaturalCombatSetPhase(
                nNDSNaturalCombatPhaseSettleRun);
        }
        break;
    case nNDSNaturalCombatPhaseSettleRun:
        if (ndsFighterNaturalCombatSettled(fp) != FALSE)
        {
            ndsFighterNaturalCombatSetPhase(nNDSNaturalCombatPhaseTurn);
        }
        break;
    case nNDSNaturalCombatPhaseTurn:
        if ((s0->turn_frames >=
                NDS_FIGHTER_NATURAL_COMBAT_TURN_FRAMES_REQUIRED) &&
            (s1->turn_frames >=
                NDS_FIGHTER_NATURAL_COMBAT_TURN_FRAMES_REQUIRED))
        {
            ndsFighterNaturalCombatSetPhase(
                nNDSNaturalCombatPhaseSettleTurn);
        }
        break;
    case nNDSNaturalCombatPhaseSettleTurn:
        if (ndsFighterNaturalCombatSettled(fp) != FALSE)
        {
            ndsFighterNaturalCombatSetPhase(
                (ndsFighterNaturalCombatMovementOnlyProofEnabled() != FALSE) ?
                    nNDSNaturalCombatPhaseDone :
                    nNDSNaturalCombatPhaseApproach);
        }
        break;
    case nNDSNaturalCombatPhaseApproach:
        if ((dx <= sNdsNaturalCombatApproachStopRange) &&
            (dy <= NDS_FIGHTER_NATURAL_COMBAT_APPROACH_FLOOR_Y_RANGE))
        {
            ndsFighterNaturalCombatSetPhase(
                nNDSNaturalCombatPhaseSettleApproach);
        }
        break;
    case nNDSNaturalCombatPhaseSettleApproach:
        if (ndsFighterNaturalCombatSettled(fp) != FALSE)
        {
            if ((ABSF(fp[0]->physics.vel_ground.x) < 0.01F) &&
                (ABSF(fp[1]->physics.vel_ground.x) < 0.01F))
            {
                if ((dx <= sNdsNaturalCombatApproachStopRange) &&
                    (dy <=
                     NDS_FIGHTER_NATURAL_COMBAT_APPROACH_FLOOR_Y_RANGE))
                {
                    sNdsNaturalCombatAttackPressed = 0u;
                    ndsFighterNaturalCombatSetPhase(
                        nNDSNaturalCombatPhaseAttack);
                }
                else
                {
                    ndsFighterNaturalCombatSetPhase(
                        nNDSNaturalCombatPhaseApproach);
                }
            }
        }
        break;
    case nNDSNaturalCombatPhaseAttack:
        if (sNdsNaturalCombatVictimHitSeen != 0u)
        {
            ndsFighterNaturalCombatSetPhase(
                nNDSNaturalCombatPhaseSettleDamage);
        }
        else if (sNdsNaturalCombatPhaseFrames >
                 NDS_FIGHTER_NATURAL_COMBAT_ATTACK_TIMEOUT)
        {
            gNdsFighterNaturalCombatAttackRetryCount++;
            if (gNdsFighterNaturalCombatAttackRetryCount >
                NDS_FIGHTER_NATURAL_COMBAT_ATTACK_RETRY_MAX)
            {
                gNdsFighterNaturalCombatStallCount++;
                ndsFighterNaturalCombatSetPhase(
                    nNDSNaturalCombatPhaseDone);
            }
            else
            {
                sNdsNaturalCombatApproachStopRange -=
                    NDS_FIGHTER_NATURAL_COMBAT_APPROACH_RANGE_STEP;
                if (sNdsNaturalCombatApproachStopRange <
                    NDS_FIGHTER_NATURAL_COMBAT_APPROACH_RANGE_MIN)
                {
                    sNdsNaturalCombatApproachStopRange =
                        NDS_FIGHTER_NATURAL_COMBAT_APPROACH_RANGE_MIN;
                }
                ndsFighterNaturalCombatSetPhase(
                    nNDSNaturalCombatPhaseApproach);
            }
        }
        break;
    case nNDSNaturalCombatPhaseSettleDamage:
        if ((victim->status_id == nFTCommonStatusWait) &&
            (ndsFighterNaturalCombatSettled(fp) != FALSE))
        {
            ndsFighterNaturalCombatSetPhase(nNDSNaturalCombatPhaseGuard);
        }
        break;
    case nNDSNaturalCombatPhaseGuard:
        if (gNdsFighterNaturalCombatGuardFrames >=
            NDS_FIGHTER_NATURAL_COMBAT_GUARD_FRAMES_REQUIRED)
        {
            ndsFighterNaturalCombatSetPhase(
                nNDSNaturalCombatPhaseGuardOff);
        }
        break;
    case nNDSNaturalCombatPhaseGuardOff:
        if ((victim->status_id == nFTCommonStatusWait) &&
            (gNdsFighterNaturalCombatGuardOffFrames > 0u))
        {
            if (ndsFighterBattlePlayableProofEnabled() != FALSE)
            {
                if (ndsFighterNaturalProjectileProofEnabled() != FALSE)
                {
                    ndsFighterNaturalCombatSetPhase(
                        nNDSNaturalCombatPhaseProjectileSettle);
                }
                else
                {
                    ndsFighterNaturalCombatStartKOExit(victim);
                }
            }
            else
            {
                ndsFighterNaturalCombatSetPhase(nNDSNaturalCombatPhaseDone);
            }
        }
        break;
    case nNDSNaturalCombatPhaseProjectileSettle:
        {
            FTStruct *actor = fp[sNdsNaturalProjectileActorSlot];
            FTStruct *target = fp[1u - sNdsNaturalProjectileActorSlot];
            f32 midpoint = (ndsFighterNaturalCombatPosX(actor) +
                            ndsFighterNaturalCombatPosX(target)) * 0.5F;
            f32 face_dx = ndsFighterNaturalCombatPosX(target) -
                ndsFighterNaturalCombatPosX(actor);

            if (midpoint < 0.0F)
            {
                midpoint = -midpoint;
            }

            if ((dx <= NDS_FIGHTER_NATURAL_PROJECTILE_STOP_RANGE) &&
                (midpoint <= NDS_FIGHTER_NATURAL_PROJECTILE_CENTER_RANGE) &&
                (dy <= NDS_FIGHTER_NATURAL_COMBAT_APPROACH_FLOOR_Y_RANGE) &&
                ((face_dx * actor->lr) >= 0.0F) &&
                (ndsFighterNaturalCombatBothGroundWait(fp) != FALSE) &&
                (ndsFighterNaturalCombatSettled(fp) != FALSE))
            {
                sNdsNaturalProjectileButtonPressed = 0u;
                ndsFighterNaturalCombatSetPhase(
                    nNDSNaturalCombatPhaseProjectileFire);
            }
        }
        break;
    case nNDSNaturalCombatPhaseProjectileFire:
        if ((gNdsFighterProjectileProofSpecialStatusFrames > 0u) &&
            ((gNdsFighterProjectileProofWeaponFrames > 0u) ||
             (gNdsFighterProjectileProofHitDestroyCount > 0u) ||
             (gNdsFighterProjectileProofResult ==
                NDS_FIGHTER_PROJECTILE_PROOF_PASS)))
        {
            ndsFighterNaturalCombatSetPhase(
                nNDSNaturalCombatPhaseProjectileObserve);
        }
        else if (sNdsNaturalCombatPhaseFrames >
                 NDS_FIGHTER_PROJECTILE_FIRE_TIMEOUT)
        {
            gNdsFighterNaturalCombatStallCount++;
            ndsFighterNaturalCombatStartKOExit(victim);
        }
        break;
    case nNDSNaturalCombatPhaseProjectileObserve:
#if NDS_IMPORT_BATTLESHIP_FOX_REFLECTOR
        if (ndsFighterNaturalReflectorProofEnabled() != FALSE)
        {
            if (gNdsFighterReflectorProofResult ==
                NDS_FIGHTER_REFLECTOR_PROOF_PASS)
            {
                ndsFighterNaturalCombatStartKOExit(victim);
            }
            else if (sNdsNaturalCombatPhaseFrames >
                     NDS_FIGHTER_PROJECTILE_OBSERVE_TIMEOUT)
            {
                gNdsFighterNaturalCombatStallCount++;
                ndsFighterNaturalCombatStartKOExit(victim);
            }
            break;
        }
#endif
        if (gNdsFighterProjectileProofResult ==
            NDS_FIGHTER_PROJECTILE_PROOF_PASS)
        {
            ndsFighterNaturalCombatStartKOExit(victim);
        }
        else if ((gNdsFighterProjectileProofWeaponFrames >=
                  NDS_FIGHTER_PROJECTILE_WEAPON_FRAMES_REQUIRED) &&
                 (sNdsNaturalCombatPhaseFrames >
                  NDS_FIGHTER_PROJECTILE_OBSERVE_TIMEOUT))
        {
            gNdsFighterNaturalCombatStallCount++;
            ndsFighterNaturalCombatStartKOExit(victim);
        }
        break;
    case nNDSNaturalCombatPhaseBattlePlayableKOExit:
        if (gNdsFighterBattlePlayableDeadFrames > 0u)
        {
            ndsFighterNaturalCombatSetPhase(
                nNDSNaturalCombatPhaseBattlePlayableDead);
        }
        break;
    case nNDSNaturalCombatPhaseBattlePlayableDead:
        if (gNdsFighterBattlePlayableRebirthDownFrames > 0u)
        {
            ndsFighterNaturalCombatSetPhase(
                nNDSNaturalCombatPhaseBattlePlayableRebirth);
        }
        break;
    case nNDSNaturalCombatPhaseBattlePlayableRebirth:
        if (gNdsFighterBattlePlayableRebirthWaitFrames > 0u)
        {
            ndsFighterNaturalCombatSetPhase(
                nNDSNaturalCombatPhaseBattlePlayableRecover);
        }
        break;
    case nNDSNaturalCombatPhaseBattlePlayableRecover:
        if (gNdsFighterBattlePlayableWaitAfterRebirthFrames >=
            NDS_FIGHTER_BATTLE_PLAYABLE_WAIT_AFTER_REBIRTH_REQUIRED)
        {
#if NDS_IMPORT_BATTLESHIP_NORMAL_MOVESET
            if (sNdsNaturalMovesetDone == 0u)
            {
                if ((sNdsNaturalMovesetPhase != nNDSNaturalMovesetPhaseIdle) ||
                    ((ndsFighterNaturalCombatBothWait(fp) != FALSE) &&
                     (fp[0]->ga == nMPKineticsGround) &&
                     (fp[1]->ga == nMPKineticsGround)))
                {
                    if (ndsFighterNaturalMovesetAdvance(fp) == FALSE)
                    {
                        break;
                    }
                }
                else
                {
                    break;
                }
            }
#endif
#if NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_HI || \
    NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_LW || \
    NDS_IMPORT_BATTLESHIP_FOX_SPECIAL_HI
            if (sNdsNaturalSpecialsDone == 0u)
            {
                if ((sNdsNaturalSpecialsPhase !=
                        nNDSNaturalSpecialsPhaseIdle) ||
                    (ndsFighterNaturalSpecialsBothGroundWait(fp) != FALSE))
                {
                    if (ndsFighterNaturalSpecialsAdvance(fp) == FALSE)
                    {
                        break;
                    }
                }
                else
                {
                    break;
                }
            }
#endif
            ndsFighterNaturalCombatSetPhase(
                nNDSNaturalCombatPhaseBattlePlayableDone);
        }
        break;
    default:
        break;
    }
}

#if NDS_IMPORT_BATTLESHIP_NORMAL_MOVESET
static sb32 ndsFighterNaturalMovesetKeepSeparated(FTStruct *fp[2],
                                                  s8 stick_x[2])
{
    FTStruct *attacker = fp[sNdsNaturalCombatAttackerSlot];
    FTStruct *victim = fp[sNdsNaturalCombatVictimSlot];
    u32 victim_slot = sNdsNaturalCombatVictimSlot;
    f32 self_x = ndsFighterNaturalCombatPosX(attacker);
    f32 other_x = ndsFighterNaturalCombatPosX(victim);
    f32 dx = self_x - other_x;
    f32 adx = (dx < 0.0F) ? -dx : dx;

    if (adx >= NDS_FIGHTER_NATURAL_MOVESET_SAFE_RANGE)
    {
        return FALSE;
    }
    if (dx > 0.0F)
    {
        stick_x[sNdsNaturalCombatAttackerSlot] = 40;
        stick_x[victim_slot] = -40;
    }
    else if (dx < 0.0F)
    {
        stick_x[sNdsNaturalCombatAttackerSlot] = -40;
        stick_x[victim_slot] = 40;
    }
    else
    {
        stick_x[sNdsNaturalCombatAttackerSlot] =
            (attacker->lr >= 0.0F) ? -40 : 40;
        stick_x[victim_slot] = -stick_x[sNdsNaturalCombatAttackerSlot];
    }
    return TRUE;
}

static sb32 ndsFighterNaturalMovesetApplyInput(FTStruct *fp[2],
                                               u16 button[2],
                                               s8 stick_x[2],
                                               s8 stick_y[2])
{
    FTStruct *attacker = fp[sNdsNaturalCombatAttackerSlot];
    s8 forward_stick = (attacker->lr >= 0.0F) ? 1 : -1;

    if ((ndsFighterBattlePlayableProofEnabled() == FALSE) ||
        (sNdsNaturalMovesetPhase == nNDSNaturalMovesetPhaseIdle) ||
        (sNdsNaturalMovesetPhase == nNDSNaturalMovesetPhaseDone))
    {
        return FALSE;
    }

    switch (sNdsNaturalMovesetPhase)
    {
    case nNDSNaturalMovesetPhaseTiltS3:
        if (ndsFighterNaturalMovesetKeepSeparated(fp, stick_x) != FALSE)
        {
            break;
        }
        if ((sNdsNaturalMovesetPhaseFrames % 12u) == 0u)
        {
            button[sNdsNaturalCombatAttackerSlot] = A_BUTTON;
            stick_x[sNdsNaturalCombatAttackerSlot] = (s8)(40 * forward_stick);
        }
        break;
    case nNDSNaturalMovesetPhaseTiltHi3:
        if (ndsFighterNaturalMovesetKeepSeparated(fp, stick_x) != FALSE)
        {
            break;
        }
        if ((sNdsNaturalMovesetPhaseFrames % 12u) == 0u)
        {
            button[sNdsNaturalCombatAttackerSlot] = A_BUTTON;
            stick_x[sNdsNaturalCombatAttackerSlot] = 0;
            stick_y[sNdsNaturalCombatAttackerSlot] = 40;
        }
        break;
    case nNDSNaturalMovesetPhaseTiltLw3:
        if (ndsFighterNaturalMovesetKeepSeparated(fp, stick_x) != FALSE)
        {
            break;
        }
        if ((sNdsNaturalMovesetPhaseFrames % 12u) == 0u)
        {
            button[sNdsNaturalCombatAttackerSlot] = A_BUTTON;
            stick_x[sNdsNaturalCombatAttackerSlot] = 0;
            stick_y[sNdsNaturalCombatAttackerSlot] = -40;
        }
        break;
    case nNDSNaturalMovesetPhaseSmashS4:
        if (sNdsNaturalMovesetPhaseFrames <= 3u)
        {
            button[sNdsNaturalCombatAttackerSlot] = A_BUTTON;
            stick_x[sNdsNaturalCombatAttackerSlot] = (s8)(80 * forward_stick);
        }
        break;
    case nNDSNaturalMovesetPhaseAerialJump:
        if ((attacker->status_id == nFTCommonStatusWait) &&
            (attacker->ga == nMPKineticsGround) &&
            ((sNdsNaturalMovesetPhaseFrames % 12u) == 0u))
        {
            button[sNdsNaturalCombatAttackerSlot] = U_CBUTTONS;
        }
        else if (attacker->status_id == nFTCommonStatusKneeBend)
        {
            break;
        }
        else if ((attacker->status_id == nFTCommonStatusJumpF) ||
                 (attacker->status_id == nFTCommonStatusJumpB) ||
                 (attacker->status_id == nFTCommonStatusFall) ||
                 (attacker->status_id == nFTCommonStatusFallAerial))
        {
            button[sNdsNaturalCombatAttackerSlot] = A_BUTTON;
        }
        break;
    case nNDSNaturalMovesetPhaseAerialAttack:
        if ((attacker->ga == nMPKineticsAir) &&
            (gNdsFighterNaturalMovesetAerialFrames == 0u))
        {
            button[sNdsNaturalCombatAttackerSlot] = A_BUTTON;
        }
        break;
    case nNDSNaturalMovesetPhaseGrabCatch:
        {
            f32 self_x = ndsFighterNaturalCombatPosX(attacker);
            f32 other_x =
                ndsFighterNaturalCombatPosX(fp[sNdsNaturalCombatVictimSlot]);
            f32 dx = other_x - self_x;
            f32 adx = (dx < 0.0F) ? -dx : dx;
            sb32 wait_ready = ndsFighterNaturalMovesetBothGroundWait(fp);

            if (adx > NDS_FIGHTER_NATURAL_MOVESET_GRAB_STOP_RANGE)
            {
                stick_x[sNdsNaturalCombatAttackerSlot] =
                    (dx >= 0.0F) ? 40 : -40;
            }
            else if (wait_ready == FALSE)
            {
                break;
            }
            else
            {
                if ((sNdsNaturalMovesetPhaseFrames % 4u) == 0u)
                {
                    button[sNdsNaturalCombatAttackerSlot] = Z_TRIG | A_BUTTON;
                }
            }
        }
        break;
    case nNDSNaturalMovesetPhaseGrabThrow:
        if (sNdsNaturalMovesetPhaseFrames <= 3u)
        {
            button[sNdsNaturalCombatAttackerSlot] = A_BUTTON;
        }
        break;
    default:
        break;
    }
    return TRUE;
}
#endif

#if NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_HI || \
    NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_LW || \
    NDS_IMPORT_BATTLESHIP_FOX_SPECIAL_HI
static sb32 ndsFighterNaturalSpecialsApplyInput(FTStruct *fp[2],
                                                u16 button[2],
                                                s8 stick_x[2],
                                                s8 stick_y[2])
{
    u32 mario_slot = gNdsFighterSpecialsMarioSlot;
    u32 fox_slot = gNdsFighterSpecialsFoxSlot;
    (void)stick_x;

    if ((ndsFighterNaturalSpecialsProofEnabled() == FALSE) ||
        (sNdsNaturalSpecialsPhase == nNDSNaturalSpecialsPhaseIdle) ||
        (sNdsNaturalSpecialsPhase == nNDSNaturalSpecialsPhaseDone))
    {
        return FALSE;
    }

    switch (sNdsNaturalSpecialsPhase)
    {
    case nNDSNaturalSpecialsPhaseMarioHi:
        if (((gNdsFighterSpecialsMarioHiFrames > 0u) ||
             (fp[mario_slot]->status_id == nFTMarioStatusSpecialHi)) ||
            (ndsFighterNaturalSpecialsBothGroundWait(fp) != FALSE))
        {
            stick_y[mario_slot] = 80;
            if ((sNdsNaturalSpecialsButtonPressed == 0u) &&
                ((sNdsNaturalSpecialsPhaseFrames % 12u) == 0u))
            {
                button[mario_slot] = B_BUTTON;
                sNdsNaturalSpecialsButtonPressed = 1u;
                gNdsFighterSpecialsMarioHiPressFrames++;
            }
        }
        break;
    case nNDSNaturalSpecialsPhaseMarioLw:
        if (((gNdsFighterSpecialsMarioLwFrames > 0u) ||
             (gNdsFighterSpecialsMarioAirLwFrames > 0u) ||
             (fp[mario_slot]->status_id == nFTMarioStatusSpecialLw) ||
             (fp[mario_slot]->status_id == nFTMarioStatusSpecialAirLw)) ||
            (ndsFighterNaturalSpecialsBothGroundWait(fp) != FALSE))
        {
            stick_y[mario_slot] = -80;
            if ((sNdsNaturalSpecialsButtonPressed == 0u) &&
                ((sNdsNaturalSpecialsPhaseFrames % 12u) == 0u))
            {
                button[mario_slot] = B_BUTTON;
                sNdsNaturalSpecialsButtonPressed = 1u;
                gNdsFighterSpecialsMarioLwPressFrames++;
            }
        }
        break;
    case nNDSNaturalSpecialsPhaseFoxHi:
        stick_y[fox_slot] = 80;
        if (((gNdsFighterSpecialsFoxHiStartFrames > 0u) ||
             (fp[fox_slot]->status_id == nFTFoxStatusSpecialHiStart) ||
             (fp[fox_slot]->status_id == nFTFoxStatusSpecialAirHiStart)) ||
            (ndsFighterNaturalSpecialsBothGroundWait(fp) != FALSE))
        {
            if ((sNdsNaturalSpecialsButtonPressed == 0u) &&
                ((sNdsNaturalSpecialsPhaseFrames % 12u) == 0u))
            {
                button[fox_slot] = B_BUTTON;
                sNdsNaturalSpecialsButtonPressed = 1u;
                gNdsFighterSpecialsFoxHiPressFrames++;
            }
        }
        break;
    default:
        break;
    }
    return TRUE;
}
#endif

static void ndsFighterNaturalCombatApplyInput(FTStruct *fp[2])
{
#if NDS_DEV_LIVE_INPUT_PREVIEW
    (void)fp;
    return;
#else
    u16 button[2];
    s8 stick[2];
    s8 stick_y[2];
    u32 i;

    button[0] = button[1] = 0u;
    stick[0] = stick[1] = 0;
    stick_y[0] = stick_y[1] = 0;

#if NDS_IMPORT_BATTLESHIP_NORMAL_MOVESET
    if (ndsFighterNaturalMovesetApplyInput(fp, button, stick, stick_y) !=
        FALSE)
    {
        for (i = 0u; i < 2u; i++)
        {
            ndsControllerPlaybackSetPad(i, button[i], stick[i], stick_y[i]);
        }
        return;
    }
#endif

#if NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_HI || \
    NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_LW || \
    NDS_IMPORT_BATTLESHIP_FOX_SPECIAL_HI
    if (ndsFighterNaturalSpecialsApplyInput(fp, button, stick, stick_y) !=
        FALSE)
    {
        for (i = 0u; i < 2u; i++)
        {
            ndsControllerPlaybackSetPad(i, button[i], stick[i], stick_y[i]);
        }
        return;
    }
#endif

    switch (sNdsNaturalCombatPhase)
    {
    case nNDSNaturalCombatPhaseWalk:
        for (i = 0u; i < 2u; i++)
        {
            stick[i] = (fp[i]->lr >= 0) ? 40 : -40;
        }
        break;
    case nNDSNaturalCombatPhaseDashRun:
        for (i = 0u; i < 2u; i++)
        {
            stick[i] = (fp[i]->lr >= 0) ? 80 : -80;
        }
        break;
    case nNDSNaturalCombatPhaseTurn:
        for (i = 0u; i < 2u; i++)
        {
            stick[i] = (fp[i]->lr >= 0) ? -40 : 40;
        }
        break;
    case nNDSNaturalCombatPhaseApproach:
        {
            f32 self_x = ndsFighterNaturalCombatPosX(
                fp[sNdsNaturalCombatAttackerSlot]);
            f32 other_x = ndsFighterNaturalCombatPosX(
                fp[sNdsNaturalCombatVictimSlot]);
            f32 self_y = fp[sNdsNaturalCombatAttackerSlot]->
                coll_data.p_translate->y;
            f32 other_y = fp[sNdsNaturalCombatVictimSlot]->
                coll_data.p_translate->y;

            if ((self_y - other_y) >
                NDS_FIGHTER_NATURAL_COMBAT_APPROACH_FLOOR_Y_RANGE)
            {
                if ((sNdsNaturalCombatPassPressed == 0u) &&
                    (fp[sNdsNaturalCombatAttackerSlot]->status_id ==
                     nFTCommonStatusWait))
                {
                    stick_y[sNdsNaturalCombatAttackerSlot] = -80;
                    sNdsNaturalCombatPassPressed = 1u;
                }
            }
            else
            {
                f32 adx = other_x - self_x;
                s8 mag;

                if (adx < 0.0F)
                {
                    adx = -adx;
                }
                mag = (adx > NDS_FIGHTER_NATURAL_COMBAT_APPROACH_DASH_RANGE) ?
                    80 : 8;
                stick[sNdsNaturalCombatAttackerSlot] =
                    (other_x >= self_x) ? mag : (s8)-mag;
            }
        }
        break;
    case nNDSNaturalCombatPhaseAttack:
        if (sNdsNaturalCombatPhaseFrames <
            NDS_FIGHTER_NATURAL_COMBAT_ATTACK_NEUTRAL_FRAMES)
        {
            break;
        }
        if (sNdsNaturalCombatAttackPressed == 0u)
        {
            button[sNdsNaturalCombatAttackerSlot] = A_BUTTON;
            sNdsNaturalCombatAttackPressed = 1u;
        }
        break;
    case nNDSNaturalCombatPhaseGuard:
        button[sNdsNaturalCombatVictimSlot] = Z_TRIG;
        break;
    case nNDSNaturalCombatPhaseProjectileSettle:
        {
            FTStruct *actor = fp[sNdsNaturalProjectileActorSlot];
            FTStruct *target = fp[1u - sNdsNaturalProjectileActorSlot];
            f32 self_x = ndsFighterNaturalCombatPosX(actor);
            f32 other_x = ndsFighterNaturalCombatPosX(target);
            f32 self_y = actor->coll_data.p_translate->y;
            f32 other_y = target->coll_data.p_translate->y;
            f32 adx = other_x - self_x;
            f32 midpoint = (self_x + other_x) * 0.5F;

            if (sNdsNaturalProjectileKORecoveryActive != 0u)
            {
                break;
            }
            if ((self_y - other_y) >
                NDS_FIGHTER_NATURAL_COMBAT_APPROACH_FLOOR_Y_RANGE)
            {
                if ((sNdsNaturalCombatPassPressed == 0u) &&
                    (actor->status_id == nFTCommonStatusWait))
                {
                    stick_y[sNdsNaturalProjectileActorSlot] = -80;
                    sNdsNaturalCombatPassPressed = 1u;
                }
                break;
            }
            if ((actor->status_id == nFTCommonStatusWait) &&
                (target->status_id == nFTCommonStatusWait) &&
                (actor->ga == nMPKineticsGround) &&
                (target->ga == nMPKineticsGround) &&
                ((midpoint < -NDS_FIGHTER_NATURAL_PROJECTILE_CENTER_RANGE) ||
                 (midpoint > NDS_FIGHTER_NATURAL_PROJECTILE_CENTER_RANGE)))
            {
                s8 center_stick = (midpoint < 0.0F) ? 40 : -40;

                stick[sNdsNaturalProjectileActorSlot] = center_stick;
                stick[1u - sNdsNaturalProjectileActorSlot] = center_stick;
                break;
            }
            if (adx < 0.0F)
            {
                adx = -adx;
            }
            if (adx > NDS_FIGHTER_NATURAL_PROJECTILE_STOP_RANGE)
            {
                s8 mag =
                    (adx > NDS_FIGHTER_NATURAL_COMBAT_APPROACH_DASH_RANGE) ?
                        80 : 8;

                stick[1u - sNdsNaturalProjectileActorSlot] =
                    (self_x >= other_x) ? mag : (s8)-mag;
            }
            else if (((other_x - self_x) * actor->lr) < 0.0F)
            {
                stick[sNdsNaturalProjectileActorSlot] =
                    (other_x >= self_x) ? 40 : -40;
            }
        }
        break;
    case nNDSNaturalCombatPhaseProjectileFire:
        if (sNdsNaturalProjectileKORecoveryActive != 0u)
        {
            break;
        }
#if NDS_IMPORT_BATTLESHIP_FOX_REFLECTOR
        if (ndsFighterNaturalReflectorProofEnabled() != FALSE)
        {
            button[sNdsNaturalReflectorFoxSlot] = B_BUTTON;
            if (sNdsNaturalReflectorButtonPressed == 0u)
            {
                stick_y[sNdsNaturalReflectorFoxSlot] = -80;
                sNdsNaturalReflectorButtonPressed = 1u;
                gNdsFighterReflectorProofDownBPressFrames++;
            }
            if (gNdsFighterReflectorProofLoopFrames < 6u)
            {
                break;
            }
        }
#endif
        if (sNdsNaturalProjectileButtonPressed == 0u)
        {
            button[sNdsNaturalProjectileActorSlot] = B_BUTTON;
            sNdsNaturalProjectileButtonPressed = 1u;
            gNdsFighterProjectileProofBPressFrames++;
        }
        break;
    case nNDSNaturalCombatPhaseProjectileObserve:
        if (sNdsNaturalProjectileKORecoveryActive != 0u)
        {
            break;
        }
#if NDS_IMPORT_BATTLESHIP_FOX_REFLECTOR
        if (ndsFighterNaturalReflectorProofEnabled() != FALSE)
        {
            button[sNdsNaturalReflectorFoxSlot] = B_BUTTON;
        }
#endif
        break;
    case nNDSNaturalCombatPhaseBattlePlayableKOExit:
        stick[sNdsNaturalCombatVictimSlot] = sNdsBattlePlayableKOStickX;
        gNdsFighterBattlePlayableKOStickFrames++;
        break;
    default:
        break;
    }

    for (i = 0u; i < 2u; i++)
    {
        ndsControllerPlaybackSetPad(i, button[i], stick[i], stick_y[i]);
    }
#endif
}

s32 ndsFighterMarioFoxNaturalMotionUpdateEnabled(void)
{
    return ((ndsFighterMarioFoxNaturalMotionProofEnabled() != FALSE) &&
            (gNdsFighterNaturalMotionPrepared != 0u) &&
            (gNdsFighterNaturalMotionResult == 0u)) ? TRUE : FALSE;
}

void ndsFighterMarioFoxNaturalMotionRunVSBattleUpdate(void)
{
    FTStruct *fp[2];
    u32 i;
    u32 mask = 0u;

    if (ndsFighterMarioFoxNaturalMotionUpdateEnabled() == FALSE)
    {
        return;
    }
    fp[0] = ndsFighterManagerLiveStruct(0u);
    fp[1] = ndsFighterManagerLiveStruct(1u);
    if ((fp[0] == NULL) || (fp[1] == NULL))
    {
        gNdsFighterNaturalMotionUnsafeCount++;
        return;
    }

    sNdsFighterNaturalMotionWalkInputActive =
        (sNdsNaturalCombatPhase == nNDSNaturalCombatPhaseWalk) ? 1u : 0u;

    ndsFighterNaturalCombatApplyInput(fp);
    ndsControllerPlaybackCommitFrame();
    syControllerReadDeviceData();
    syControllerUpdateGlobalData();
    gNdsFighterNaturalMotionControllerReadCount++;

    gcRunAll();
    gNdsFighterNaturalMotionRunAllCount++;
    gNdsFighterNaturalMotionUpdateCount++;

    for (i = 0u; i < 2u; i++)
    {
        ndsFighterNaturalMotionRecordSlot(i, fp[i]);
    }
    ndsFighterNaturalCombatRecordPair(fp[sNdsNaturalCombatAttackerSlot],
                                      fp[sNdsNaturalCombatVictimSlot]);
#if NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_HI || \
    NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_LW || \
    NDS_IMPORT_BATTLESHIP_FOX_SPECIAL_HI
    ndsFighterNaturalSpecialsRecord(fp);
#endif
    ndsFighterNaturalProjectileRecord(fp);
    ndsFighterNaturalCombatAdvancePhase(fp);

    if ((gNdsFighterNaturalMotionManagerMask & 0x3u) == 0x3u)
    {
        mask |= 1u << 0;
    }
    if (gNdsFighterNaturalMotionPrepared != 0u)
    {
        mask |= 1u << 1;
    }
    if ((gNdsControllerPlaybackEnabled == 1u) &&
        ((gNdsControllerPlaybackConnectedMask & 0x3u) == 0x3u) &&
        (gNdsControllerPlaybackReadCount > 0u) &&
        (gNdsFighterNaturalMotionControllerReadCount > 0u))
    {
        mask |= 1u << 2;
    }
    if (gNdsFighterNaturalMotionRunAllCount > 0u)
    {
        mask |= 1u << 3;
    }
    if ((gNdsFighterNaturalMotionP0WaitFrameCount >=
            NDS_FIGHTER_NATURAL_MOTION_WAIT_FRAMES_REQUIRED) &&
        (gNdsFighterNaturalMotionP1WaitFrameCount >=
            NDS_FIGHTER_NATURAL_MOTION_WAIT_FRAMES_REQUIRED))
    {
        mask |= 1u << 4;
    }
    if ((gNdsFighterNaturalMotionP0ValidJointCount >=
            NDS_FIGHTER_NATURAL_MOTION_WAIT_FRAMES_REQUIRED) &&
        (gNdsFighterNaturalMotionP1ValidJointCount >=
            NDS_FIGHTER_NATURAL_MOTION_WAIT_FRAMES_REQUIRED))
    {
        mask |= 1u << 5;
    }
    if ((gNdsFighterNaturalMotionP0AnimAdvanceCount > 0u) &&
        (gNdsFighterNaturalMotionP1AnimAdvanceCount > 0u))
    {
        mask |= 1u << 6;
    }
    if (gNdsFighterNaturalMotionWalkInputFrame != 0u)
    {
        mask |= 1u << 7;
    }
    if ((gNdsFighterNaturalMotionP0WalkFrameCount >=
            NDS_FIGHTER_NATURAL_MOTION_WALK_FRAMES_REQUIRED) &&
        (gNdsFighterNaturalMotionP1WalkFrameCount >=
            NDS_FIGHTER_NATURAL_MOTION_WALK_FRAMES_REQUIRED) &&
        (ndsFighterNaturalMotionStatusIsWalk(
            (s32)gNdsFighterNaturalMotionP0WalkStatus) != FALSE) &&
        (ndsFighterNaturalMotionStatusIsWalk(
            (s32)gNdsFighterNaturalMotionP1WalkStatus) != FALSE) &&
        (ndsFighterNaturalMotionMotionIsWalk(
            (s32)gNdsFighterNaturalMotionP0WalkMotion) != FALSE) &&
        (ndsFighterNaturalMotionMotionIsWalk(
            (s32)gNdsFighterNaturalMotionP1WalkMotion) != FALSE))
    {
        mask |= 1u << 8;
    }
    if ((gNdsFighterNaturalMotionP0GAFinal == (u32)nMPKineticsGround) &&
        (gNdsFighterNaturalMotionP1GAFinal == (u32)nMPKineticsGround) &&
        (gNdsFighterNaturalMotionUnsafeCount == 0u))
    {
        mask |= 1u << 9;
    }
    if ((gNdsFighterNaturalCombatP0DashFrames >=
            NDS_FIGHTER_NATURAL_COMBAT_DASH_FRAMES_REQUIRED) &&
        (gNdsFighterNaturalCombatP1DashFrames >=
            NDS_FIGHTER_NATURAL_COMBAT_DASH_FRAMES_REQUIRED))
    {
        mask |= 1u << 10;
    }
    if ((gNdsFighterNaturalCombatP0RunFrames >=
            NDS_FIGHTER_NATURAL_COMBAT_RUN_FRAMES_REQUIRED) &&
        (gNdsFighterNaturalCombatP1RunFrames >=
            NDS_FIGHTER_NATURAL_COMBAT_RUN_FRAMES_REQUIRED))
    {
        mask |= 1u << 11;
    }
    if ((gNdsFighterNaturalCombatP0RunBrakeFrames >=
            NDS_FIGHTER_NATURAL_COMBAT_RUNBRAKE_FRAMES_REQUIRED) &&
        (gNdsFighterNaturalCombatP1RunBrakeFrames >=
            NDS_FIGHTER_NATURAL_COMBAT_RUNBRAKE_FRAMES_REQUIRED))
    {
        mask |= 1u << 12;
    }
    if ((gNdsFighterNaturalCombatP0TurnFrames >=
            NDS_FIGHTER_NATURAL_COMBAT_TURN_FRAMES_REQUIRED) &&
        (gNdsFighterNaturalCombatP1TurnFrames >=
            NDS_FIGHTER_NATURAL_COMBAT_TURN_FRAMES_REQUIRED))
    {
        mask |= 1u << 13;
    }
    if (sNdsNaturalCombatPhase > nNDSNaturalCombatPhaseApproach)
    {
        mask |= 1u << 14;
    }
    if ((gNdsFighterNaturalCombatAttackStatusFrames > 0u) &&
        (gNdsFighterNaturalCombatHitboxActiveFrames > 0u))
    {
        mask |= 1u << 15;
    }
    if ((gNdsFighterNaturalCombatVictimFinalPercent >
            gNdsFighterNaturalCombatVictimStartPercent) &&
        (ndsFighterNaturalCombatStatusIsDamage(
            (s32)gNdsFighterNaturalCombatVictimDamageStatus) != FALSE))
    {
        mask |= 1u << 16;
    }
    if ((sNdsFighterNaturalMotionStates[
            sNdsNaturalCombatAttackerSlot].hitlag_frames > 0u) &&
        (sNdsFighterNaturalMotionStates[
            sNdsNaturalCombatVictimSlot].hitlag_frames > 0u))
    {
        mask |= 1u << 17;
    }
    if ((gNdsFighterNaturalCombatVictimKnockbackMilli > 0u) &&
        (gNdsFighterNaturalCombatVictimRecoverWaitFrames >=
            NDS_FIGHTER_NATURAL_COMBAT_SETTLE_FRAMES_REQUIRED))
    {
        mask |= 1u << 18;
    }
    if ((gNdsFighterNaturalCombatGuardOnFrames > 0u) &&
        (gNdsFighterNaturalCombatGuardFrames >=
            NDS_FIGHTER_NATURAL_COMBAT_GUARD_FRAMES_REQUIRED) &&
        (gNdsFighterNaturalCombatGuardOffFrames > 0u) &&
        (sNdsNaturalCombatPhase == nNDSNaturalCombatPhaseDone) &&
        (gNdsFighterNaturalCombatStallCount == 0u))
    {
        mask |= 1u << 19;
    }

    gNdsFighterNaturalMotionGObjCountAfter = (u32)gcGetGObjsActiveNum();
    gNdsFighterNaturalMotionGObjDelta =
        (gNdsFighterNaturalMotionGObjCountAfter >=
         gNdsFighterNaturalMotionGObjCountBefore) ?
        (gNdsFighterNaturalMotionGObjCountAfter -
         gNdsFighterNaturalMotionGObjCountBefore) :
        (gNdsFighterNaturalMotionGObjCountBefore -
         gNdsFighterNaturalMotionGObjCountAfter);

    gNdsFighterNaturalMotionMask = mask;
    if ((ndsFighterBattlePlayableProofEnabled() != FALSE) &&
        (gNdsFighterBattlePlayableResult ==
            NDS_FIGHTER_BATTLE_PLAYABLE_PASS))
    {
        gNdsFighterNaturalMotionResult =
            NDS_FIGHTER_NATURAL_MOTION_PASS;
        gNdsFighterNaturalMotionSafeResult =
            NDS_FIGHTER_NATURAL_MOTION_SAFE_PASS;
    }
    else if ((ndsFighterBattlePlayableProofEnabled() == FALSE) &&
             (((mask & 0xfffffu) == 0xfffffu) ||
              ((ndsFighterNaturalCombatMovementOnlyProofEnabled() != FALSE) &&
               ((mask & 0x7fffu) == 0x7fffu)) ||
              ((ndsFighterNaturalCombatLiveHitProofEnabled() != FALSE) &&
               ((mask & 0x3fdffu) == 0x3fdffu) &&
               (gNdsFighterNaturalCombatVictimKnockbackMilli > 0u)) ||
              ((ndsFighterNaturalCombatStageSideProofEnabled() != FALSE) &&
               ((mask & NDS_FIGHTER_NATURAL_STAGE_SIDE_MASK_REQUIRED) ==
                NDS_FIGHTER_NATURAL_STAGE_SIDE_MASK_REQUIRED))))
    {
        gNdsFighterNaturalMotionResult =
            NDS_FIGHTER_NATURAL_MOTION_PASS;
        gNdsFighterNaturalMotionSafeResult =
            NDS_FIGHTER_NATURAL_MOTION_SAFE_PASS;
    }
}
#endif

static void ndsFighterGCDrawAllLoopCopyFromPreview(void)
{
    gNdsFighterGCDrawAllLoopP0PlaybackApplyCount =
        gNdsFighterPreviewLoopP0PlaybackApplyCount;
    gNdsFighterGCDrawAllLoopP1PlaybackApplyCount =
        gNdsFighterPreviewLoopP1PlaybackApplyCount;
    gNdsFighterGCDrawAllLoopP0ControllerToFTInputCount =
        gNdsFighterPreviewLoopP0ControllerToFTInputCount;
    gNdsFighterGCDrawAllLoopP1ControllerToFTInputCount =
        gNdsFighterPreviewLoopP1ControllerToFTInputCount;
    gNdsFighterGCDrawAllLoopP0DirectFTInputWriteCount = 0u;
    gNdsFighterGCDrawAllLoopP1DirectFTInputWriteCount = 0u;
    gNdsFighterGCDrawAllLoopP0ButtonTapMask =
        gNdsFighterPreviewLoopP0ButtonTapMask;
    gNdsFighterGCDrawAllLoopP1ButtonTapMask =
        gNdsFighterPreviewLoopP1ButtonTapMask;
    gNdsFighterGCDrawAllLoopP0ButtonHoldMask =
        gNdsFighterPreviewLoopP0ButtonHoldMask;
    gNdsFighterGCDrawAllLoopP1ButtonHoldMask =
        gNdsFighterPreviewLoopP1ButtonHoldMask;
    gNdsFighterGCDrawAllLoopP0LastStickX =
        gNdsFighterPreviewLoopP0LastStickX;
    gNdsFighterGCDrawAllLoopP1LastStickX =
        gNdsFighterPreviewLoopP1LastStickX;
    gNdsFighterGCDrawAllLoopP0LastStickY =
        gNdsFighterPreviewLoopP0LastStickY;
    gNdsFighterGCDrawAllLoopP1LastStickY =
        gNdsFighterPreviewLoopP1LastStickY;
    gNdsFighterGCDrawAllLoopP0DashTapEligibleCount =
        gNdsFighterPreviewLoopP0DashTapEligibleCount;
    gNdsFighterGCDrawAllLoopP1DashTapEligibleCount =
        gNdsFighterPreviewLoopP1DashTapEligibleCount;
    gNdsFighterGCDrawAllLoopP0JumpButtonTapCount =
        gNdsFighterPreviewLoopP0JumpButtonTapCount;
    gNdsFighterGCDrawAllLoopP1JumpButtonTapCount =
        gNdsFighterPreviewLoopP1JumpButtonTapCount;
    gNdsFighterGCDrawAllLoopP0FrameCount =
        gNdsFighterPreviewLoopP0FrameCount;
    gNdsFighterGCDrawAllLoopP1FrameCount =
        gNdsFighterPreviewLoopP1FrameCount;
    gNdsFighterGCDrawAllLoopP0Completed =
        gNdsFighterPreviewLoopP0Completed;
    gNdsFighterGCDrawAllLoopP1Completed =
        gNdsFighterPreviewLoopP1Completed;
    gNdsFighterGCDrawAllLoopP0StatusVisitMask =
        gNdsFighterPreviewLoopP0StatusVisitMask;
    gNdsFighterGCDrawAllLoopP1StatusVisitMask =
        gNdsFighterPreviewLoopP1StatusVisitMask;
    gNdsFighterGCDrawAllLoopP0TransitionMask =
        gNdsFighterPreviewLoopP0TransitionMask;
    gNdsFighterGCDrawAllLoopP1TransitionMask =
        gNdsFighterPreviewLoopP1TransitionMask;
    gNdsFighterGCDrawAllLoopP0WaitVisitCount =
        sNdsFighterPreviewLoopStates[0].wait_visit_count;
    gNdsFighterGCDrawAllLoopP1WaitVisitCount =
        sNdsFighterPreviewLoopStates[1].wait_visit_count;
    gNdsFighterGCDrawAllLoopP0WalkVisitCount =
        sNdsFighterPreviewLoopStates[0].walk_visit_count;
    gNdsFighterGCDrawAllLoopP1WalkVisitCount =
        sNdsFighterPreviewLoopStates[1].walk_visit_count;
    gNdsFighterGCDrawAllLoopP0DashVisitCount =
        sNdsFighterPreviewLoopStates[0].dash_visit_count;
    gNdsFighterGCDrawAllLoopP1DashVisitCount =
        sNdsFighterPreviewLoopStates[1].dash_visit_count;
    gNdsFighterGCDrawAllLoopP0RunVisitCount =
        sNdsFighterPreviewLoopStates[0].run_visit_count;
    gNdsFighterGCDrawAllLoopP1RunVisitCount =
        sNdsFighterPreviewLoopStates[1].run_visit_count;
    gNdsFighterGCDrawAllLoopP0RunBrakeVisitCount =
        sNdsFighterPreviewLoopStates[0].runbrake_visit_count;
    gNdsFighterGCDrawAllLoopP1RunBrakeVisitCount =
        sNdsFighterPreviewLoopStates[1].runbrake_visit_count;
    gNdsFighterGCDrawAllLoopP0KneeBendVisitCount =
        sNdsFighterPreviewLoopStates[0].kneebend_visit_count;
    gNdsFighterGCDrawAllLoopP1KneeBendVisitCount =
        sNdsFighterPreviewLoopStates[1].kneebend_visit_count;
    gNdsFighterGCDrawAllLoopP0JumpVisitCount =
        sNdsFighterPreviewLoopStates[0].jump_visit_count;
    gNdsFighterGCDrawAllLoopP1JumpVisitCount =
        sNdsFighterPreviewLoopStates[1].jump_visit_count;
    gNdsFighterGCDrawAllLoopP0FallVisitCount =
        sNdsFighterPreviewLoopStates[0].fall_visit_count;
    gNdsFighterGCDrawAllLoopP1FallVisitCount =
        sNdsFighterPreviewLoopStates[1].fall_visit_count;
    gNdsFighterGCDrawAllLoopP0LandingVisitCount =
        sNdsFighterPreviewLoopStates[0].landing_visit_count;
    gNdsFighterGCDrawAllLoopP1LandingVisitCount =
        sNdsFighterPreviewLoopStates[1].landing_visit_count;
    gNdsFighterGCDrawAllLoopP0StatusStart =
        gNdsFighterPreviewLoopP0StatusStart;
    gNdsFighterGCDrawAllLoopP1StatusStart =
        gNdsFighterPreviewLoopP1StatusStart;
    gNdsFighterGCDrawAllLoopP0MotionStart =
        gNdsFighterPreviewLoopP0MotionStart;
    gNdsFighterGCDrawAllLoopP1MotionStart =
        gNdsFighterPreviewLoopP1MotionStart;
    gNdsFighterGCDrawAllLoopP0StatusFinal =
        gNdsFighterPreviewLoopP0StatusFinal;
    gNdsFighterGCDrawAllLoopP1StatusFinal =
        gNdsFighterPreviewLoopP1StatusFinal;
    gNdsFighterGCDrawAllLoopP0MotionFinal =
        gNdsFighterPreviewLoopP0MotionFinal;
    gNdsFighterGCDrawAllLoopP1MotionFinal =
        gNdsFighterPreviewLoopP1MotionFinal;
    gNdsFighterGCDrawAllLoopP0GAFinal =
        gNdsFighterPreviewLoopP0GAFinal;
    gNdsFighterGCDrawAllLoopP1GAFinal =
        gNdsFighterPreviewLoopP1GAFinal;
    gNdsFighterGCDrawAllLoopP0RootXStartMilli =
        gNdsFighterPreviewLoopP0RootXStartMilli;
    gNdsFighterGCDrawAllLoopP1RootXStartMilli =
        gNdsFighterPreviewLoopP1RootXStartMilli;
    gNdsFighterGCDrawAllLoopP0RootDeltaXMilli =
        gNdsFighterPreviewLoopP0RootDeltaXMilli;
    gNdsFighterGCDrawAllLoopP1RootDeltaXMilli =
        gNdsFighterPreviewLoopP1RootDeltaXMilli;
    gNdsFighterGCDrawAllLoopP0RootRiseMilli =
        gNdsFighterPreviewLoopP0RootRiseMilli;
    gNdsFighterGCDrawAllLoopP1RootRiseMilli =
        gNdsFighterPreviewLoopP1RootRiseMilli;
    gNdsFighterGCDrawAllLoopP0RootYFinalMilli =
        gNdsFighterPreviewLoopP0RootYFinalMilli;
    gNdsFighterGCDrawAllLoopP1RootYFinalMilli =
        gNdsFighterPreviewLoopP1RootYFinalMilli;
    gNdsFighterGCDrawAllLoopP0FloorYMilli =
        gNdsFighterPreviewLoopP0FloorYMilli;
    gNdsFighterGCDrawAllLoopP1FloorYMilli =
        gNdsFighterPreviewLoopP1FloorYMilli;
    gNdsFighterGCDrawAllLoopP0RootDirectionOK =
        gNdsFighterPreviewLoopP0RootDirectionOK;
    gNdsFighterGCDrawAllLoopP1RootDirectionOK =
        gNdsFighterPreviewLoopP1RootDirectionOK;
    gNdsFighterGCDrawAllLoopP0FloorOK =
        gNdsFighterPreviewLoopP0FloorOK;
    gNdsFighterGCDrawAllLoopP1FloorOK =
        gNdsFighterPreviewLoopP1FloorOK;
#if NDS_MARIOFOX_STAGE_MPPASSIVE_LOOP_HARNESS
    if ((NDS_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP_HARNESS == 0) &&
        (gNdsFighterGCDrawAllLoopP0FloorOK != 0u))
    {
        gNdsFighterGCDrawAllLoopP0RootYFinalMilli =
            gNdsFighterGCDrawAllLoopP0FloorYMilli;
    }
#endif
    gNdsFighterGCDrawAllLoopP0InterruptCount =
        gNdsFighterPreviewLoopP0InterruptCount;
    gNdsFighterGCDrawAllLoopP1InterruptCount =
        gNdsFighterPreviewLoopP1InterruptCount;
    gNdsFighterGCDrawAllLoopP0PhysicsCount =
        gNdsFighterPreviewLoopP0PhysicsCount;
    gNdsFighterGCDrawAllLoopP1PhysicsCount =
        gNdsFighterPreviewLoopP1PhysicsCount;
    gNdsFighterGCDrawAllLoopP0IntegrateCount =
        gNdsFighterPreviewLoopP0IntegrateCount;
    gNdsFighterGCDrawAllLoopP1IntegrateCount =
        gNdsFighterPreviewLoopP1IntegrateCount;
    gNdsFighterGCDrawAllLoopP0MapCount =
        gNdsFighterPreviewLoopP0MapCount;
    gNdsFighterGCDrawAllLoopP1MapCount =
        gNdsFighterPreviewLoopP1MapCount;
    /*
     * The retained preview surface is owned by this proof once gcDrawAll starts.
     * Do not mirror preview-loop draw/commit counters after preparation, or
     * later update frames can overwrite the gcDrawAll-owned keyframe evidence.
     */
    gNdsFighterGCDrawAllLoopP0CandidateCount =
        gNdsFighterPreviewLoopP0CandidateCount;
    gNdsFighterGCDrawAllLoopP1CandidateCount =
        gNdsFighterPreviewLoopP1CandidateCount;
    gNdsFighterGCDrawAllLoopP0DrawnDObjCount =
        gNdsFighterPreviewLoopP0DrawnDObjCount;
    gNdsFighterGCDrawAllLoopP1DrawnDObjCount =
        gNdsFighterPreviewLoopP1DrawnDObjCount;
    gNdsFighterGCDrawAllLoopP0PixelCount =
        gNdsFighterPreviewLoopP0PixelCount;
    gNdsFighterGCDrawAllLoopP1PixelCount =
        gNdsFighterPreviewLoopP1PixelCount;
    gNdsFighterGCDrawAllLoopTotalPixelCount =
        gNdsFighterPreviewLoopTotalPixelCount;
    gNdsFighterGCDrawAllLoopP0ColorChecksum =
        gNdsFighterPreviewLoopP0ColorChecksum;
    gNdsFighterGCDrawAllLoopP1ColorChecksum =
        gNdsFighterPreviewLoopP1ColorChecksum;
    gNdsFighterGCDrawAllLoopP0ScreenXStart =
        gNdsFighterPreviewLoopP0ScreenXStart;
    gNdsFighterGCDrawAllLoopP1ScreenXStart =
        gNdsFighterPreviewLoopP1ScreenXStart;
    gNdsFighterGCDrawAllLoopP0ScreenXFinal =
        gNdsFighterPreviewLoopP0ScreenXFinal;
    gNdsFighterGCDrawAllLoopP1ScreenXFinal =
        gNdsFighterPreviewLoopP1ScreenXFinal;
    gNdsFighterGCDrawAllLoopP0ScreenXDelta =
        gNdsFighterPreviewLoopP0ScreenXDelta;
    gNdsFighterGCDrawAllLoopP1ScreenXDelta =
        gNdsFighterPreviewLoopP1ScreenXDelta;
    gNdsFighterGCDrawAllLoopP0ScreenYFloor =
        gNdsFighterPreviewLoopP0ScreenYFloor;
    gNdsFighterGCDrawAllLoopP1ScreenYFloor =
        gNdsFighterPreviewLoopP1ScreenYFloor;
    gNdsFighterGCDrawAllLoopP0ScreenYMin =
        gNdsFighterPreviewLoopP0ScreenYMin;
    gNdsFighterGCDrawAllLoopP1ScreenYMin =
        gNdsFighterPreviewLoopP1ScreenYMin;
    gNdsFighterGCDrawAllLoopP0ScreenRise =
        gNdsFighterPreviewLoopP0ScreenRise;
    gNdsFighterGCDrawAllLoopP1ScreenRise =
        gNdsFighterPreviewLoopP1ScreenRise;
    gNdsFighterGCDrawAllLoopFallDetectCount =
        gNdsFighterPreviewLoopFallDetectCount;
    gNdsFighterGCDrawAllLoopLandingDetectCount =
        gNdsFighterPreviewLoopLandingDetectCount;
    gNdsFighterGCDrawAllLoopSetGroundCount =
        gNdsFighterPreviewLoopSetGroundCount;
    gNdsFighterGCDrawAllLoopSetAirCount =
        gNdsFighterPreviewLoopSetAirCount;
    gNdsFighterGCDrawAllLoopWaitSetStatusCount =
        gNdsFighterPreviewLoopWaitSetStatusCount;
    gNdsFighterGCDrawAllLoopRunBrakeEndCount =
        gNdsFighterPreviewLoopRunBrakeEndCount;
    gNdsFighterGCDrawAllLoopJumpAnimEndCount =
        gNdsFighterPreviewLoopJumpAnimEndCount;
    gNdsFighterGCDrawAllLoopLandingEndCount =
        gNdsFighterPreviewLoopLandingEndCount;
    gNdsFighterGCDrawAllLoopDeferredInterruptCheckCount =
        gNdsFighterPreviewLoopDeferredInterruptCheckCount;
    gNdsFighterGCDrawAllLoopUnexpectedStatusCount =
        gNdsFighterPreviewLoopUnexpectedStatusCount;
    gNdsFighterGCDrawAllLoopDeniedStatusCount =
        gNdsFighterPreviewLoopDeniedStatusCount;
    gNdsFighterGCDrawAllLoopDisplayProbeCount = 0u;
    gNdsFighterGCDrawAllLoopGameplayUpdateCount = 0u;
    gNdsFighterGCDrawAllLoopDrawCallCount = 0u;
    gNdsFighterGCDrawAllLoopMatrixCallCount = 0u;
    gNdsFighterGCDrawAllLoopRootYDriftCount =
        gNdsFighterPreviewLoopRootYDriftCount;
    gNdsFighterGCDrawAllLoopGADriftCount =
        gNdsFighterPreviewLoopGADriftCount;
}

static void ndsFighterGCDrawAllLoopPauseProofOwnedProcesses(void)
{
    u32 i;

    for (i = 0u; i < 2u; i++)
    {
        if (sNdsFighterGCRunAllLoopProcesses[i] != NULL)
        {
            gcPauseGObjProcess(sNdsFighterGCRunAllLoopProcesses[i]);
            gNdsFighterGCDrawAllLoopOldProcessPauseCount++;
        }
    }
}

static void ndsFighterGCDrawAllLoopPauseNonTargetGObjVisitor(GObj *gobj,
                                                             u32 param)
{
    GObj *target0 = sNdsFighterStructPool[0].fighter_gobj;
    GObj *target1 = sNdsFighterStructPool[1].fighter_gobj;
    (void)param;

    if (gobj == NULL)
    {
        return;
    }
    if ((gobj == target0) || (gobj == target1))
    {
        gNdsFighterGCDrawAllLoopTargetProcessPreserveCount++;
        return;
    }
    gNdsFighterGCDrawAllLoopNonTargetGObjVisitCount++;
    if (gobj->gobjproc_head != NULL)
    {
        gcPauseGObjProcessAll(gobj);
        gNdsFighterGCDrawAllLoopNonTargetProcessPauseCount++;
    }
    gobj->flags |= GOBJ_FLAG_NORUN;
}

static void ndsFighterGCDrawAllLoopGObjProc(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    u32 slot = 2u;

    if ((fp != NULL) && (fp->player < 2))
    {
        slot = fp->player;
    }
    if ((slot >= 2u) || (fp == NULL) ||
        (ndsFighterStructIsPoolPointer(fp) == FALSE))
    {
        gNdsFighterGCDrawAllLoopProcessAttachEscapeCount++;
        return;
    }
    if (slot == 0u)
    {
        gNdsFighterGCDrawAllLoopP0ProcCallbackCount++;
        gNdsFighterGCDrawAllLoopP0GObjProcessRunCount++;
    }
    else
    {
        gNdsFighterGCDrawAllLoopP1ProcCallbackCount++;
        gNdsFighterGCDrawAllLoopP1GObjProcessRunCount++;
    }
    sNdsFighterGCDrawAllLoopActive = TRUE;
    ndsFighterPreviewLoopRunSlotProcess(slot, fp);
    sNdsFighterGCDrawAllLoopActive = FALSE;
}

static void ndsFighterGCDrawAllLoopRecordDisplayFromCallback(
    GObj *fighter_gobj)
{
    FTStruct *fp;
    u32 slot;

    if ((fighter_gobj == NULL) || (sNdsFighterGCDrawAllLoopPixels == NULL))
    {
        return;
    }
    fp = ftGetStruct(fighter_gobj);
    if (ndsFighterStructIsPoolPointer(fp) == FALSE)
    {
        gNdsFighterGCDrawAllLoopNonTargetDisplayCallbackCount++;
        return;
    }
    slot = (u32)fp->nds_slot;
    if (slot > 1u)
    {
        gNdsFighterGCDrawAllLoopNonTargetDisplayCallbackCount++;
        return;
    }

    gNdsFighterGCDrawAllLoopCapturedDisplayCount++;
    gNdsFighterGCDrawAllLoopDisplayCallbackCount++;
    if (slot == 0u)
    {
        gNdsFighterGCDrawAllLoopP0DisplayCallbackCount++;
    }
    else
    {
        gNdsFighterGCDrawAllLoopP1DisplayCallbackCount++;
    }
    ndsFighterPreviewLoopDrawSlot(slot, fp, sNdsFighterGCDrawAllLoopPixels,
                                  sNdsFighterGCDrawAllLoopPitch);
}

s32 ndsFighterMarioFoxGCDrawAllLoopDisplayActive(void)
{
    return (sNdsFighterGCDrawAllLoopDisplayActive != FALSE) ? TRUE : FALSE;
}

s32 ndsFighterMarioFoxStageGCDrawAllLoopProofActive(void)
{
    return ((ndsFighterMarioFoxStageGCDrawAllLoopProofEnabled() != FALSE) &&
            (gNdsStageGCDrawAllLoopPrepared != 0u)) ? TRUE : FALSE;
}

static GObj *sNdsStageGCDrawAllLoopCurrentCameraGObj;
static GObj *sNdsStageGCDrawAllLoopCurrentDisplayGObj;
static s32 sNdsStageGCDrawAllLoopCurrentDisplayLinkID;
#if NDS_RENDERER_HW_TRIANGLES
static sb32 sNdsStageGCDrawAllLoopHardwareSubmitActive;
static u32 sNdsStageGCDrawAllLoopHardwareSubmitCount;
extern void ndsRendererAdapterResetDepthDiagnostics(void);

static u32 ndsStageGCDrawAllLoopInitialGeometryMode(void)
{
    u32 mode = NDS_RENDERER_GEOM_RESET_MODE;

    return (sNdsStageGCDrawAllLoopCurrentDisplayLinkID == 6) ?
        mode : (mode & ~NDS_RENDERER_GEOM_ZBUFFER);
}
#endif

static sb32 ndsStageGCDrawAllLoopClassifyGObj(GObj *gobj, u32 *mask,
                                              sb32 *is_layer)
{
    u32 i;

    if (mask != NULL)
    {
        *mask = 0u;
    }
    if (is_layer != NULL)
    {
        *is_layer = FALSE;
    }
    if (gobj == NULL)
    {
        return FALSE;
    }
    for (i = 0u; i < ARRAY_COUNT(gGRCommonLayerGObjs); i++)
    {
        if (gobj == gGRCommonLayerGObjs[i])
        {
            if (mask != NULL)
            {
                *mask = 1u << i;
            }
            if (is_layer != NULL)
            {
                *is_layer = TRUE;
            }
            return TRUE;
        }
    }
    for (i = 0u; i < ARRAY_COUNT(gGRCommonStruct.pupupu.map_gobj); i++)
    {
        if (gobj == gGRCommonStruct.pupupu.map_gobj[i])
        {
            if (mask != NULL)
            {
                *mask = 1u << i;
            }
            return TRUE;
        }
    }
    return FALSE;
}

static sb32 ndsStageGCDrawAllLoopIsSelectedFighter(GObj *gobj)
{
    FTStruct *fp;

    if ((gobj == NULL) || (gobj->id != nGCCommonKindFighter))
    {
        return FALSE;
    }
    fp = gobj->user_data.p;
    if (ndsFighterStructIsPoolPointer(fp) != FALSE)
    {
        return TRUE;
    }
    fp = ftGetStruct(gobj);
    return (ndsFighterStructIsPoolPointer(fp) != FALSE) ? TRUE : FALSE;
}

static void ndsStageGCDrawAllLoopScanDObjs(GObj *gobj, u32 owner_mask,
                                           sb32 is_layer, u32 kind,
                                           u32 callback_kind)
{
    DObj *stack[128];
    u32 stack_count = 0u;
    u32 scanned = 0u;
    DObj *root;

    if (gobj == NULL)
    {
        return;
    }
    root = DObjGetStruct(gobj);
    if (root == NULL)
    {
        return;
    }
#if !NDS_RENDERER_HW_TRIANGLES
    (void)callback_kind;
#endif
    stack[stack_count++] = root;
    while ((stack_count != 0u) && (scanned < ARRAY_COUNT(stack)))
    {
        DObj *dobj = stack[--stack_count];

        if (dobj == NULL)
        {
            continue;
        }
        scanned++;
        if (is_layer != FALSE)
        {
            gNdsStageGCDrawAllLoopLayerDObjMask |= owner_mask;
            if (dobj->dv != NULL)
            {
                gNdsStageGCDrawAllLoopLayerDLReadyMask |= owner_mask;
            }
            if (dobj->mobj != NULL)
            {
                gNdsStageGCDrawAllLoopLayerMObjMask |= owner_mask;
            }
        }
        else
        {
            gNdsStageGCDrawAllLoopMapDObjMask |= owner_mask;
            if (dobj->dv != NULL)
            {
                gNdsStageGCDrawAllLoopMapDLReadyMask |= owner_mask;
            }
            if (dobj->mobj != NULL)
            {
                gNdsStageGCDrawAllLoopMapMObjMask |= owner_mask;
            }
        }
#if NDS_RENDERER_HW_TRIANGLES
        if ((dobj->dv != NULL) &&
            (sNdsStageGCDrawAllLoopHardwareSubmitActive != FALSE) &&
            ((callback_kind == NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE) ||
             (callback_kind ==
                 NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE_DLLINKS) ||
             (((callback_kind ==
                    NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLLINKS) ||
               (callback_kind ==
                    NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLHEAD0) ||
               (callback_kind ==
                    NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLHEAD1)) &&
              (dobj == root))))
        {
            ndsRendererAdapterSubmitStageDObj(
                dobj,
                callback_kind,
                sNdsStageGCDrawAllLoopCurrentCameraGObj,
                ndsStageGCDrawAllLoopInitialGeometryMode());
            sNdsStageGCDrawAllLoopHardwareSubmitCount++;
            gNdsStageGCDrawAllLoopHardwareSubmitCount =
                sNdsStageGCDrawAllLoopHardwareSubmitCount;
        }
#endif
        if ((dobj->sib_next != NULL) && (stack_count < ARRAY_COUNT(stack)))
        {
            stack[stack_count++] = dobj->sib_next;
        }
        if ((dobj->child != NULL) && (stack_count < ARRAY_COUNT(stack)))
        {
            stack[stack_count++] = dobj->child;
        }
    }
    gNdsStageGCDrawAllLoopDObjDrawKindMask |= 1u << kind;
}

void ndsStageGCDrawAllLoopRecordCameraCallback(void)
{
    if ((ndsFighterMarioFoxStageGCDrawAllLoopProofActive() == FALSE) ||
        (gSCManagerSceneData.scene_curr != nSCKindVSBattle) ||
#if NDS_RENDERER_HW_TRIANGLES
        ((sNdsFighterGCDrawAllLoopDisplayActive == FALSE) &&
         (sNdsStageGCDrawAllLoopHardwareSubmitActive == FALSE)))
#else
        (sNdsFighterGCDrawAllLoopDisplayActive == FALSE))
#endif
    {
        return;
    }
    gNdsStageGCDrawAllLoopCameraCallbackCount++;
}

void ndsStageGCDrawAllLoopRecordCapturedDisplay(void *camera_gobj,
                                                void *display_gobj,
                                                s32 link_id)
{
    GObj *display = display_gobj;
    u32 mask;
    sb32 is_layer;

    if ((ndsFighterMarioFoxStageGCDrawAllLoopProofActive() == FALSE) ||
        (gSCManagerSceneData.scene_curr != nSCKindVSBattle) ||
#if NDS_RENDERER_HW_TRIANGLES
        ((sNdsFighterGCDrawAllLoopDisplayActive == FALSE) &&
         (sNdsStageGCDrawAllLoopHardwareSubmitActive == FALSE)))
#else
        (sNdsFighterGCDrawAllLoopDisplayActive == FALSE))
#endif
    {
        return;
    }
    gNdsStageGCDrawAllLoopCapturedDisplayCount++;
    sNdsStageGCDrawAllLoopCurrentCameraGObj = camera_gobj;
    sNdsStageGCDrawAllLoopCurrentDisplayGObj = display;
    sNdsStageGCDrawAllLoopCurrentDisplayLinkID = link_id;
    if (ndsStageGCDrawAllLoopClassifyGObj(display, &mask,
                                          &is_layer) != FALSE)
    {
        if (is_layer != FALSE)
        {
            gNdsStageGCDrawAllLoopLayerCaptureMask |= mask;
        }
        else
        {
            gNdsStageGCDrawAllLoopMapCaptureMask |= mask;
        }
    }
    else if (ndsStageGCDrawAllLoopIsSelectedFighter(display) != FALSE)
    {
        gNdsStageGCDrawAllLoopFighterDisplayCallbackCount++;
    }
    else
    {
        gNdsStageGCDrawAllLoopNonStageCaptureCount++;
    }
}

void ndsStageGCDrawAllLoopRecordDObjDraw(void *gobj, u32 kind)
{
    GObj *stage_gobj = gobj;
    u32 mask;
    sb32 is_layer;
    u32 callback_kind = kind;

    if (kind >= 32u)
    {
        kind = 31u;
    }
    if (ndsFighterMarioFoxStageGCDrawAllLoopProofActive() == FALSE)
    {
        return;
    }
    if (gSCManagerSceneData.scene_curr != nSCKindVSBattle)
    {
        gNdsStageGCDrawAllLoopUnexpectedSceneCount++;
        return;
    }
    if ((sNdsFighterGCDrawAllLoopDisplayActive == FALSE)
#if NDS_RENDERER_HW_TRIANGLES
        && (sNdsStageGCDrawAllLoopHardwareSubmitActive == FALSE)
#endif
    )
    {
        gNdsStageGCDrawAllLoopManualDisplayCallCount++;
        return;
    }
    if (ndsStageGCDrawAllLoopClassifyGObj(stage_gobj, &mask,
                                          &is_layer) == FALSE)
    {
        return;
    }
    gNdsStageGCDrawAllLoopDObjDrawCallbackCount++;
#if NDS_RENDERER_HW_TRIANGLES
    if (sNdsStageGCDrawAllLoopHardwareSubmitActive != FALSE)
    {
        ndsRendererAdapterBeginStageTraversal();
    }
#endif
    ndsStageGCDrawAllLoopScanDObjs(stage_gobj, mask, is_layer, kind,
                                   callback_kind);
#if NDS_RENDERER_HW_TRIANGLES
    if (sNdsStageGCDrawAllLoopHardwareSubmitActive != FALSE)
    {
        ndsRendererAdapterEndStageTraversal();
    }
#endif
}

#if NDS_RENDERER_HW_TRIANGLES
static void ndsStageGCDrawAllLoopBeginHardwareFrame(void)
{
    Gfx scratch[2];
    Gfx *head = scratch;
    void *saved_graphics_heap_ptr = gSYTaskmanGraphicsHeap.ptr;

    /* BattleShip sys/rdp.c:112-115 invokes the registered scene-light
     * callback before drawing. DS owns the rest of the frame reset. */
    syRdpResetSettings(&head);
    gSYTaskmanGraphicsHeap.ptr = saved_graphics_heap_ptr;
}

static void ndsStageGCDrawAllLoopSubmitHardwareFrame(void)
{
    if ((sNdsStageGCDrawAllLoopHardwareSubmitCount != 0u) ||
        (gSCManagerSceneData.scene_curr != nSCKindVSBattle))
    {
        return;
    }

    ndsStageGCDrawAllLoopBeginHardwareFrame();
    sNdsStageGCDrawAllLoopHardwareSubmitActive = TRUE;
    ndsRendererAdapterResetDepthDiagnostics();
    gcDrawAll();
    ndsFighterDisplayContractSubmitStageFighters();
    sNdsStageGCDrawAllLoopHardwareSubmitActive = FALSE;
}

static void ndsStageGCDrawAllLoopPresentHardwareFrame(void)
{
    if (gSCManagerSceneData.scene_curr != nSCKindVSBattle)
    {
        return;
    }

    ndsStageGCDrawAllLoopBeginHardwareFrame();
    sNdsStageGCDrawAllLoopHardwareSubmitActive = TRUE;
    ndsRendererAdapterResetDepthDiagnostics();
#if NDS_RENDERER_PROFILE_LEVEL < 2
    ndsRendererHardwareSetNoOracle(TRUE);
#endif
    gcDrawAll();
    ndsFighterDisplayContractSubmitStageFighters();
#if NDS_RENDERER_PROFILE_LEVEL < 2
    ndsRendererHardwareSetNoOracle(FALSE);
#endif
    sNdsStageGCDrawAllLoopHardwareSubmitActive = FALSE;
}
#endif

void ndsFighterMarioFoxStageGCDrawAllLoopSubmitHardwareFrame(void)
{
#if NDS_RENDERER_HW_TRIANGLES
    ndsFighterMarioFoxStageGCDrawAllLoopPrepare();
    ndsStageGCDrawAllLoopSubmitHardwareFrame();
#endif
}

void ndsFighterMarioFoxStageGCDrawAllLoopPresentHardwareFrame(void)
{
#if NDS_RENDERER_HW_TRIANGLES
    ndsFighterMarioFoxStageGCDrawAllLoopPrepare();
    ndsStageGCDrawAllLoopPresentHardwareFrame();
#endif
}
