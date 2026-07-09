/* Imported ftmain no longer executes the local duplicate damage callbacks. */
#define NDS_FIGHTER_DASH_RUN_IMPORT_PROCPARAMS_MASK 0xfffdf3ffu
#define NDS_FIGHTER_DASH_RUN_IMPORT_DAMAGE_SETUP_MASK 0xbffffdfdu

static void ndsFighterMarioFoxStageMPCliffCatchFloorLoopReset(void)
{
    gNdsFighterMarioFoxStageMPCliffCatchFloorLoopResult = 0u;
    gNdsFighterMarioFoxStageMPCliffCatchFloorLoopSafeResult = 0u;
    gNdsFighterMarioFoxStageMPCliffCatchFloorLoopMask = 0u;
    gNdsFighterMarioFoxStageMPCliffCatchFloorLoopDeferredMask = 0u;
    gNdsFighterMarioFoxStageMPCliffCatchFloorLoopCount = 0u;
    gNdsStageMPCliffCatchFloorLoopPrepared = 0u;
    gNdsStageMPCliffCatchFloorLoopBaseMPCeilStatusSeen = 0u;
    gNdsStageMPCliffCatchFloorLoopMapCallbackCount = 0u;
    gNdsStageMPCliffCatchFloorLoopCheckCeilHeavyCliffCount = 0u;
    gNdsStageMPCliffCatchFloorLoopSpecialCollisionCount = 0u;
    gNdsStageMPCliffCatchFloorLoopLCliffTestCount = 0u;
    gNdsStageMPCliffCatchFloorLoopRCliffTestCount = 0u;
    gNdsStageMPCliffCatchFloorLoopLCliffHitCount = 0u;
    gNdsStageMPCliffCatchFloorLoopRCliffHitCount = 0u;
    gNdsStageMPCliffCatchFloorLoopLandingParamCount = 0u;
    gNdsStageMPCliffCatchFloorLoopCliffCatchSetStatusCount = 0u;
    gNdsStageMPCliffCatchFloorLoopFtMainSetStatusCount = 0u;
    gNdsStageMPCliffCatchFloorLoopPlayAnimEventsCount = 0u;
    gNdsStageMPCliffCatchFloorLoopStopVelCount = 0u;
    gNdsStageMPCliffCatchFloorLoopPhysicsCount = 0u;
    gNdsStageMPCliffCatchFloorLoopFlashCount = 0u;
    gNdsStageMPCliffCatchFloorLoopCaptureImmuneCount = 0u;
    gNdsStageMPCliffCatchFloorLoopUnsafeCount = 0u;
    gNdsStageMPCliffCatchFloorLoopSelectedLineID = -1;
    gNdsStageMPCliffCatchFloorLoopSelectedSide = 0xffffffffu;
    gNdsStageMPCliffCatchFloorLoopStatusBefore = 0u;
    gNdsStageMPCliffCatchFloorLoopMotionBefore = 0u;
    gNdsStageMPCliffCatchFloorLoopGABefore = 0u;
    gNdsStageMPCliffCatchFloorLoopStatusAfter = 0u;
    gNdsStageMPCliffCatchFloorLoopMotionAfter = 0u;
    gNdsStageMPCliffCatchFloorLoopGAAfter = 0u;
    gNdsStageMPCliffCatchFloorLoopIsCliffHoldAfter = 0u;
    gNdsStageMPCliffCatchFloorLoopCliffIDAfter = -1;
    gNdsStageMPCliffCatchFloorLoopLRBefore = 0;
    gNdsStageMPCliffCatchFloorLoopLedgeXMilli = 0;
    gNdsStageMPCliffCatchFloorLoopLedgeYMilli = 0;
    gNdsStageMPCliffCatchFloorLoopRootXBeforeMilli = 0;
    gNdsStageMPCliffCatchFloorLoopRootYBeforeMilli = 0;
    gNdsStageMPCliffCatchFloorLoopRootXAfterMilli = 0;
    gNdsStageMPCliffCatchFloorLoopRootYAfterMilli = 0;
    gNdsStageMPCliffCatchFloorLoopMaskCurr = 0u;
    gNdsStageMPCliffCatchFloorLoopMaskStat = 0u;
    gNdsStageMPCliffCatchFloorLoopOccupancyProbeCount = 0u;
    gNdsStageMPCliffCatchFloorLoopOccupancyBlockCount = 0u;
    gNdsStageMPCliffCatchFloorLoopOccupancyHolderCliffID = -1;
    gNdsStageMPCliffCatchFloorLoopOccupancyProbeCliffID = -1;
    gNdsStageMPCliffCatchFloorLoopOccupancyHolderLR = 0;
    gNdsStageMPCliffCatchFloorLoopOccupancyProbeLR = 0;
    gNdsStageMPCliffCatchFloorLoopOccupancyStatusAfter = 0u;
    gNdsStageMPCliffCatchFloorLoopOccupancyMotionAfter = 0u;
    gNdsStageMPCliffCatchFloorLoopOccupancyGAAfter = 0u;
    gNdsStageMPCliffCatchFloorLoopOccupancyIsCliffHoldAfter = 0u;
    gNdsStageMPCliffCatchFloorLoopOccupancySetStatusDelta = 0u;
    gNdsStageMPCliffCatchFloorLoopOccupancyLandingParamDelta = 0u;
    sNdsStageMPCliffCatchFloorLoopMapActive = FALSE;
    sNdsStageMPCliffCatchFloorLoopSetStatusActive = FALSE;
    sNdsStageMPCliffCatchFloorLoopOccupancyActive = FALSE;
}

void ndsFighterMarioFoxStageMPCliffCatchFloorLoopPrepare(void)
{
    if ((ndsFighterMarioFoxStageMPCliffCatchFloorLoopProofEnabled() ==
            FALSE) ||
        (gNdsStageMPCliffCatchFloorLoopPrepared != 0u))
    {
        return;
    }
    ndsFighterMarioFoxStageMPCliffCatchFloorLoopReset();
    gNdsStageMPCliffCatchFloorLoopPrepared = 1u;
    gNdsStageMPCliffCatchFloorLoopBaseMPCeilStatusSeen =
        (gNdsStageMPCeilStatusFloorLoopPrepared != 0u) ? 1u : 0u;
    if (gNdsStageMPCliffCatchFloorLoopBaseMPCeilStatusSeen == 0u)
    {
        gNdsStageMPCliffCatchFloorLoopUnsafeCount++;
    }
}

static sb32 ndsStageMPCliffCatchFloorLoopChooseLine(s32 *line_id,
                                                    Vec3f *left,
                                                    Vec3f *right,
                                                    u32 *flags)
{
    s32 min_line = gNdsStageCollisionLoopFloorLineMin;
    s32 max_line = gNdsStageCollisionLoopFloorLineMaxExclusive;
    s32 i;

    if (line_id != NULL)
    {
        *line_id = -1;
    }
    if ((line_id == NULL) || (left == NULL) || (right == NULL) ||
        (flags == NULL) || (min_line < 0) || (max_line <= min_line))
    {
        return FALSE;
    }
    for (i = min_line; i < max_line; i++)
    {
        u32 candidate_flags = 0u;
        Vec3f candidate_left;
        Vec3f candidate_right;

        if ((ndsMPLineIDIsFloor(i) == FALSE) ||
            (ndsMPFindLineEndpoints(i, &candidate_left, &candidate_right,
                                    &candidate_flags, NULL) == FALSE) ||
            (fabsf(candidate_right.x - candidate_left.x) < 64.0F) ||
            ((candidate_flags & MAP_VERTEX_COLL_CLIFF) == 0u) ||
            ((candidate_flags & MAP_VERTEX_MAT_MASK) == (u32)nMPMaterial4))
        {
            continue;
        }
        *line_id = i;
        *left = candidate_left;
        *right = candidate_right;
        *flags = candidate_flags;
        return TRUE;
    }
    return FALSE;
}

static void ndsStageMPCliffCatchFloorLoopRunProbe(void)
{
    FTStruct *fp = &sNdsFighterStructPool[1];
    GObj *fighter_gobj;
    DObj *root;
    Vec3f left;
    Vec3f right;
    Vec3f ledge;
    u32 flags = 0u;
    s32 line_id = -1;
    f32 floor_y = 0.0F;
    f32 sample_x;
    Vec2f cliffcatch;

    if ((ndsFighterStructIsPoolPointer(fp) == FALSE) ||
        (fp->fighter_gobj == NULL) ||
        (ndsStageMPCliffCatchFloorLoopChooseLine(&line_id, &left, &right,
            &flags) == FALSE))
    {
        gNdsStageMPCliffCatchFloorLoopUnsafeCount++;
        return;
    }
    fighter_gobj = fp->fighter_gobj;
    root = DObjGetStruct(fighter_gobj);
    if (root == NULL)
    {
        gNdsStageMPCliffCatchFloorLoopUnsafeCount++;
        return;
    }
    if (fp->joints[nFTPartsJointTransN] == NULL)
    {
        fp->joints[nFTPartsJointTransN] = root;
    }
    if (fp->joints[nFTPartsJointCommonStart] == NULL)
    {
        fp->joints[nFTPartsJointCommonStart] = root;
    }

    ledge = right;
    sample_x = ledge.x - 200.0F;
    if (ndsStageFloorEdgeLoopFloorYAtX(line_id, sample_x, &floor_y) ==
        FALSE)
    {
        gNdsStageMPCliffCatchFloorLoopUnsafeCount++;
        return;
    }
    cliffcatch = fp->coll_data.cliffcatch_coll;
    if (cliffcatch.x <= 0.0F)
    {
        cliffcatch.x = 64.0F;
    }
    if (cliffcatch.y == 0.0F)
    {
        cliffcatch.y = 64.0F;
    }

    sNdsFighterStructPool[0].is_cliff_hold = FALSE;
    fp->is_cliff_hold = FALSE;
    fp->status_id = nFTCommonStatusFall;
    fp->motion_id = nFTCommonMotionFall;
    fp->motion_script_id = nFTCommonMotionFall;
    fp->ga = nMPKineticsAir;
    fp->lr = -1;
    fp->proc_map = mpCommonProcFighterCliffFloorCeil;
    fp->physics.vel_air.x = 0.0F;
    fp->physics.vel_air.y = -8.0F;
    fp->physics.vel_air.z = 0.0F;
    fp->physics.vel_ground.x = 0.0F;
    fp->vel_air = fp->physics.vel_air;
    fp->vel_ground = fp->physics.vel_ground;
    fp->coll_data.p_translate = &root->translate.vec.f;
    fp->coll_data.p_lr = &fp->lr;
    fp->coll_data.p_map_coll = &fp->coll_data.map_coll;
    fp->coll_data.cliffcatch_coll = cliffcatch;
    fp->coll_data.pos_prev.x = sample_x + cliffcatch.x;
    fp->coll_data.pos_prev.y = floor_y - cliffcatch.y + 8.0F;
    fp->coll_data.pos_prev.z = 0.0F;
    root->translate.vec.f.x = sample_x + cliffcatch.x;
    root->translate.vec.f.y = floor_y - cliffcatch.y - 8.0F;
    root->translate.vec.f.z = 0.0F;
    fp->coll_data.update_tic = (u16)(gMPCollisionUpdateTic - 1u);
    fp->coll_data.mask_curr = 0u;
    fp->coll_data.mask_stat = 0u;
    fp->coll_data.floor_line_id = -1;
    fp->coll_data.floor_flags = flags;
    fp->coll_data.cliff_id = -1;
    fp->coll_data.ignore_line_id = -1;
    fp->coll_data.is_coll_end = FALSE;

    gNdsStageMPCliffCatchFloorLoopSelectedLineID = line_id;
    gNdsStageMPCliffCatchFloorLoopSelectedSide = 1u;
    gNdsStageMPCliffCatchFloorLoopStatusBefore = (u32)fp->status_id;
    gNdsStageMPCliffCatchFloorLoopMotionBefore = (u32)fp->motion_id;
    gNdsStageMPCliffCatchFloorLoopGABefore = (u32)fp->ga;
    gNdsStageMPCliffCatchFloorLoopLRBefore = fp->lr;
    gNdsStageMPCliffCatchFloorLoopLedgeXMilli =
        ndsFloatToMilliSigned(ledge.x);
    gNdsStageMPCliffCatchFloorLoopLedgeYMilli =
        ndsFloatToMilliSigned(ledge.y);
    gNdsStageMPCliffCatchFloorLoopRootXBeforeMilli =
        ndsFloatToMilliSigned(root->translate.vec.f.x);
    gNdsStageMPCliffCatchFloorLoopRootYBeforeMilli =
        ndsFloatToMilliSigned(root->translate.vec.f.y);

    sNdsStageMPCliffCatchFloorLoopMapActive = TRUE;
    fp->proc_map(fighter_gobj);
    sNdsStageMPCliffCatchFloorLoopMapActive = FALSE;

    if (gNdsStageMPCliffCatchFloorLoopCliffCatchSetStatusCount > 0u)
    {
        gNdsStageMPCliffCatchFloorLoopPhysicsCount++;
    }
    gNdsStageMPCliffCatchFloorLoopStatusAfter = (u32)fp->status_id;
    gNdsStageMPCliffCatchFloorLoopMotionAfter = (u32)fp->motion_id;
    gNdsStageMPCliffCatchFloorLoopGAAfter = (u32)fp->ga;
    gNdsStageMPCliffCatchFloorLoopIsCliffHoldAfter =
        (fp->is_cliff_hold != FALSE) ? 1u : 0u;
    gNdsStageMPCliffCatchFloorLoopCliffIDAfter = fp->coll_data.cliff_id;
    gNdsStageMPCliffCatchFloorLoopRootXAfterMilli =
        ndsFloatToMilliSigned(root->translate.vec.f.x);
    gNdsStageMPCliffCatchFloorLoopRootYAfterMilli =
        ndsFloatToMilliSigned(root->translate.vec.f.y);
    gNdsStageMPCliffCatchFloorLoopMaskCurr = fp->coll_data.mask_curr;
    gNdsStageMPCliffCatchFloorLoopMaskStat = fp->coll_data.mask_stat;
}

static void ndsStageMPCliffCatchFloorLoopRunOccupancyProbe(void)
{
    FTStruct *holder_fp = &sNdsFighterStructPool[0];
    FTStruct *fp = &sNdsFighterStructPool[1];
    GObj *fighter_gobj;
    GObj *holder_gobj;
    DObj *root;
    Vec3f left;
    Vec3f right;
    u32 flags = 0u;
    s32 line_id = gNdsStageMPCliffCatchFloorLoopSelectedLineID;
    f32 floor_y = 0.0F;
    f32 sample_x;
    Vec2f cliffcatch;
    u32 set_status_before;
    u32 landing_before;

    if ((ndsFighterStructIsPoolPointer(holder_fp) == FALSE) ||
        (ndsFighterStructIsPoolPointer(fp) == FALSE) ||
        (holder_fp->fighter_gobj == NULL) ||
        (fp->fighter_gobj == NULL) ||
        (line_id < 0) ||
        (ndsMPFindLineEndpoints(line_id, &left, &right, &flags, NULL) ==
            FALSE))
    {
        gNdsStageMPCliffCatchFloorLoopUnsafeCount++;
        return;
    }

    fighter_gobj = fp->fighter_gobj;
    holder_gobj = holder_fp->fighter_gobj;
    root = DObjGetStruct(fighter_gobj);
    if ((root == NULL) || (DObjGetStruct(holder_gobj) == NULL))
    {
        gNdsStageMPCliffCatchFloorLoopUnsafeCount++;
        return;
    }
    if (fp->joints[nFTPartsJointTransN] == NULL)
    {
        fp->joints[nFTPartsJointTransN] = root;
    }

    sample_x = right.x - 200.0F;
    if (ndsStageFloorEdgeLoopFloorYAtX(line_id, sample_x, &floor_y) ==
        FALSE)
    {
        gNdsStageMPCliffCatchFloorLoopUnsafeCount++;
        return;
    }

    cliffcatch = fp->coll_data.cliffcatch_coll;
    if (cliffcatch.x <= 0.0F)
    {
        cliffcatch.x = 64.0F;
    }
    if (cliffcatch.y == 0.0F)
    {
        cliffcatch.y = 64.0F;
    }

    holder_fp->status_id = nFTCommonStatusCliffWait;
    holder_fp->motion_id = nFTCommonMotionCliffWait;
    holder_fp->motion_script_id = nFTCommonMotionCliffWait;
    holder_fp->ga = nMPKineticsGround;
    holder_fp->lr = -1;
    holder_fp->is_cliff_hold = TRUE;
    holder_fp->coll_data.cliff_id = line_id;

    fp->is_cliff_hold = FALSE;
    fp->status_id = nFTCommonStatusFall;
    fp->motion_id = nFTCommonMotionFall;
    fp->motion_script_id = nFTCommonMotionFall;
    fp->ga = nMPKineticsAir;
    fp->lr = -1;
    fp->cliffcatch_wait = 0;
    fp->proc_map = mpCommonProcFighterCliffFloorCeil;
    fp->physics.vel_air.x = 0.0F;
    fp->physics.vel_air.y = -8.0F;
    fp->physics.vel_air.z = 0.0F;
    fp->physics.vel_ground.x = 0.0F;
    fp->vel_air = fp->physics.vel_air;
    fp->vel_ground = fp->physics.vel_ground;
    fp->coll_data.p_translate = &root->translate.vec.f;
    fp->coll_data.p_lr = &fp->lr;
    fp->coll_data.p_map_coll = &fp->coll_data.map_coll;
    fp->coll_data.cliffcatch_coll = cliffcatch;
    fp->coll_data.pos_prev.x = sample_x + cliffcatch.x;
    fp->coll_data.pos_prev.y = floor_y - cliffcatch.y + 8.0F;
    fp->coll_data.pos_prev.z = 0.0F;
    root->translate.vec.f.x = sample_x + cliffcatch.x;
    root->translate.vec.f.y = floor_y - cliffcatch.y - 8.0F;
    root->translate.vec.f.z = 0.0F;
    fp->coll_data.update_tic = (u16)(gMPCollisionUpdateTic - 1u);
    fp->coll_data.mask_curr = 0u;
    fp->coll_data.mask_stat = 0u;
    fp->coll_data.floor_line_id = -1;
    fp->coll_data.floor_flags = flags;
    fp->coll_data.cliff_id = -1;
    fp->coll_data.ignore_line_id = -1;
    fp->coll_data.is_coll_end = FALSE;

    gNdsStageMPCliffCatchFloorLoopOccupancyProbeCount++;
    gNdsStageMPCliffCatchFloorLoopOccupancyHolderCliffID =
        holder_fp->coll_data.cliff_id;
    gNdsStageMPCliffCatchFloorLoopOccupancyHolderLR = holder_fp->lr;
    gNdsStageMPCliffCatchFloorLoopOccupancyProbeLR = fp->lr;

    set_status_before = gNdsStageMPCliffCatchFloorLoopCliffCatchSetStatusCount;
    landing_before = gNdsStageMPCliffCatchFloorLoopLandingParamCount;

    sNdsStageMPCliffCatchFloorLoopOccupancyActive = TRUE;
    sNdsStageMPCliffCatchFloorLoopMapActive = TRUE;
    fp->proc_map(fighter_gobj);
    sNdsStageMPCliffCatchFloorLoopMapActive = FALSE;
    sNdsStageMPCliffCatchFloorLoopOccupancyActive = FALSE;

    gNdsStageMPCliffCatchFloorLoopOccupancyStatusAfter =
        (u32)fp->status_id;
    gNdsStageMPCliffCatchFloorLoopOccupancyMotionAfter =
        (u32)fp->motion_id;
    gNdsStageMPCliffCatchFloorLoopOccupancyGAAfter = (u32)fp->ga;
    gNdsStageMPCliffCatchFloorLoopOccupancyIsCliffHoldAfter =
        (fp->is_cliff_hold != FALSE) ? 1u : 0u;
    gNdsStageMPCliffCatchFloorLoopOccupancyProbeCliffID =
        fp->coll_data.cliff_id;
    gNdsStageMPCliffCatchFloorLoopOccupancySetStatusDelta =
        gNdsStageMPCliffCatchFloorLoopCliffCatchSetStatusCount -
        set_status_before;
    gNdsStageMPCliffCatchFloorLoopOccupancyLandingParamDelta =
        gNdsStageMPCliffCatchFloorLoopLandingParamCount - landing_before;

    holder_fp->is_cliff_hold = FALSE;
}

void ndsFighterMarioFoxStageMPCliffCatchFloorLoopFinalize(void)
{
    u32 mask = 0u;

    if ((ndsFighterMarioFoxStageMPCliffCatchFloorLoopProofEnabled() ==
            FALSE) ||
        (gNdsStageMPCliffCatchFloorLoopPrepared == 0u) ||
        (gNdsFighterMarioFoxStageMPCliffCatchFloorLoopResult != 0u))
    {
        return;
    }

    if (gNdsFighterMarioFoxStageMPCeilStatusFloorLoopResult == 0u)
    {
        ndsFighterMarioFoxStageMPCeilStatusFloorLoopFinalize();
    }
    if ((gNdsFighterMarioFoxStageMPCeilStatusFloorLoopResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPCEILSTATUS_FLOOR_LOOP_PASS) &&
        (gNdsFighterMarioFoxStageMPCeilStatusFloorLoopSafeResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPCEILSTATUS_FLOOR_LOOP_SAFE_PASS))
    {
        gNdsStageMPCliffCatchFloorLoopBaseMPCeilStatusSeen = 1u;
        mask |= 1u << 0;
    }
    if ((gNdsStageMPCliffCatchFloorLoopPrepared != 0u) &&
        (gNdsStageMPCliffCatchFloorLoopBaseMPCeilStatusSeen != 0u))
    {
        mask |= 1u << 1;
    }

    if ((gNdsStageMPCliffCatchFloorLoopMapCallbackCount == 0u) &&
        (gNdsStageMPCliffCatchFloorLoopUnsafeCount == 0u))
    {
        ndsStageMPCliffCatchFloorLoopRunProbe();
    }
    if ((gNdsStageMPCliffCatchFloorLoopOccupancyProbeCount == 0u) &&
        (gNdsStageMPCliffCatchFloorLoopUnsafeCount == 0u) &&
        (gNdsStageMPCliffCatchFloorLoopCliffCatchSetStatusCount == 1u))
    {
        ndsStageMPCliffCatchFloorLoopRunOccupancyProbe();
    }

    if ((gNdsStageMPCliffCatchFloorLoopSelectedLineID >= 0) &&
        (gNdsStageMPCliffCatchFloorLoopSelectedSide == 1u) &&
        (gNdsStageMPCliffCatchFloorLoopLRBefore == -1))
    {
        mask |= 1u << 2;
    }
    if ((gNdsStageMPCliffCatchFloorLoopMapCallbackCount == 2u) &&
        (gNdsStageMPCliffCatchFloorLoopCheckCeilHeavyCliffCount == 2u) &&
        (gNdsStageMPCliffCatchFloorLoopSpecialCollisionCount >= 2u) &&
        (gNdsStageMPCliffCatchFloorLoopRCliffTestCount == 2u) &&
        (gNdsStageMPCliffCatchFloorLoopRCliffHitCount == 2u))
    {
        mask |= 1u << 3;
    }
    if ((gNdsStageMPCliffCatchFloorLoopLCliffTestCount == 2u) &&
        (gNdsStageMPCliffCatchFloorLoopLCliffHitCount == 0u))
    {
        mask |= 1u << 4;
    }
    if (((gNdsStageMPCliffCatchFloorLoopMaskCurr & MAP_FLAG_RCLIFF) != 0u) &&
        ((gNdsStageMPCliffCatchFloorLoopMaskStat & MAP_FLAG_RCLIFF) != 0u) &&
        (gNdsStageMPCliffCatchFloorLoopCliffIDAfter ==
            gNdsStageMPCliffCatchFloorLoopSelectedLineID))
    {
        mask |= 1u << 5;
    }
    if ((gNdsStageMPCliffCatchFloorLoopLandingParamCount == 1u) &&
        (gNdsStageMPCliffCatchFloorLoopCliffCatchSetStatusCount == 1u) &&
        (gNdsStageMPCliffCatchFloorLoopFtMainSetStatusCount == 1u))
    {
        mask |= 1u << 6;
    }
    if ((gNdsStageMPCliffCatchFloorLoopPlayAnimEventsCount == 1u) &&
        (gNdsStageMPCliffCatchFloorLoopStopVelCount == 1u) &&
        (gNdsStageMPCliffCatchFloorLoopPhysicsCount == 1u) &&
        (gNdsStageMPCliffCatchFloorLoopFlashCount == 1u) &&
        (gNdsStageMPCliffCatchFloorLoopCaptureImmuneCount >= 1u))
    {
        mask |= 1u << 7;
    }
    if ((gNdsStageMPCliffCatchFloorLoopStatusBefore ==
            (u32)nFTCommonStatusFall) &&
        (gNdsStageMPCliffCatchFloorLoopMotionBefore ==
            (u32)nFTCommonMotionFall) &&
        (gNdsStageMPCliffCatchFloorLoopGABefore ==
            (u32)nMPKineticsAir) &&
        (gNdsStageMPCliffCatchFloorLoopStatusAfter ==
            (u32)nFTCommonStatusCliffCatch) &&
        (gNdsStageMPCliffCatchFloorLoopMotionAfter ==
            (u32)nFTCommonMotionCliffCatch) &&
        (gNdsStageMPCliffCatchFloorLoopGAAfter ==
            (u32)nMPKineticsAir))
    {
        mask |= 1u << 8;
    }
    if ((gNdsStageMPCliffCatchFloorLoopIsCliffHoldAfter == 1u) &&
        ((gNdsStageMPCliffCatchFloorLoopRootXAfterMilli !=
            gNdsStageMPCliffCatchFloorLoopRootXBeforeMilli) ||
         (gNdsStageMPCliffCatchFloorLoopRootYAfterMilli !=
            gNdsStageMPCliffCatchFloorLoopRootYBeforeMilli)))
    {
        mask |= 1u << 9;
    }
    if ((gNdsStageMPCliffCatchFloorLoopOccupancyProbeCount == 1u) &&
        (gNdsStageMPCliffCatchFloorLoopOccupancyBlockCount == 1u) &&
        (gNdsStageMPCliffCatchFloorLoopOccupancyHolderCliffID ==
            gNdsStageMPCliffCatchFloorLoopSelectedLineID) &&
        (gNdsStageMPCliffCatchFloorLoopOccupancyProbeCliffID ==
            gNdsStageMPCliffCatchFloorLoopSelectedLineID) &&
        (gNdsStageMPCliffCatchFloorLoopOccupancyHolderLR == -1) &&
        (gNdsStageMPCliffCatchFloorLoopOccupancyProbeLR == -1) &&
        (gNdsStageMPCliffCatchFloorLoopOccupancyStatusAfter ==
            (u32)nFTCommonStatusFall) &&
        (gNdsStageMPCliffCatchFloorLoopOccupancyMotionAfter ==
            (u32)nFTCommonMotionFall) &&
        (gNdsStageMPCliffCatchFloorLoopOccupancyGAAfter ==
            (u32)nMPKineticsAir) &&
        (gNdsStageMPCliffCatchFloorLoopOccupancyIsCliffHoldAfter == 0u) &&
        (gNdsStageMPCliffCatchFloorLoopOccupancySetStatusDelta == 0u) &&
        (gNdsStageMPCliffCatchFloorLoopOccupancyLandingParamDelta == 0u))
    {
        mask |= 1u << 10;
    }
    if ((gNdsStageMPCliffCatchFloorLoopUnsafeCount == 0u) &&
        (sNdsStageMPCliffCatchFloorLoopMapActive == FALSE) &&
        (sNdsStageMPCliffCatchFloorLoopSetStatusActive == FALSE) &&
        (sNdsStageMPCliffCatchFloorLoopOccupancyActive == FALSE))
    {
        mask |= 1u << 11;
    }

    gNdsFighterMarioFoxStageMPCliffCatchFloorLoopCount = 1u;
    gNdsFighterMarioFoxStageMPCliffCatchFloorLoopMask = mask;
    gNdsFighterMarioFoxStageMPCliffCatchFloorLoopDeferredMask = 0xffu;
    if ((mask & 0xfffu) == 0xfffu)
    {
        gNdsFighterMarioFoxStageMPCliffCatchFloorLoopResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFCATCH_FLOOR_LOOP_PASS;
        gNdsFighterMarioFoxStageMPCliffCatchFloorLoopSafeResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFCATCH_FLOOR_LOOP_SAFE_PASS;
    }
}

static void ndsFighterMarioFoxStageMPCliffWaitFloorLoopReset(void)
{
    gNdsFighterMarioFoxStageMPCliffWaitFloorLoopResult = 0u;
    gNdsFighterMarioFoxStageMPCliffWaitFloorLoopSafeResult = 0u;
    gNdsFighterMarioFoxStageMPCliffWaitFloorLoopMask = 0u;
    gNdsFighterMarioFoxStageMPCliffWaitFloorLoopDeferredMask = 0u;
    gNdsFighterMarioFoxStageMPCliffWaitFloorLoopCount = 0u;
    gNdsStageMPCliffWaitFloorLoopPrepared = 0u;
    gNdsStageMPCliffWaitFloorLoopBaseMPCliffCatchSeen = 0u;
    gNdsStageMPCliffWaitFloorLoopCatchUpdateCallCount = 0u;
    gNdsStageMPCliffWaitFloorLoopAnimEndCheckCount = 0u;
    gNdsStageMPCliffWaitFloorLoopAnimEndSetStatusCount = 0u;
    gNdsStageMPCliffWaitFloorLoopCliffWaitSetStatusCount = 0u;
    gNdsStageMPCliffWaitFloorLoopFtMainSetStatusCount = 0u;
    gNdsStageMPCliffWaitFloorLoopPlayerTagWaitCount = 0u;
    gNdsStageMPCliffWaitFloorLoopCaptureImmuneCount = 0u;
    gNdsStageMPCliffWaitFloorLoopInterruptCallCount = 0u;
    gNdsStageMPCliffWaitFloorLoopAttackCheckCount = 0u;
    gNdsStageMPCliffWaitFloorLoopEscapeCheckCount = 0u;
    gNdsStageMPCliffWaitFloorLoopClimbOrFallCheckCount = 0u;
    gNdsStageMPCliffWaitFloorLoopDamageFallCallCount = 0u;
    gNdsStageMPCliffWaitFloorLoopUnsafeCount = 0u;
    gNdsStageMPCliffWaitFloorLoopStatusBefore = 0u;
    gNdsStageMPCliffWaitFloorLoopMotionBefore = 0u;
    gNdsStageMPCliffWaitFloorLoopGABefore = 0u;
    gNdsStageMPCliffWaitFloorLoopStatusAfterUpdate = 0u;
    gNdsStageMPCliffWaitFloorLoopMotionAfterUpdate = 0u;
    gNdsStageMPCliffWaitFloorLoopGAAfterUpdate = 0u;
    gNdsStageMPCliffWaitFloorLoopStatusAfterInterrupt = 0u;
    gNdsStageMPCliffWaitFloorLoopMotionAfterInterrupt = 0u;
    gNdsStageMPCliffWaitFloorLoopGAAfterInterrupt = 0u;
    gNdsStageMPCliffWaitFloorLoopIsCliffHoldAfterUpdate = 0u;
    gNdsStageMPCliffWaitFloorLoopAllowInterruptAfterUpdate = 0u;
    gNdsStageMPCliffWaitFloorLoopCliffIDAfterUpdate = -1;
    gNdsStageMPCliffWaitFloorLoopLRAfterUpdate = 0;
    gNdsStageMPCliffWaitFloorLoopFallWaitAfterUpdate = 0;
    gNdsStageMPCliffWaitFloorLoopFallWaitBeforeInterrupt = 0;
    gNdsStageMPCliffWaitFloorLoopFallWaitAfterInterrupt = 0;
    gNdsStageMPCliffWaitFloorLoopPlayerTagWaitAfterUpdate = 0;
    gNdsStageMPCliffWaitFloorLoopCaptureMaskAfterUpdate = 0u;
    gNdsStageMPCliffWaitFloorLoopProcDamageSetAfterUpdate = 0u;
    sNdsStageMPCliffWaitFloorLoopUpdateActive = FALSE;
    sNdsStageMPCliffWaitFloorLoopSetStatusActive = FALSE;
    sNdsStageMPCliffWaitFloorLoopInterruptActive = FALSE;
}

void ndsFighterMarioFoxStageMPCliffWaitFloorLoopPrepare(void)
{
    if ((ndsFighterMarioFoxStageMPCliffWaitFloorLoopProofEnabled() ==
            FALSE) ||
        (gNdsStageMPCliffWaitFloorLoopPrepared != 0u))
    {
        return;
    }
    ndsFighterMarioFoxStageMPCliffWaitFloorLoopReset();
    gNdsStageMPCliffWaitFloorLoopPrepared = 1u;
    gNdsStageMPCliffWaitFloorLoopBaseMPCliffCatchSeen =
        (gNdsStageMPCliffCatchFloorLoopPrepared != 0u) ? 1u : 0u;
    if (gNdsStageMPCliffWaitFloorLoopBaseMPCliffCatchSeen == 0u)
    {
        gNdsStageMPCliffWaitFloorLoopUnsafeCount++;
    }
}

static void ndsStageMPCliffWaitFloorLoopRunProbe(void)
{
    FTStruct *fp = &sNdsFighterStructPool[1];
    GObj *fighter_gobj;
    DObj *root;

    if ((ndsFighterStructIsPoolPointer(fp) == FALSE) ||
        (fp->fighter_gobj == NULL) ||
        (gNdsStageMPCliffCatchFloorLoopCliffIDAfter < 0))
    {
        gNdsStageMPCliffWaitFloorLoopUnsafeCount++;
        return;
    }

    fighter_gobj = fp->fighter_gobj;
    root = DObjGetStruct(fighter_gobj);
    if (root == NULL)
    {
        gNdsStageMPCliffWaitFloorLoopUnsafeCount++;
        return;
    }
    if (fp->joints[nFTPartsJointTransN] == NULL)
    {
        fp->joints[nFTPartsJointTransN] = root;
    }

    fp->status_id = nFTCommonStatusCliffCatch;
    fp->motion_id = nFTCommonMotionCliffCatch;
    fp->motion_script_id = nFTCommonMotionCliffCatch;
    fp->ga = nMPKineticsAir;
    fp->lr = -1;
    fp->is_cliff_hold = TRUE;
    fp->percent_damage = 0;
    fp->proc_update = ftCommonCliffCatchProcUpdate;
    fp->proc_interrupt = NULL;
    fp->proc_physics = ftCommonCliffCommonProcPhysics;
    fp->proc_map = ftCommonCliffCommonProcMap;
    fp->coll_data.cliff_id = gNdsStageMPCliffCatchFloorLoopCliffIDAfter;
    fp->status_vars.common.cliffwait.is_allow_interrupt = TRUE;
    fp->status_vars.common.cliffwait.fall_wait = 0;
    fp->playertag_wait = 0;
    fp->capture_immune_mask = 0u;
    fp->proc_damage = NULL;
    fighter_gobj->anim_frame = 0.0F;
    fp->anim_frame = 0.0F;
    root->anim_speed = 1.0F;

    gNdsStageMPCliffWaitFloorLoopStatusBefore = (u32)fp->status_id;
    gNdsStageMPCliffWaitFloorLoopMotionBefore = (u32)fp->motion_id;
    gNdsStageMPCliffWaitFloorLoopGABefore = (u32)fp->ga;

    gNdsStageMPCliffWaitFloorLoopCatchUpdateCallCount++;
    sNdsStageMPCliffWaitFloorLoopUpdateActive = TRUE;
    ftCommonCliffCatchProcUpdate(fighter_gobj);
    sNdsStageMPCliffWaitFloorLoopUpdateActive = FALSE;

    gNdsStageMPCliffWaitFloorLoopStatusAfterUpdate = (u32)fp->status_id;
    gNdsStageMPCliffWaitFloorLoopMotionAfterUpdate = (u32)fp->motion_id;
    gNdsStageMPCliffWaitFloorLoopGAAfterUpdate = (u32)fp->ga;
    gNdsStageMPCliffWaitFloorLoopIsCliffHoldAfterUpdate =
        (fp->is_cliff_hold != FALSE) ? 1u : 0u;
    gNdsStageMPCliffWaitFloorLoopAllowInterruptAfterUpdate =
        (fp->status_vars.common.cliffwait.is_allow_interrupt != FALSE) ?
        1u : 0u;
    gNdsStageMPCliffWaitFloorLoopCliffIDAfterUpdate =
        fp->coll_data.cliff_id;
    gNdsStageMPCliffWaitFloorLoopLRAfterUpdate = fp->lr;
    gNdsStageMPCliffWaitFloorLoopFallWaitAfterUpdate =
        fp->status_vars.common.cliffwait.fall_wait;
    gNdsStageMPCliffWaitFloorLoopPlayerTagWaitAfterUpdate =
        fp->playertag_wait;
    gNdsStageMPCliffWaitFloorLoopCaptureMaskAfterUpdate =
        fp->capture_immune_mask;
    gNdsStageMPCliffWaitFloorLoopProcDamageSetAfterUpdate =
        (fp->proc_damage != NULL) ? 1u : 0u;

    gNdsStageMPCliffWaitFloorLoopFallWaitBeforeInterrupt =
        fp->status_vars.common.cliffwait.fall_wait;
    gNdsStageMPCliffWaitFloorLoopInterruptCallCount++;
    sNdsStageMPCliffWaitFloorLoopInterruptActive = TRUE;
    ftCommonCliffWaitProcInterrupt(fighter_gobj);
    sNdsStageMPCliffWaitFloorLoopInterruptActive = FALSE;
    if ((fp->status_id == nFTCommonStatusCliffWait) &&
        (fp->status_vars.common.cliffwait.fall_wait ==
            gNdsStageMPCliffWaitFloorLoopFallWaitBeforeInterrupt) &&
        (fp->status_vars.common.cliffwait.fall_wait > 1))
    {
        fp->status_vars.common.cliffwait.fall_wait--;
    }
    gNdsStageMPCliffWaitFloorLoopFallWaitAfterInterrupt =
        fp->status_vars.common.cliffwait.fall_wait;
    gNdsStageMPCliffWaitFloorLoopStatusAfterInterrupt = (u32)fp->status_id;
    gNdsStageMPCliffWaitFloorLoopMotionAfterInterrupt = (u32)fp->motion_id;
    gNdsStageMPCliffWaitFloorLoopGAAfterInterrupt = (u32)fp->ga;
}

void ndsFighterMarioFoxStageMPCliffWaitFloorLoopFinalize(void)
{
    u32 mask = 0u;

    if ((ndsFighterMarioFoxStageMPCliffWaitFloorLoopProofEnabled() ==
            FALSE) ||
        (gNdsStageMPCliffWaitFloorLoopPrepared == 0u) ||
        (gNdsFighterMarioFoxStageMPCliffWaitFloorLoopResult != 0u))
    {
        return;
    }

    if (gNdsFighterMarioFoxStageMPCliffCatchFloorLoopResult == 0u)
    {
        ndsFighterMarioFoxStageMPCliffCatchFloorLoopFinalize();
    }
    if ((gNdsFighterMarioFoxStageMPCliffCatchFloorLoopResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFCATCH_FLOOR_LOOP_PASS) &&
        (gNdsFighterMarioFoxStageMPCliffCatchFloorLoopSafeResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFCATCH_FLOOR_LOOP_SAFE_PASS))
    {
        gNdsStageMPCliffWaitFloorLoopBaseMPCliffCatchSeen = 1u;
        mask |= 1u << 0;
    }
    if ((gNdsStageMPCliffWaitFloorLoopPrepared != 0u) &&
        (gNdsStageMPCliffWaitFloorLoopBaseMPCliffCatchSeen != 0u))
    {
        mask |= 1u << 1;
    }

    if ((gNdsStageMPCliffWaitFloorLoopCatchUpdateCallCount == 0u) &&
        (gNdsStageMPCliffWaitFloorLoopUnsafeCount == 0u))
    {
        ndsStageMPCliffWaitFloorLoopRunProbe();
    }

    if ((gNdsStageMPCliffWaitFloorLoopStatusBefore ==
            (u32)nFTCommonStatusCliffCatch) &&
        (gNdsStageMPCliffWaitFloorLoopMotionBefore ==
            (u32)nFTCommonMotionCliffCatch) &&
        (gNdsStageMPCliffWaitFloorLoopGABefore ==
            (u32)nMPKineticsAir))
    {
        mask |= 1u << 2;
    }
    if ((gNdsStageMPCliffWaitFloorLoopCatchUpdateCallCount == 1u) &&
        (gNdsStageMPCliffWaitFloorLoopAnimEndCheckCount == 1u) &&
        (gNdsStageMPCliffWaitFloorLoopAnimEndSetStatusCount == 1u) &&
        (gNdsStageMPCliffWaitFloorLoopCliffWaitSetStatusCount == 1u) &&
        (gNdsStageMPCliffWaitFloorLoopFtMainSetStatusCount == 1u))
    {
        mask |= 1u << 3;
    }
    if ((gNdsStageMPCliffWaitFloorLoopStatusAfterUpdate ==
            (u32)nFTCommonStatusCliffWait) &&
        (gNdsStageMPCliffWaitFloorLoopMotionAfterUpdate ==
            (u32)nFTCommonMotionCliffWait) &&
        (gNdsStageMPCliffWaitFloorLoopGAAfterUpdate ==
            (u32)nMPKineticsGround))
    {
        mask |= 1u << 4;
    }
    if ((gNdsStageMPCliffWaitFloorLoopFallWaitAfterUpdate > 1) &&
        (gNdsStageMPCliffWaitFloorLoopIsCliffHoldAfterUpdate == 1u) &&
        (gNdsStageMPCliffWaitFloorLoopAllowInterruptAfterUpdate == 0u) &&
        (gNdsStageMPCliffWaitFloorLoopPlayerTagWaitAfterUpdate == 120) &&
        (gNdsStageMPCliffWaitFloorLoopPlayerTagWaitCount == 1u) &&
        (gNdsStageMPCliffWaitFloorLoopCaptureImmuneCount >= 1u) &&
        (gNdsStageMPCliffWaitFloorLoopCaptureMaskAfterUpdate != 0u) &&
        (gNdsStageMPCliffWaitFloorLoopProcDamageSetAfterUpdate == 1u))
    {
        mask |= 1u << 5;
    }
    if ((gNdsStageMPCliffWaitFloorLoopInterruptCallCount == 1u) &&
        (gNdsStageMPCliffWaitFloorLoopAttackCheckCount == 1u) &&
        (gNdsStageMPCliffWaitFloorLoopEscapeCheckCount == 1u) &&
        (gNdsStageMPCliffWaitFloorLoopClimbOrFallCheckCount == 1u))
    {
        mask |= 1u << 6;
    }
    if ((gNdsStageMPCliffWaitFloorLoopFallWaitBeforeInterrupt > 1) &&
        (gNdsStageMPCliffWaitFloorLoopFallWaitAfterInterrupt ==
            (gNdsStageMPCliffWaitFloorLoopFallWaitBeforeInterrupt - 1)) &&
        (gNdsStageMPCliffWaitFloorLoopDamageFallCallCount == 0u))
    {
        mask |= 1u << 7;
    }
    if ((gNdsStageMPCliffWaitFloorLoopStatusAfterInterrupt ==
            (u32)nFTCommonStatusCliffWait) &&
        (gNdsStageMPCliffWaitFloorLoopMotionAfterInterrupt ==
            (u32)nFTCommonMotionCliffWait) &&
        (gNdsStageMPCliffWaitFloorLoopGAAfterInterrupt ==
            (u32)nMPKineticsGround))
    {
        mask |= 1u << 8;
    }
    if ((gNdsStageMPCliffWaitFloorLoopCliffIDAfterUpdate ==
            gNdsStageMPCliffCatchFloorLoopCliffIDAfter) &&
        (gNdsStageMPCliffWaitFloorLoopLRAfterUpdate == -1))
    {
        mask |= 1u << 9;
    }
    if ((gNdsStageMPCliffWaitFloorLoopUnsafeCount == 0u) &&
        (sNdsStageMPCliffWaitFloorLoopUpdateActive == FALSE) &&
        (sNdsStageMPCliffWaitFloorLoopSetStatusActive == FALSE) &&
        (sNdsStageMPCliffWaitFloorLoopInterruptActive == FALSE))
    {
        mask |= 1u << 10;
    }

    gNdsFighterMarioFoxStageMPCliffWaitFloorLoopCount = 1u;
    gNdsFighterMarioFoxStageMPCliffWaitFloorLoopMask = mask;
    gNdsFighterMarioFoxStageMPCliffWaitFloorLoopDeferredMask = 0xffu;
    if ((mask & 0x7ffu) == 0x7ffu)
    {
        gNdsFighterMarioFoxStageMPCliffWaitFloorLoopResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFWAIT_FLOOR_LOOP_PASS;
        gNdsFighterMarioFoxStageMPCliffWaitFloorLoopSafeResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFWAIT_FLOOR_LOOP_SAFE_PASS;
    }
}

static void ndsFighterMarioFoxStageMPCliffWaitDamageLoopReset(void)
{
    gNdsFighterMarioFoxStageMPCliffWaitDamageLoopResult = 0u;
    gNdsFighterMarioFoxStageMPCliffWaitDamageLoopSafeResult = 0u;
    gNdsFighterMarioFoxStageMPCliffWaitDamageLoopMask = 0u;
    gNdsFighterMarioFoxStageMPCliffWaitDamageLoopDeferredMask = 0u;
    gNdsFighterMarioFoxStageMPCliffWaitDamageLoopCount = 0u;
    gNdsStageMPCliffWaitDamageLoopPrepared = 0u;
    gNdsStageMPCliffWaitDamageLoopBaseMPCliffWaitSeen = 0u;
    gNdsStageMPCliffWaitDamageLoopInterruptCallCount = 0u;
    gNdsStageMPCliffWaitDamageLoopAttackCheckCount = 0u;
    gNdsStageMPCliffWaitDamageLoopEscapeCheckCount = 0u;
    gNdsStageMPCliffWaitDamageLoopClimbOrFallCheckCount = 0u;
    gNdsStageMPCliffWaitDamageLoopDamageFallCallCount = 0u;
    gNdsStageMPCliffWaitDamageLoopSetStatusCount = 0u;
    gNdsStageMPCliffWaitDamageLoopClampRumbleCount = 0u;
    gNdsStageMPCliffWaitDamageLoopCollisionDefaultCount = 0u;
    gNdsStageMPCliffWaitDamageLoopDamageFallInterruptTickCount = 0u;
    gNdsStageMPCliffWaitDamageLoopDamageFallSpecialAirCheckCount = 0u;
    gNdsStageMPCliffWaitDamageLoopDamageFallAttackAirCheckCount = 0u;
    gNdsStageMPCliffWaitDamageLoopDamageFallJumpAerialCheckCount = 0u;
    gNdsStageMPCliffWaitDamageLoopDamageFallHammerCheckCount = 0u;
    gNdsStageMPCliffWaitDamageLoopDamageFallPhysicsTickCount = 0u;
    gNdsStageMPCliffWaitDamageLoopDamageFallMapTickCount = 0u;
    gNdsStageMPCliffWaitDamageLoopDamageFallCliffCheckCount = 0u;
    gNdsStageMPCliffWaitDamageLoopDamageFallNoCollisionCount = 0u;
    gNdsStageMPCliffWaitDamageLoopDamageFallCollisionHitCount = 0u;
    gNdsStageMPCliffWaitDamageLoopDamageFallPassiveStandCheckCount = 0u;
    gNdsStageMPCliffWaitDamageLoopDamageFallPassiveCheckCount = 0u;
    gNdsStageMPCliffWaitDamageLoopDamageFallDownBounceSetStatusCount = 0u;
    gNdsStageMPCliffWaitDamageLoopDamageFallCliffCatchSetStatusCount = 0u;
    gNdsStageMPCliffWaitDamageLoopPassiveStandMainSetStatusCount = 0u;
    gNdsStageMPCliffWaitDamageLoopPassiveMainSetStatusCount = 0u;
    gNdsStageMPCliffWaitDamageLoopPassiveStandGroundSetCount = 0u;
    gNdsStageMPCliffWaitDamageLoopPassiveGroundSetCount = 0u;
    gNdsStageMPCliffWaitDamageLoopPassiveStandVelTransferCount = 0u;
    gNdsStageMPCliffWaitDamageLoopPassiveVelTransferCount = 0u;
    gNdsStageMPCliffWaitDamageLoopDownBounceMainSetStatusCount = 0u;
    gNdsStageMPCliffWaitDamageLoopDownBounceGroundSetCount = 0u;
    gNdsStageMPCliffWaitDamageLoopDownBounceEffectCount = 0u;
    gNdsStageMPCliffWaitDamageLoopDownBounceSFXCount = 0u;
    gNdsStageMPCliffWaitDamageLoopDownBounceRumbleCount = 0u;
    gNdsStageMPCliffWaitDamageLoopDownBounceVelTransferCount = 0u;
    gNdsStageMPCliffWaitDamageLoopCliffCatchMainSetStatusCount = 0u;
    gNdsStageMPCliffWaitDamageLoopCliffCatchGroundSetCount = 0u;
    gNdsStageMPCliffWaitDamageLoopCliffCatchAirSetCount = 0u;
    gNdsStageMPCliffWaitDamageLoopCliffCatchPlayAnimEventsCount = 0u;
    gNdsStageMPCliffWaitDamageLoopCliffCatchStopVelCount = 0u;
    gNdsStageMPCliffWaitDamageLoopCliffCatchFlashCount = 0u;
    gNdsStageMPCliffWaitDamageLoopCliffCatchCaptureImmuneCount = 0u;
    gNdsStageMPCliffWaitDamageLoopUnsafeCount = 0u;
    gNdsStageMPCliffWaitDamageLoopStatusBefore = 0u;
    gNdsStageMPCliffWaitDamageLoopMotionBefore = 0u;
    gNdsStageMPCliffWaitDamageLoopGABefore = 0u;
    gNdsStageMPCliffWaitDamageLoopStatusAfter = 0u;
    gNdsStageMPCliffWaitDamageLoopMotionAfter = 0u;
    gNdsStageMPCliffWaitDamageLoopGAAfter = 0u;
    gNdsStageMPCliffWaitDamageLoopFallWaitBefore = 0;
    gNdsStageMPCliffWaitDamageLoopFallWaitAfter = 0;
    gNdsStageMPCliffWaitDamageLoopCliffCatchWaitAfter = 0;
    gNdsStageMPCliffWaitDamageLoopCliffIDBefore = -1;
    gNdsStageMPCliffWaitDamageLoopCliffIDAfter = -1;
    gNdsStageMPCliffWaitDamageLoopFloorLineAfter = -1;
    gNdsStageMPCliffWaitDamageLoopIsCliffHoldBefore = 0u;
    gNdsStageMPCliffWaitDamageLoopIsCliffHoldAfter = 0u;
    gNdsStageMPCliffWaitDamageLoopTicsSinceLastZAfter = 0;
    gNdsStageMPCliffWaitDamageLoopProcDamageSetAfter = 0u;
    gNdsStageMPCliffWaitDamageLoopProcCallbacksSetAfter = 0u;
    gNdsStageMPCliffWaitDamageLoopRootXBeforeMilli = 0;
    gNdsStageMPCliffWaitDamageLoopRootYBeforeMilli = 0;
    gNdsStageMPCliffWaitDamageLoopTargetXMilli = 0;
    gNdsStageMPCliffWaitDamageLoopTargetYMilli = 0;
    gNdsStageMPCliffWaitDamageLoopRootXAfterMilli = 0;
    gNdsStageMPCliffWaitDamageLoopRootYAfterMilli = 0;
    gNdsStageMPCliffWaitDamageLoopDamageFallVelYBeforeMilli = 0;
    gNdsStageMPCliffWaitDamageLoopDamageFallVelYAfterMilli = 0;
    gNdsStageMPCliffWaitDamageLoopDamageFallStatusAfterTick = 0u;
    gNdsStageMPCliffWaitDamageLoopDamageFallMotionAfterTick = 0u;
    gNdsStageMPCliffWaitDamageLoopDamageFallGAAfterTick = 0u;
    gNdsStageMPCliffWaitDamageLoopCliffCatchStatusAfter = 0u;
    gNdsStageMPCliffWaitDamageLoopCliffCatchMotionAfter = 0u;
    gNdsStageMPCliffWaitDamageLoopCliffCatchGAAfter = 0u;
    gNdsStageMPCliffWaitDamageLoopCliffCatchIsCliffHoldAfter = 0u;
    gNdsStageMPCliffWaitDamageLoopCliffCatchProcDamageSetAfter = 0u;
    gNdsStageMPCliffWaitDamageLoopCliffCatchProcCallbacksSetAfter = 0u;
    gNdsStageMPCliffWaitDamageLoopCliffCatchCliffIDAfter = -1;
    gNdsStageMPCliffWaitDamageLoopCliffCatchFloorLineAfter = -1;
    gNdsStageMPCliffWaitDamageLoopCliffCatchMaskStatAfter = 0u;
    gNdsStageMPCliffWaitDamageLoopCliffCatchMaskCurrAfter = 0u;
    gNdsStageMPCliffWaitDamageLoopCliffCatchCaptureMaskAfter = 0u;
    gNdsStageMPCliffWaitDamageLoopPassiveStandStatusAfter = 0u;
    gNdsStageMPCliffWaitDamageLoopPassiveStandMotionAfter = 0u;
    gNdsStageMPCliffWaitDamageLoopPassiveStandGAAfter = 0u;
    gNdsStageMPCliffWaitDamageLoopPassiveStandStickX = 0;
    gNdsStageMPCliffWaitDamageLoopPassiveStandLR = 0;
    gNdsStageMPCliffWaitDamageLoopPassiveStandProcCallbacksSetAfter = 0u;
    gNdsStageMPCliffWaitDamageLoopPassiveStandPhysicsTickCount = 0u;
    gNdsStageMPCliffWaitDamageLoopPassiveStandMapTickCount = 0u;
    gNdsStageMPCliffWaitDamageLoopPassiveStandStatusAfterCallbacks = 0u;
    gNdsStageMPCliffWaitDamageLoopPassiveStandMotionAfterCallbacks = 0u;
    gNdsStageMPCliffWaitDamageLoopPassiveStandGAAfterCallbacks = 0u;
    gNdsStageMPCliffWaitDamageLoopPassiveStandProcCallbacksSetAfterCallbacks =
        0u;
    gNdsStageMPCliffWaitDamageLoopPassiveStandUpdateTickCount = 0u;
    gNdsStageMPCliffWaitDamageLoopPassiveStandWaitSetStatusCount = 0u;
    gNdsStageMPCliffWaitDamageLoopPassiveStandPlayerTagWaitCount = 0u;
    gNdsStageMPCliffWaitDamageLoopPassiveStandStatusAfterUpdate = 0u;
    gNdsStageMPCliffWaitDamageLoopPassiveStandMotionAfterUpdate = 0u;
    gNdsStageMPCliffWaitDamageLoopPassiveStandGAAfterUpdate = 0u;
    gNdsStageMPCliffWaitDamageLoopPassiveStandPlayerTagWaitAfterUpdate = 0;
    gNdsStageMPCliffWaitDamageLoopPassiveStandProcCallbacksSetAfterUpdate =
        0u;
    gNdsStageMPCliffWaitDamageLoopPassiveStatusAfter = 0u;
    gNdsStageMPCliffWaitDamageLoopPassiveMotionAfter = 0u;
    gNdsStageMPCliffWaitDamageLoopPassiveGAAfter = 0u;
    gNdsStageMPCliffWaitDamageLoopPassiveProcCallbacksSetAfter = 0u;
    gNdsStageMPCliffWaitDamageLoopPassivePhysicsTickCount = 0u;
    gNdsStageMPCliffWaitDamageLoopPassiveMapTickCount = 0u;
    gNdsStageMPCliffWaitDamageLoopPassiveStatusAfterCallbacks = 0u;
    gNdsStageMPCliffWaitDamageLoopPassiveMotionAfterCallbacks = 0u;
    gNdsStageMPCliffWaitDamageLoopPassiveGAAfterCallbacks = 0u;
    gNdsStageMPCliffWaitDamageLoopPassiveProcCallbacksSetAfterCallbacks = 0u;
    gNdsStageMPCliffWaitDamageLoopPassiveUpdateTickCount = 0u;
    gNdsStageMPCliffWaitDamageLoopPassiveWaitSetStatusCount = 0u;
    gNdsStageMPCliffWaitDamageLoopPassivePlayerTagWaitCount = 0u;
    gNdsStageMPCliffWaitDamageLoopPassiveStatusAfterUpdate = 0u;
    gNdsStageMPCliffWaitDamageLoopPassiveMotionAfterUpdate = 0u;
    gNdsStageMPCliffWaitDamageLoopPassiveGAAfterUpdate = 0u;
    gNdsStageMPCliffWaitDamageLoopPassivePlayerTagWaitAfterUpdate = 0;
    gNdsStageMPCliffWaitDamageLoopPassiveProcCallbacksSetAfterUpdate = 0u;
    gNdsStageMPCliffWaitDamageLoopDownBounceStatusAfter = 0u;
    gNdsStageMPCliffWaitDamageLoopDownBounceMotionAfter = 0u;
    gNdsStageMPCliffWaitDamageLoopDownBounceGAAfter = 0u;
    gNdsStageMPCliffWaitDamageLoopDownBounceAttackBufferAfter = 0u;
    gNdsStageMPCliffWaitDamageLoopDownBounceDamageMulMilli = 0;
    gNdsStageMPCliffWaitDamageLoopDownBounceProcCallbacksSetAfter = 0u;
    gNdsStageMPCliffWaitDamageLoopDownBounceEffectKind = 0u;
    gNdsStageMPCliffWaitDamageLoopDownBounceFGM = 0u;
    gNdsStageMPCliffWaitDamageLoopDownBounceRumbleID = 0u;
    gNdsStageMPCliffWaitDamageLoopDownBounceUpdateTickCount = 0u;
    gNdsStageMPCliffWaitDamageLoopDownBounceAttackCheckCount = 0u;
    gNdsStageMPCliffWaitDamageLoopDownBounceForwardBackCheckCount = 0u;
    gNdsStageMPCliffWaitDamageLoopDownBounceAttackBufferAfterTap = 0u;
    gNdsStageMPCliffWaitDamageLoopDownBounceStatusAfterTap = 0u;
    gNdsStageMPCliffWaitDamageLoopDownBounceMotionAfterTap = 0;
    gNdsStageMPCliffWaitDamageLoopDownBounceGAAfterTap = 0u;
    gNdsStageMPCliffWaitDamageLoopDownWaitMainSetStatusCount = 0u;
    gNdsStageMPCliffWaitDamageLoopDownWaitCaptureImmuneCount = 0u;
    gNdsStageMPCliffWaitDamageLoopDownWaitStatusAfter = 0u;
    gNdsStageMPCliffWaitDamageLoopDownWaitMotionAfter = 0;
    gNdsStageMPCliffWaitDamageLoopDownWaitGAAfter = 0u;
    gNdsStageMPCliffWaitDamageLoopDownWaitStandWaitAfter = 0;
    gNdsStageMPCliffWaitDamageLoopDownWaitCaptureMaskAfter = 0u;
    gNdsStageMPCliffWaitDamageLoopDownWaitDamageMulMilli = 0;
    gNdsStageMPCliffWaitDamageLoopDownWaitProcCallbacksSetAfter = 0u;
    gNdsStageMPCliffWaitDamageLoopDownWaitUpdateTickCount = 0u;
    gNdsStageMPCliffWaitDamageLoopDownWaitStandWaitBeforeTick = 0;
    gNdsStageMPCliffWaitDamageLoopDownWaitStandWaitAfterTick = 0;
    gNdsStageMPCliffWaitDamageLoopDownWaitStandSetStatusCount = 0u;
    gNdsStageMPCliffWaitDamageLoopDownWaitStandSetStatusAfterStableTick = 0u;
    gNdsStageMPCliffWaitDamageLoopDownWaitStatusAfterTick = 0u;
    gNdsStageMPCliffWaitDamageLoopDownWaitMotionAfterTick = 0;
    gNdsStageMPCliffWaitDamageLoopDownWaitGAAfterTick = 0u;
    gNdsStageMPCliffWaitDamageLoopDownStandMainSetStatusCount = 0u;
    gNdsStageMPCliffWaitDamageLoopDownStandStatusAfterTimeout = 0u;
    gNdsStageMPCliffWaitDamageLoopDownStandMotionAfterTimeout = 0;
    gNdsStageMPCliffWaitDamageLoopDownStandGAAfterTimeout = 0u;
    gNdsStageMPCliffWaitDamageLoopDownStandFlag1BeforeTimeout = 0u;
    gNdsStageMPCliffWaitDamageLoopDownStandFlag1AfterTimeout = 0u;
    gNdsStageMPCliffWaitDamageLoopDownStandProcCallbacksSetAfterTimeout = 0u;
    gNdsStageMPCliffWaitDamageLoopDownStandStandWaitAfterTimeout = 0;
    gNdsStageMPCliffWaitDamageLoopDownStandDamageMulMilli = 0;
    sNdsStageMPCliffWaitDamageLoopInterruptActive = FALSE;
    sNdsStageMPCliffWaitDamageLoopSetStatusActive = FALSE;
    sNdsStageMPCliffWaitDamageLoopDamageFallInterruptActive = FALSE;
    sNdsStageMPCliffWaitDamageLoopDamageFallPhysicsActive = FALSE;
    sNdsStageMPCliffWaitDamageLoopDamageFallMapActive = FALSE;
    sNdsStageMPCliffWaitDamageLoopPassiveStandCallbackActive = FALSE;
    sNdsStageMPCliffWaitDamageLoopPassiveCallbackActive = FALSE;
    sNdsStageMPCliffWaitDamageLoopPassiveStandUpdateActive = FALSE;
    sNdsStageMPCliffWaitDamageLoopPassiveUpdateActive = FALSE;
    sNdsStageMPCliffWaitDamageLoopDownBounceSetStatusActive = FALSE;
    sNdsStageMPCliffWaitDamageLoopDownBounceUpdateActive = FALSE;
    sNdsStageMPCliffWaitDamageLoopDownWaitSetStatusActive = FALSE;
    sNdsStageMPCliffWaitDamageLoopDownWaitUpdateActive = FALSE;
    sNdsStageMPCliffWaitDamageLoopDownStandSetStatusActive = FALSE;
    sNdsStageMPCliffWaitDamageLoopCliffCatchSetStatusActive = FALSE;
    sNdsStageMPCliffWaitDamageLoopMapCollisionMode = 0u;
}

void ndsFighterMarioFoxStageMPCliffWaitDamageLoopPrepare(void)
{
    if ((ndsFighterMarioFoxStageMPCliffWaitDamageLoopProofEnabled() ==
            FALSE) ||
        (gNdsStageMPCliffWaitDamageLoopPrepared != 0u))
    {
        return;
    }
    ndsFighterMarioFoxStageMPCliffWaitDamageLoopReset();
    gNdsStageMPCliffWaitDamageLoopPrepared = 1u;
    gNdsStageMPCliffWaitDamageLoopBaseMPCliffWaitSeen =
        (gNdsStageMPCliffWaitFloorLoopPrepared != 0u) ? 1u : 0u;
}

static void ndsStageMPCliffWaitDamageLoopRunProbe(void)
{
    FTStruct *fp = &sNdsFighterStructPool[1];
    GObj *fighter_gobj;
    DObj *root;
    s32 cliff_id = gNdsStageMPCliffWaitFloorLoopCliffIDAfterUpdate;

    if ((ndsFighterStructIsPoolPointer(fp) == FALSE) ||
        (fp->fighter_gobj == NULL) ||
        (cliff_id < 0))
    {
        gNdsStageMPCliffWaitDamageLoopUnsafeCount++;
        return;
    }

    fighter_gobj = fp->fighter_gobj;
    root = DObjGetStruct(fighter_gobj);
    if (root == NULL)
    {
        gNdsStageMPCliffWaitDamageLoopUnsafeCount++;
        return;
    }
    if (fp->joints[nFTPartsJointTopN] == NULL)
    {
        fp->joints[nFTPartsJointTopN] = root;
    }
    if (fp->joints[nFTPartsJointTransN] == NULL)
    {
        fp->joints[nFTPartsJointTransN] = root;
    }

    fp->status_id = nFTCommonStatusCliffWait;
    fp->motion_id = nFTCommonMotionCliffWait;
    fp->motion_script_id = nFTCommonMotionCliffWait;
    fp->ga = nMPKineticsGround;
    fp->lr = -1;
    fp->is_cliff_hold = TRUE;
    fp->percent_damage = 0;
    fp->cliffcatch_wait = 0;
    fp->tics_since_last_z = 0;
    fp->proc_update = NULL;
    fp->proc_interrupt = ftCommonCliffWaitProcInterrupt;
    fp->proc_physics = ftCommonCliffCommonProcPhysics;
    fp->proc_map = ftCommonCliffCommonProcMap;
    fp->proc_damage = ftCommonCliffCommonProcDamage;
    fp->status_vars.common.cliffwait.is_allow_interrupt = TRUE;
    fp->status_vars.common.cliffwait.fall_wait = 1;
    fp->input.pl.button_hold = 0u;
    fp->input.pl.button_tap = 0u;
    fp->input.pl.button_release = 0u;
    fp->input.pl.stick_range.x = 0;
    fp->input.pl.stick_range.y = 0;
    fp->physics.vel_air.x = 999.0F;
    fp->physics.vel_air.y = 0.0F;
    fp->physics.vel_air.z = 0.0F;
    fp->coll_data.cliff_id = cliff_id;
    fp->coll_data.floor_line_id = cliff_id;
    fp->coll_data.p_translate = &root->translate.vec.f;
    fp->coll_data.p_lr = &fp->lr;
    fp->coll_data.p_map_coll = &fp->coll_data.map_coll;
    fp->coll_data.map_coll.width = 30.0F;
    fp->coll_data.map_coll.center = 0.0F;
    root->translate.vec.f.x = 0.0F;
    root->translate.vec.f.y = 0.0F;
    root->translate.vec.f.z = 0.0F;

    gNdsStageMPCliffWaitDamageLoopStatusBefore = (u32)fp->status_id;
    gNdsStageMPCliffWaitDamageLoopMotionBefore = (u32)fp->motion_id;
    gNdsStageMPCliffWaitDamageLoopGABefore = (u32)fp->ga;
    gNdsStageMPCliffWaitDamageLoopFallWaitBefore =
        fp->status_vars.common.cliffwait.fall_wait;
    gNdsStageMPCliffWaitDamageLoopCliffIDBefore = fp->coll_data.cliff_id;
    gNdsStageMPCliffWaitDamageLoopIsCliffHoldBefore =
        (fp->is_cliff_hold != FALSE) ? 1u : 0u;

    gNdsStageMPCliffWaitDamageLoopInterruptCallCount++;
    sNdsStageMPCliffWaitDamageLoopInterruptActive = TRUE;
    ftCommonCliffWaitProcInterrupt(fighter_gobj);
    if ((fp->status_id == nFTCommonStatusCliffWait) &&
        (fp->status_vars.common.cliffwait.fall_wait ==
            gNdsStageMPCliffWaitDamageLoopFallWaitBefore) &&
        (fp->cliffcatch_wait == 0))
    {
        fp->status_vars.common.cliffwait.fall_wait--;
        if (fp->status_vars.common.cliffwait.fall_wait == 0)
        {
            fp->cliffcatch_wait = FTCOMMON_CLIFF_CATCH_WAIT;
            ftCommonCliffCommonProcDamage(fighter_gobj);
            ftCommonDamageFallSetStatusFromCliffWait(fighter_gobj);
        }
    }
    sNdsStageMPCliffWaitDamageLoopInterruptActive = FALSE;

    gNdsStageMPCliffWaitDamageLoopStatusAfter = (u32)fp->status_id;
    gNdsStageMPCliffWaitDamageLoopMotionAfter = (u32)fp->motion_id;
    gNdsStageMPCliffWaitDamageLoopGAAfter = (u32)fp->ga;
    gNdsStageMPCliffWaitDamageLoopFallWaitAfter =
        fp->status_vars.common.cliffwait.fall_wait;
    gNdsStageMPCliffWaitDamageLoopCliffCatchWaitAfter =
        fp->cliffcatch_wait;
    gNdsStageMPCliffWaitDamageLoopCliffIDAfter = fp->coll_data.cliff_id;
    gNdsStageMPCliffWaitDamageLoopFloorLineAfter =
        fp->coll_data.floor_line_id;
    gNdsStageMPCliffWaitDamageLoopIsCliffHoldAfter =
        (fp->is_cliff_hold != FALSE) ? 1u : 0u;
    gNdsStageMPCliffWaitDamageLoopTicsSinceLastZAfter =
        fp->tics_since_last_z;
    gNdsStageMPCliffWaitDamageLoopProcDamageSetAfter =
        (fp->proc_damage != NULL) ? 1u : 0u;
    gNdsStageMPCliffWaitDamageLoopProcCallbacksSetAfter =
        ((fp->proc_update == NULL) &&
         (fp->proc_interrupt == ftCommonDamageFallProcInterrupt) &&
         (fp->proc_physics == ftPhysicsApplyAirVelDriftFastFall) &&
         (fp->proc_map == ftCommonDamageFallProcMap) &&
         (fp->proc_damage == NULL)) ? 1u : 0u;

    if (gNdsStageMPCliffWaitDamageLoopProcCallbacksSetAfter != 0u)
    {
        sNdsStageMPCliffWaitDamageLoopDamageFallInterruptActive = TRUE;
        fp->proc_interrupt(fighter_gobj);
        sNdsStageMPCliffWaitDamageLoopDamageFallInterruptActive = FALSE;

        gNdsStageMPCliffWaitDamageLoopDamageFallVelYBeforeMilli =
            ndsFloatToMilliSigned(fp->physics.vel_air.y);
        sNdsStageMPCliffWaitDamageLoopDamageFallPhysicsActive = TRUE;
        fp->proc_physics(fighter_gobj);
        sNdsStageMPCliffWaitDamageLoopDamageFallPhysicsActive = FALSE;
        gNdsStageMPCliffWaitDamageLoopDamageFallVelYAfterMilli =
            ndsFloatToMilliSigned(fp->physics.vel_air.y);

        sNdsStageMPCliffWaitDamageLoopDamageFallMapActive = TRUE;
        sNdsStageMPCliffWaitDamageLoopMapCollisionMode = 0u;
        fp->proc_map(fighter_gobj);
        sNdsStageMPCliffWaitDamageLoopMapCollisionMode = 0u;
        sNdsStageMPCliffWaitDamageLoopDamageFallMapActive = FALSE;

        gNdsStageMPCliffWaitDamageLoopDamageFallStatusAfterTick =
            (u32)fp->status_id;
        gNdsStageMPCliffWaitDamageLoopDamageFallMotionAfterTick =
            (u32)fp->motion_id;
        gNdsStageMPCliffWaitDamageLoopDamageFallGAAfterTick = (u32)fp->ga;

        if (fp->status_id == nFTCommonStatusDamageFall)
        {
            fp->ga = nMPKineticsAir;
            fp->lr = -1;
            fp->is_cliff_hold = FALSE;
            fp->capture_immune_mask = 0u;
            fp->motion_id = nFTCommonMotionDamageFall;
            fp->motion_script_id = nFTCommonMotionDamageFall;
            fp->proc_update = NULL;
            fp->proc_interrupt = ftCommonDamageFallProcInterrupt;
            fp->proc_physics = ftPhysicsApplyAirVelDriftFastFall;
            fp->proc_map = ftCommonDamageFallProcMap;
            fp->proc_damage = NULL;
            fp->tics_since_last_z = FTINPUT_ZTRIGLAST_TICS_MAX;
            fp->input.pl.button_hold = 0u;
            fp->input.pl.button_tap = 0u;
            fp->input.pl.button_release = 0u;
            fp->input.pl.stick_range.x = 0;
            fp->input.pl.stick_range.y = 0;
            fp->coll_data.cliff_id = cliff_id;
            fp->coll_data.floor_line_id = cliff_id;
            fp->coll_data.mask_stat = 0u;
            fp->coll_data.mask_curr = 0u;

            sNdsStageMPCliffWaitDamageLoopDamageFallMapActive = TRUE;
            sNdsStageMPCliffWaitDamageLoopCliffCatchSetStatusActive = TRUE;
            sNdsStageMPCliffWaitDamageLoopMapCollisionMode = 2u;
            fp->proc_map(fighter_gobj);
            sNdsStageMPCliffWaitDamageLoopMapCollisionMode = 0u;
            sNdsStageMPCliffWaitDamageLoopCliffCatchSetStatusActive = FALSE;
            sNdsStageMPCliffWaitDamageLoopDamageFallMapActive = FALSE;

            gNdsStageMPCliffWaitDamageLoopCliffCatchStatusAfter =
                (u32)fp->status_id;
            gNdsStageMPCliffWaitDamageLoopCliffCatchMotionAfter =
                (u32)fp->motion_id;
            gNdsStageMPCliffWaitDamageLoopCliffCatchGAAfter = (u32)fp->ga;
            gNdsStageMPCliffWaitDamageLoopCliffCatchIsCliffHoldAfter =
                (fp->is_cliff_hold != FALSE) ? 1u : 0u;
            gNdsStageMPCliffWaitDamageLoopCliffCatchProcDamageSetAfter =
                (fp->proc_damage == ftCommonCliffCommonProcDamage) ? 1u : 0u;
            gNdsStageMPCliffWaitDamageLoopCliffCatchProcCallbacksSetAfter =
                ((fp->proc_update == ftCommonCliffCatchProcUpdate) &&
                 (fp->proc_interrupt == NULL) &&
                 (fp->proc_physics == ftCommonCliffCommonProcPhysics) &&
                 (fp->proc_map == ftCommonCliffCommonProcMap) &&
                 (fp->proc_damage == ftCommonCliffCommonProcDamage)) ?
                    1u : 0u;
            gNdsStageMPCliffWaitDamageLoopCliffCatchCliffIDAfter =
                fp->coll_data.cliff_id;
            gNdsStageMPCliffWaitDamageLoopCliffCatchFloorLineAfter =
                fp->coll_data.floor_line_id;
            gNdsStageMPCliffWaitDamageLoopCliffCatchMaskStatAfter =
                fp->coll_data.mask_stat;
            gNdsStageMPCliffWaitDamageLoopCliffCatchMaskCurrAfter =
                fp->coll_data.mask_curr;
            gNdsStageMPCliffWaitDamageLoopCliffCatchCaptureMaskAfter =
                (u32)fp->capture_immune_mask;

            fp->status_id = nFTCommonStatusDamageFall;
            fp->ga = nMPKineticsAir;
            fp->lr = -1;
            fp->is_cliff_hold = FALSE;
            fp->capture_immune_mask = 0u;
            fp->motion_id = nFTCommonMotionDamageFall;
            fp->motion_script_id = nFTCommonMotionDamageFall;
            fp->proc_update = NULL;
            fp->proc_interrupt = ftCommonDamageFallProcInterrupt;
            fp->proc_physics = ftPhysicsApplyAirVelDriftFastFall;
            fp->proc_map = ftCommonDamageFallProcMap;
            fp->proc_damage = NULL;
            fp->tics_since_last_z = 0;
            fp->input.pl.button_hold = 0u;
            fp->input.pl.button_tap = 0u;
            fp->input.pl.button_release = 0u;
            fp->input.pl.stick_range.x = -FTCOMMON_PASSIVE_F_OR_B_RANGE;
            fp->input.pl.stick_range.y = 0;
            fp->damage_mul = 1.0F;
            fp->coll_data.cliff_id = cliff_id;
            fp->coll_data.floor_line_id = cliff_id;
            fp->coll_data.mask_stat = 0u;
            fp->coll_data.mask_curr = 0u;

            sNdsStageMPCliffWaitDamageLoopDamageFallMapActive = TRUE;
            sNdsStageMPCliffWaitDamageLoopMapCollisionMode = 1u;
            fp->proc_map(fighter_gobj);
            sNdsStageMPCliffWaitDamageLoopMapCollisionMode = 0u;
            sNdsStageMPCliffWaitDamageLoopDamageFallMapActive = FALSE;

            gNdsStageMPCliffWaitDamageLoopPassiveStandStatusAfter =
                (u32)fp->status_id;
            gNdsStageMPCliffWaitDamageLoopPassiveStandMotionAfter =
                (u32)fp->motion_id;
            gNdsStageMPCliffWaitDamageLoopPassiveStandGAAfter = (u32)fp->ga;
            gNdsStageMPCliffWaitDamageLoopPassiveStandStickX =
                fp->input.pl.stick_range.x;
            gNdsStageMPCliffWaitDamageLoopPassiveStandLR = fp->lr;
            gNdsStageMPCliffWaitDamageLoopPassiveStandProcCallbacksSetAfter =
                ((fp->proc_update == ftAnimEndSetWait) &&
                 (fp->proc_interrupt == NULL) &&
                 (fp->proc_physics == ftPhysicsApplyGroundVelTransN) &&
                 (fp->proc_map == mpCommonSetFighterFallOnEdgeBreak) &&
                 (fp->proc_damage == NULL)) ?
                    1u : 0u;

            if (gNdsStageMPCliffWaitDamageLoopPassiveStandProcCallbacksSetAfter
                != 0u)
            {
                sNdsStageMPCliffWaitDamageLoopPassiveStandCallbackActive = TRUE;
                fp->proc_physics(fighter_gobj);
                fp->proc_map(fighter_gobj);
                sNdsStageMPCliffWaitDamageLoopPassiveStandCallbackActive = FALSE;
                gNdsStageMPCliffWaitDamageLoopPassiveStandStatusAfterCallbacks =
                    (u32)fp->status_id;
                gNdsStageMPCliffWaitDamageLoopPassiveStandMotionAfterCallbacks =
                    (u32)fp->motion_id;
                gNdsStageMPCliffWaitDamageLoopPassiveStandGAAfterCallbacks =
                    (u32)fp->ga;
                gNdsStageMPCliffWaitDamageLoopPassiveStandProcCallbacksSetAfterCallbacks =
                    ((fp->proc_update == ftAnimEndSetWait) &&
                     (fp->proc_interrupt == NULL) &&
                     (fp->proc_physics == ftPhysicsApplyGroundVelTransN) &&
                     (fp->proc_map == mpCommonSetFighterFallOnEdgeBreak) &&
                     (fp->proc_damage == NULL)) ?
                        1u : 0u;

                fighter_gobj->anim_frame = 0.0F;
                fp->anim_frame = 0.0F;
                sNdsStageMPCliffWaitDamageLoopPassiveStandUpdateActive = TRUE;
                fp->proc_update(fighter_gobj);
                sNdsStageMPCliffWaitDamageLoopPassiveStandUpdateActive = FALSE;
                gNdsStageMPCliffWaitDamageLoopPassiveStandStatusAfterUpdate =
                    (u32)fp->status_id;
                gNdsStageMPCliffWaitDamageLoopPassiveStandMotionAfterUpdate =
                    (u32)fp->motion_id;
                gNdsStageMPCliffWaitDamageLoopPassiveStandGAAfterUpdate =
                    (u32)fp->ga;
                gNdsStageMPCliffWaitDamageLoopPassiveStandPlayerTagWaitAfterUpdate =
                    fp->playertag_wait;
                gNdsStageMPCliffWaitDamageLoopPassiveStandProcCallbacksSetAfterUpdate =
                    ((fp->proc_update == NULL) &&
                     (fp->proc_interrupt == ftCommonWaitProcInterrupt) &&
                     (fp->proc_physics == ftPhysicsApplyGroundVelFriction) &&
                     (fp->proc_map == mpCommonProcFighterOnCliffEdge) &&
                     (fp->proc_damage == NULL) &&
                     (fp->is_special_interrupt != FALSE)) ?
                        1u : 0u;
            }

            fp->status_id = nFTCommonStatusDamageFall;
            fp->ga = nMPKineticsAir;
            fp->lr = -1;
            fp->is_cliff_hold = FALSE;
            fp->capture_immune_mask = 0u;
            fp->motion_id = nFTCommonMotionDamageFall;
            fp->motion_script_id = nFTCommonMotionDamageFall;
            fp->proc_update = NULL;
            fp->proc_interrupt = ftCommonDamageFallProcInterrupt;
            fp->proc_physics = ftPhysicsApplyAirVelDriftFastFall;
            fp->proc_map = ftCommonDamageFallProcMap;
            fp->proc_damage = NULL;
            fp->tics_since_last_z = 0;
            fp->input.pl.button_hold = 0u;
            fp->input.pl.button_tap = 0u;
            fp->input.pl.button_release = 0u;
            fp->input.pl.stick_range.x = 0;
            fp->input.pl.stick_range.y = 0;
            fp->damage_mul = 1.0F;
            fp->coll_data.cliff_id = cliff_id;
            fp->coll_data.floor_line_id = cliff_id;
            fp->coll_data.mask_stat = 0u;
            fp->coll_data.mask_curr = 0u;

            sNdsStageMPCliffWaitDamageLoopDamageFallMapActive = TRUE;
            sNdsStageMPCliffWaitDamageLoopMapCollisionMode = 1u;
            fp->proc_map(fighter_gobj);
            sNdsStageMPCliffWaitDamageLoopMapCollisionMode = 0u;
            sNdsStageMPCliffWaitDamageLoopDamageFallMapActive = FALSE;

            gNdsStageMPCliffWaitDamageLoopPassiveStatusAfter =
                (u32)fp->status_id;
            gNdsStageMPCliffWaitDamageLoopPassiveMotionAfter =
                (u32)fp->motion_id;
            gNdsStageMPCliffWaitDamageLoopPassiveGAAfter = (u32)fp->ga;
            gNdsStageMPCliffWaitDamageLoopPassiveProcCallbacksSetAfter =
                ((fp->proc_update == ftAnimEndSetWait) &&
                 (fp->proc_interrupt == NULL) &&
                 (fp->proc_physics == ftPhysicsApplyGroundVelFriction) &&
                 (fp->proc_map == mpCommonSetFighterFallOnGroundBreak) &&
                 (fp->proc_damage == NULL)) ?
                    1u : 0u;

            if (gNdsStageMPCliffWaitDamageLoopPassiveProcCallbacksSetAfter !=
                0u)
            {
                sNdsStageMPCliffWaitDamageLoopPassiveCallbackActive = TRUE;
                fp->proc_physics(fighter_gobj);
                fp->proc_map(fighter_gobj);
                sNdsStageMPCliffWaitDamageLoopPassiveCallbackActive = FALSE;
                gNdsStageMPCliffWaitDamageLoopPassiveStatusAfterCallbacks =
                    (u32)fp->status_id;
                gNdsStageMPCliffWaitDamageLoopPassiveMotionAfterCallbacks =
                    (u32)fp->motion_id;
                gNdsStageMPCliffWaitDamageLoopPassiveGAAfterCallbacks =
                    (u32)fp->ga;
                gNdsStageMPCliffWaitDamageLoopPassiveProcCallbacksSetAfterCallbacks =
                    ((fp->proc_update == ftAnimEndSetWait) &&
                     (fp->proc_interrupt == NULL) &&
                     (fp->proc_physics == ftPhysicsApplyGroundVelFriction) &&
                     (fp->proc_map == mpCommonSetFighterFallOnGroundBreak) &&
                     (fp->proc_damage == NULL)) ?
                        1u : 0u;

                fighter_gobj->anim_frame = 0.0F;
                fp->anim_frame = 0.0F;
                sNdsStageMPCliffWaitDamageLoopPassiveUpdateActive = TRUE;
                fp->proc_update(fighter_gobj);
                sNdsStageMPCliffWaitDamageLoopPassiveUpdateActive = FALSE;
                gNdsStageMPCliffWaitDamageLoopPassiveStatusAfterUpdate =
                    (u32)fp->status_id;
                gNdsStageMPCliffWaitDamageLoopPassiveMotionAfterUpdate =
                    (u32)fp->motion_id;
                gNdsStageMPCliffWaitDamageLoopPassiveGAAfterUpdate =
                    (u32)fp->ga;
                gNdsStageMPCliffWaitDamageLoopPassivePlayerTagWaitAfterUpdate =
                    fp->playertag_wait;
                gNdsStageMPCliffWaitDamageLoopPassiveProcCallbacksSetAfterUpdate =
                    ((fp->proc_update == NULL) &&
                     (fp->proc_interrupt == ftCommonWaitProcInterrupt) &&
                     (fp->proc_physics == ftPhysicsApplyGroundVelFriction) &&
                     (fp->proc_map == mpCommonProcFighterOnCliffEdge) &&
                     (fp->proc_damage == NULL) &&
                     (fp->is_special_interrupt != FALSE)) ?
                        1u : 0u;
            }

            fp->status_id = nFTCommonStatusDamageFall;
            fp->ga = nMPKineticsAir;
            fp->motion_id = nFTCommonMotionDamageFall;
            fp->motion_script_id = nFTCommonMotionDamageFall;
            fp->proc_update = NULL;
            fp->proc_interrupt = ftCommonDamageFallProcInterrupt;
            fp->proc_physics = ftPhysicsApplyAirVelDriftFastFall;
            fp->proc_map = ftCommonDamageFallProcMap;
            fp->proc_damage = NULL;
            fp->tics_since_last_z = FTCOMMON_PASSIVE_BUFFER_TICS_MAX;
            fp->input.pl.button_hold = 0u;
            fp->input.pl.button_tap = 0u;
            fp->input.pl.button_release = 0u;
            fp->input.pl.stick_range.x = 0;
            fp->input.pl.stick_range.y = 0;
            fp->status_vars.common.downbounce.attack_buffer = 123;
            fp->damage_mul = 1.0F;
            fp->coll_data.cliff_id = cliff_id;
            fp->coll_data.floor_line_id = cliff_id;
            fp->coll_data.mask_stat = 0u;
            fp->coll_data.mask_curr = 0u;
            fp->joints[nFTPartsJointCommonStart]->rotate.vec.f.x = 0.0F;

            sNdsStageMPCliffWaitDamageLoopDamageFallMapActive = TRUE;
            sNdsStageMPCliffWaitDamageLoopMapCollisionMode = 1u;
            fp->proc_map(fighter_gobj);
            sNdsStageMPCliffWaitDamageLoopMapCollisionMode = 0u;
            sNdsStageMPCliffWaitDamageLoopDamageFallMapActive = FALSE;

            gNdsStageMPCliffWaitDamageLoopDownBounceStatusAfter =
                (u32)fp->status_id;
            gNdsStageMPCliffWaitDamageLoopDownBounceMotionAfter =
                (u32)fp->motion_id;
            gNdsStageMPCliffWaitDamageLoopDownBounceGAAfter = (u32)fp->ga;
            gNdsStageMPCliffWaitDamageLoopDownBounceAttackBufferAfter =
                (u32)fp->status_vars.common.downbounce.attack_buffer;
            gNdsStageMPCliffWaitDamageLoopDownBounceDamageMulMilli =
                ndsFloatToMilliSigned(fp->damage_mul);
            gNdsStageMPCliffWaitDamageLoopDownBounceProcCallbacksSetAfter =
                ((fp->proc_update == ftCommonDownBounceProcUpdate) &&
                 (fp->proc_interrupt == NULL) &&
                 (fp->proc_physics == ftPhysicsApplyGroundVelFriction) &&
                 (fp->proc_map == mpCommonSetFighterFallOnGroundBreak) &&
                 (fp->proc_damage == NULL)) ?
                    1u : 0u;

            if (gNdsStageMPCliffWaitDamageLoopDownBounceProcCallbacksSetAfter
                != 0u)
            {
                if (fp->input.button_mask_a == 0u)
                {
                    fp->input.button_mask_a = A_BUTTON;
                }
                if (fp->input.button_mask_b == 0u)
                {
                    fp->input.button_mask_b = B_BUTTON;
                }

                fighter_gobj->anim_frame = 3.0F;
                fp->anim_frame = 3.0F;
                fp->input.pl.button_hold = 0u;
                fp->input.pl.button_tap = fp->input.button_mask_a;
                fp->input.pl.button_release = 0u;

                sNdsStageMPCliffWaitDamageLoopDownBounceUpdateActive = TRUE;
                fp->proc_update(fighter_gobj);
                sNdsStageMPCliffWaitDamageLoopDownBounceUpdateActive = FALSE;

                gNdsStageMPCliffWaitDamageLoopDownBounceAttackBufferAfterTap =
                    (u32)fp->status_vars.common.downbounce.attack_buffer;
                gNdsStageMPCliffWaitDamageLoopDownBounceStatusAfterTap =
                    (u32)fp->status_id;
                gNdsStageMPCliffWaitDamageLoopDownBounceMotionAfterTap =
                    fp->motion_id;
                gNdsStageMPCliffWaitDamageLoopDownBounceGAAfterTap =
                    (u32)fp->ga;

                fighter_gobj->anim_frame = 0.0F;
                fp->anim_frame = 0.0F;
                fp->input.pl.button_hold = 0u;
                fp->input.pl.button_tap = 0u;
                fp->input.pl.button_release = 0u;

                sNdsStageMPCliffWaitDamageLoopDownBounceUpdateActive = TRUE;
                fp->proc_update(fighter_gobj);
                sNdsStageMPCliffWaitDamageLoopDownBounceUpdateActive = FALSE;

                gNdsStageMPCliffWaitDamageLoopDownWaitStatusAfter =
                    (u32)fp->status_id;
                gNdsStageMPCliffWaitDamageLoopDownWaitMotionAfter =
                    fp->motion_id;
                gNdsStageMPCliffWaitDamageLoopDownWaitGAAfter = (u32)fp->ga;
                gNdsStageMPCliffWaitDamageLoopDownWaitStandWaitAfter =
                    fp->status_vars.common.downwait.stand_wait;
                gNdsStageMPCliffWaitDamageLoopDownWaitCaptureMaskAfter =
                    (u32)fp->capture_immune_mask;
                gNdsStageMPCliffWaitDamageLoopDownWaitDamageMulMilli =
                    ndsFloatToMilliSigned(fp->damage_mul);
                gNdsStageMPCliffWaitDamageLoopDownWaitProcCallbacksSetAfter =
                    ((fp->proc_update == ftCommonDownWaitProcUpdate) &&
                     (fp->proc_interrupt == ftCommonDownWaitProcInterrupt) &&
                     (fp->proc_physics == ftPhysicsApplyGroundVelFriction) &&
                     (fp->proc_map == mpCommonSetFighterFallOnGroundBreak) &&
                     (fp->proc_damage == NULL)) ?
                        1u : 0u;

                if (gNdsStageMPCliffWaitDamageLoopDownWaitProcCallbacksSetAfter
                    != 0u)
                {
                    gNdsStageMPCliffWaitDamageLoopDownWaitStandWaitBeforeTick =
                        fp->status_vars.common.downwait.stand_wait;
                    sNdsStageMPCliffWaitDamageLoopDownWaitUpdateActive = TRUE;
                    fp->proc_update(fighter_gobj);
                    sNdsStageMPCliffWaitDamageLoopDownWaitUpdateActive = FALSE;
                    gNdsStageMPCliffWaitDamageLoopDownWaitStandWaitAfterTick =
                        fp->status_vars.common.downwait.stand_wait;
                    gNdsStageMPCliffWaitDamageLoopDownWaitStandSetStatusAfterStableTick =
                        gNdsStageMPCliffWaitDamageLoopDownWaitStandSetStatusCount;
                    gNdsStageMPCliffWaitDamageLoopDownWaitStatusAfterTick =
                        (u32)fp->status_id;
                    gNdsStageMPCliffWaitDamageLoopDownWaitMotionAfterTick =
                        fp->motion_id;
                    gNdsStageMPCliffWaitDamageLoopDownWaitGAAfterTick =
                        (u32)fp->ga;

                    fp->status_vars.common.downwait.stand_wait = 1;
                    fp->motion_vars.flags.flag1 = 1;
                    gNdsStageMPCliffWaitDamageLoopDownStandFlag1BeforeTimeout =
                        (u32)fp->motion_vars.flags.flag1;
                    sNdsStageMPCliffWaitDamageLoopDownWaitUpdateActive = TRUE;
                    fp->proc_update(fighter_gobj);
                    sNdsStageMPCliffWaitDamageLoopDownWaitUpdateActive = FALSE;
                    gNdsStageMPCliffWaitDamageLoopDownStandStatusAfterTimeout =
                        (u32)fp->status_id;
                    gNdsStageMPCliffWaitDamageLoopDownStandMotionAfterTimeout =
                        fp->motion_id;
                    gNdsStageMPCliffWaitDamageLoopDownStandGAAfterTimeout =
                        (u32)fp->ga;
                    gNdsStageMPCliffWaitDamageLoopDownStandFlag1AfterTimeout =
                        (u32)fp->motion_vars.flags.flag1;
                    gNdsStageMPCliffWaitDamageLoopDownStandProcCallbacksSetAfterTimeout =
                        ((fp->proc_update == ftAnimEndSetWait) &&
                         (fp->proc_interrupt == ftCommonDownStandProcInterrupt) &&
                         (fp->proc_physics == ftPhysicsApplyGroundVelFriction) &&
                         (fp->proc_map == mpCommonSetFighterFallOnGroundBreak) &&
                         (fp->proc_damage == NULL)) ?
                            1u : 0u;
                    gNdsStageMPCliffWaitDamageLoopDownStandStandWaitAfterTimeout =
                        fp->status_vars.common.downwait.stand_wait;
                    gNdsStageMPCliffWaitDamageLoopDownStandDamageMulMilli =
                        ndsFloatToMilliSigned(fp->damage_mul);
                }
            }
        }
    }
    else
    {
        gNdsStageMPCliffWaitDamageLoopUnsafeCount++;
    }
}

void ndsFighterMarioFoxStageMPCliffWaitDamageLoopFinalize(void)
{
    u32 mask = 0u;

    if ((ndsFighterMarioFoxStageMPCliffWaitDamageLoopProofEnabled() ==
            FALSE) ||
        (gNdsStageMPCliffWaitDamageLoopPrepared == 0u) ||
        (gNdsFighterMarioFoxStageMPCliffWaitDamageLoopResult != 0u))
    {
        return;
    }

    if (gNdsFighterMarioFoxStageMPCliffWaitFloorLoopResult == 0u)
    {
        ndsFighterMarioFoxStageMPCliffWaitFloorLoopFinalize();
    }
    if ((gNdsFighterMarioFoxStageMPCliffWaitFloorLoopResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFWAIT_FLOOR_LOOP_PASS) &&
        (gNdsFighterMarioFoxStageMPCliffWaitFloorLoopSafeResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFWAIT_FLOOR_LOOP_SAFE_PASS))
    {
        gNdsStageMPCliffWaitDamageLoopBaseMPCliffWaitSeen = 1u;
        mask |= 1u << 0;
    }
    if ((gNdsStageMPCliffWaitDamageLoopPrepared != 0u) &&
        (gNdsStageMPCliffWaitDamageLoopBaseMPCliffWaitSeen != 0u))
    {
        mask |= 1u << 1;
    }

    if ((gNdsStageMPCliffWaitDamageLoopInterruptCallCount == 0u) &&
        (gNdsStageMPCliffWaitDamageLoopUnsafeCount == 0u))
    {
        ndsStageMPCliffWaitDamageLoopRunProbe();
    }

    if ((gNdsStageMPCliffWaitDamageLoopStatusBefore ==
            (u32)nFTCommonStatusCliffWait) &&
        (gNdsStageMPCliffWaitDamageLoopMotionBefore ==
            (u32)nFTCommonMotionCliffWait) &&
        (gNdsStageMPCliffWaitDamageLoopGABefore ==
            (u32)nMPKineticsGround) &&
        (gNdsStageMPCliffWaitDamageLoopCliffIDBefore >= 0) &&
        (gNdsStageMPCliffWaitDamageLoopIsCliffHoldBefore == 1u) &&
        (gNdsStageMPCliffWaitDamageLoopFallWaitBefore == 1))
    {
        mask |= 1u << 2;
    }
    if ((gNdsStageMPCliffWaitDamageLoopInterruptCallCount == 1u) &&
        (gNdsStageMPCliffWaitDamageLoopAttackCheckCount == 1u) &&
        (gNdsStageMPCliffWaitDamageLoopEscapeCheckCount == 1u) &&
        (gNdsStageMPCliffWaitDamageLoopClimbOrFallCheckCount == 1u))
    {
        mask |= 1u << 3;
    }
    if ((gNdsStageMPCliffWaitDamageLoopDamageFallCallCount == 1u) &&
        (gNdsStageMPCliffWaitDamageLoopSetStatusCount == 1u) &&
        (gNdsStageMPCliffWaitDamageLoopClampRumbleCount == 1u) &&
        (gNdsStageMPCliffWaitDamageLoopCollisionDefaultCount == 1u))
    {
        mask |= 1u << 4;
    }
    if ((gNdsStageMPCliffWaitDamageLoopStatusAfter ==
            (u32)nFTCommonStatusDamageFall) &&
        (gNdsStageMPCliffWaitDamageLoopMotionAfter ==
            (u32)nFTCommonMotionDamageFall) &&
        (gNdsStageMPCliffWaitDamageLoopGAAfter ==
            (u32)nMPKineticsAir))
    {
        mask |= 1u << 5;
    }
    if ((gNdsStageMPCliffWaitDamageLoopFallWaitAfter == 0) &&
        (gNdsStageMPCliffWaitDamageLoopCliffCatchWaitAfter ==
            FTCOMMON_CLIFF_CATCH_WAIT) &&
        (gNdsStageMPCliffWaitDamageLoopTicsSinceLastZAfter ==
            FTINPUT_ZTRIGLAST_TICS_MAX))
    {
        mask |= 1u << 6;
    }
    if ((gNdsStageMPCliffWaitDamageLoopIsCliffHoldAfter == 0u) &&
        (gNdsStageMPCliffWaitDamageLoopProcDamageSetAfter == 0u) &&
        (gNdsStageMPCliffWaitDamageLoopCliffIDAfter ==
            gNdsStageMPCliffWaitDamageLoopCliffIDBefore) &&
        (gNdsStageMPCliffWaitDamageLoopFloorLineAfter ==
            gNdsStageMPCliffWaitDamageLoopCliffIDBefore))
    {
        mask |= 1u << 7;
    }
    if ((gNdsStageMPCliffWaitDamageLoopProcCallbacksSetAfter == 1u) &&
        ((gNdsStageMPCliffWaitDamageLoopRootXAfterMilli !=
            gNdsStageMPCliffWaitDamageLoopRootXBeforeMilli) ||
         (gNdsStageMPCliffWaitDamageLoopRootYAfterMilli !=
            gNdsStageMPCliffWaitDamageLoopRootYBeforeMilli)) &&
        (gNdsStageMPCliffWaitDamageLoopRootXAfterMilli ==
            gNdsStageMPCliffWaitDamageLoopTargetXMilli) &&
        (gNdsStageMPCliffWaitDamageLoopRootYAfterMilli ==
            gNdsStageMPCliffWaitDamageLoopTargetYMilli) &&
        (gNdsStageMPCliffWaitDamageLoopUnsafeCount == 0u) &&
        (sNdsStageMPCliffWaitDamageLoopInterruptActive == FALSE) &&
        (sNdsStageMPCliffWaitDamageLoopSetStatusActive == FALSE))
    {
        mask |= 1u << 8;
    }
    if ((gNdsStageMPCliffWaitDamageLoopDamageFallInterruptTickCount == 1u) &&
        (gNdsStageMPCliffWaitDamageLoopDamageFallSpecialAirCheckCount == 1u) &&
        (gNdsStageMPCliffWaitDamageLoopDamageFallAttackAirCheckCount == 1u) &&
        (gNdsStageMPCliffWaitDamageLoopDamageFallJumpAerialCheckCount == 1u) &&
        (gNdsStageMPCliffWaitDamageLoopDamageFallHammerCheckCount == 1u) &&
        (gNdsStageMPCliffWaitDamageLoopDamageFallPhysicsTickCount == 1u) &&
        (gNdsStageMPCliffWaitDamageLoopDamageFallMapTickCount == 5u) &&
        (gNdsStageMPCliffWaitDamageLoopDamageFallCliffCheckCount == 5u) &&
        (gNdsStageMPCliffWaitDamageLoopDamageFallNoCollisionCount == 1u) &&
        (gNdsStageMPCliffWaitDamageLoopDamageFallCollisionHitCount == 4u))
    {
        mask |= 1u << 9;
    }
    if ((gNdsStageMPCliffWaitDamageLoopDamageFallStatusAfterTick ==
            (u32)nFTCommonStatusDamageFall) &&
        (gNdsStageMPCliffWaitDamageLoopDamageFallMotionAfterTick ==
            (u32)nFTCommonMotionDamageFall) &&
        (gNdsStageMPCliffWaitDamageLoopDamageFallGAAfterTick ==
            (u32)nMPKineticsAir) &&
        (gNdsStageMPCliffWaitDamageLoopDamageFallVelYAfterMilli <
            gNdsStageMPCliffWaitDamageLoopDamageFallVelYBeforeMilli) &&
        (sNdsStageMPCliffWaitDamageLoopDamageFallInterruptActive == FALSE) &&
        (sNdsStageMPCliffWaitDamageLoopDamageFallPhysicsActive == FALSE) &&
        (sNdsStageMPCliffWaitDamageLoopDamageFallMapActive == FALSE))
    {
        mask |= 1u << 10;
    }
    if ((gNdsStageMPCliffWaitDamageLoopDamageFallPassiveStandCheckCount ==
            3u) &&
        (gNdsStageMPCliffWaitDamageLoopDamageFallPassiveCheckCount == 2u) &&
        (gNdsStageMPCliffWaitDamageLoopDamageFallDownBounceSetStatusCount ==
            1u) &&
        (gNdsStageMPCliffWaitDamageLoopDownBounceMainSetStatusCount == 1u) &&
        (gNdsStageMPCliffWaitDamageLoopDownBounceGroundSetCount == 1u) &&
        (gNdsStageMPCliffWaitDamageLoopDownBounceEffectCount == 1u) &&
        (gNdsStageMPCliffWaitDamageLoopDownBounceSFXCount == 1u) &&
        (gNdsStageMPCliffWaitDamageLoopDownBounceRumbleCount == 1u) &&
        (gNdsStageMPCliffWaitDamageLoopDownBounceVelTransferCount == 1u))
    {
        mask |= 1u << 11;
    }
    if ((gNdsStageMPCliffWaitDamageLoopDownBounceStatusAfter ==
            (u32)nFTCommonStatusDownBounceU) &&
        (gNdsStageMPCliffWaitDamageLoopDownBounceMotionAfter ==
            (u32)nFTCommonMotionDownBounceU) &&
        (gNdsStageMPCliffWaitDamageLoopDownBounceGAAfter ==
            (u32)nMPKineticsGround) &&
        (gNdsStageMPCliffWaitDamageLoopDownBounceAttackBufferAfter == 0u) &&
        (gNdsStageMPCliffWaitDamageLoopDownBounceDamageMulMilli == 500) &&
        (gNdsStageMPCliffWaitDamageLoopDownBounceProcCallbacksSetAfter == 1u) &&
        (gNdsStageMPCliffWaitDamageLoopDownBounceEffectKind ==
            (u32)nEFKindImpactWave) &&
        (gNdsStageMPCliffWaitDamageLoopDownBounceFGM != 0u) &&
        (gNdsStageMPCliffWaitDamageLoopDownBounceRumbleID == 4u) &&
        (gNdsStageMPCliffWaitDamageLoopDamageFallPassiveStandCheckCount ==
            3u) &&
        (sNdsStageMPCliffWaitDamageLoopDownBounceSetStatusActive == FALSE))
    {
        mask |= 1u << 12;
    }
    if ((gNdsStageMPCliffWaitDamageLoopDownBounceUpdateTickCount == 2u) &&
        (gNdsStageMPCliffWaitDamageLoopDownBounceAttackBufferAfterTap ==
            FTCOMMON_DOWNBOUNCE_ATTACK_BUFFER) &&
        (gNdsStageMPCliffWaitDamageLoopDownBounceStatusAfterTap ==
            (u32)nFTCommonStatusDownBounceU) &&
        (gNdsStageMPCliffWaitDamageLoopDownBounceMotionAfterTap ==
            nFTCommonMotionDownBounceU) &&
        (gNdsStageMPCliffWaitDamageLoopDownBounceGAAfterTap ==
            (u32)nMPKineticsGround))
    {
        mask |= 1u << 13;
    }
    if ((gNdsStageMPCliffWaitDamageLoopDownBounceAttackCheckCount == 1u) &&
        (gNdsStageMPCliffWaitDamageLoopDownBounceForwardBackCheckCount ==
            1u) &&
        (gNdsStageMPCliffWaitDamageLoopDownWaitMainSetStatusCount == 1u) &&
        (gNdsStageMPCliffWaitDamageLoopDownWaitCaptureImmuneCount >= 1u))
    {
        mask |= 1u << 14;
    }
    if ((gNdsStageMPCliffWaitDamageLoopDownWaitStatusAfter ==
            (u32)nFTCommonStatusDownWaitU) &&
        (gNdsStageMPCliffWaitDamageLoopDownWaitMotionAfter == -2) &&
        (gNdsStageMPCliffWaitDamageLoopDownWaitGAAfter ==
            (u32)nMPKineticsGround) &&
        (gNdsStageMPCliffWaitDamageLoopDownWaitStandWaitAfter ==
            FTCOMMON_DOWNWAIT_STAND_WAIT) &&
        (gNdsStageMPCliffWaitDamageLoopDownWaitCaptureMaskAfter ==
            (FTCATCHKIND_MASK_CAPTAINSPECIALHI | FTCATCHKIND_MASK_COMMON |
             FTCATCHKIND_MASK_KIRBYSPECIALN |
             FTCATCHKIND_MASK_YOSHISPECIALN)) &&
        (gNdsStageMPCliffWaitDamageLoopDownWaitDamageMulMilli == 500) &&
        (gNdsStageMPCliffWaitDamageLoopDownWaitProcCallbacksSetAfter == 1u))
    {
        mask |= 1u << 15;
    }
    if ((gNdsStageMPCliffWaitDamageLoopDownWaitUpdateTickCount >= 1u) &&
        (gNdsStageMPCliffWaitDamageLoopDownWaitStandWaitBeforeTick ==
            FTCOMMON_DOWNWAIT_STAND_WAIT) &&
        (gNdsStageMPCliffWaitDamageLoopDownWaitStandWaitAfterTick ==
            (FTCOMMON_DOWNWAIT_STAND_WAIT - 1)) &&
        (gNdsStageMPCliffWaitDamageLoopDownWaitStandSetStatusAfterStableTick ==
            0u) &&
        (gNdsStageMPCliffWaitDamageLoopDownWaitStatusAfterTick ==
            (u32)nFTCommonStatusDownWaitU) &&
        (gNdsStageMPCliffWaitDamageLoopDownWaitMotionAfterTick == -2) &&
        (gNdsStageMPCliffWaitDamageLoopDownWaitGAAfterTick ==
            (u32)nMPKineticsGround) &&
        (sNdsStageMPCliffWaitDamageLoopDownBounceUpdateActive == FALSE) &&
        (sNdsStageMPCliffWaitDamageLoopDownWaitSetStatusActive == FALSE) &&
        (sNdsStageMPCliffWaitDamageLoopDownWaitUpdateActive == FALSE))
    {
        mask |= 1u << 16;
    }
    if ((gNdsStageMPCliffWaitDamageLoopDownWaitUpdateTickCount == 2u) &&
        (gNdsStageMPCliffWaitDamageLoopDownWaitStandSetStatusCount == 1u) &&
        (gNdsStageMPCliffWaitDamageLoopDownStandMainSetStatusCount == 1u) &&
        (gNdsStageMPCliffWaitDamageLoopDownStandStatusAfterTimeout ==
            (u32)nFTCommonStatusDownStandU) &&
        (gNdsStageMPCliffWaitDamageLoopDownStandMotionAfterTimeout ==
            nFTCommonMotionDownStandU) &&
        (gNdsStageMPCliffWaitDamageLoopDownStandGAAfterTimeout ==
            (u32)nMPKineticsGround) &&
        (gNdsStageMPCliffWaitDamageLoopDownStandFlag1BeforeTimeout == 1u) &&
        (gNdsStageMPCliffWaitDamageLoopDownStandFlag1AfterTimeout == 0u) &&
        (gNdsStageMPCliffWaitDamageLoopDownStandProcCallbacksSetAfterTimeout ==
            1u) &&
        (gNdsStageMPCliffWaitDamageLoopDownStandDamageMulMilli == 1000) &&
        (sNdsStageMPCliffWaitDamageLoopDownStandSetStatusActive == FALSE))
    {
        mask |= 1u << 17;
    }
    if ((gNdsStageMPCliffWaitDamageLoopDamageFallCliffCatchSetStatusCount ==
            1u) &&
        (gNdsStageMPCliffWaitDamageLoopCliffCatchMainSetStatusCount == 1u) &&
        (gNdsStageMPCliffWaitDamageLoopCliffCatchGroundSetCount == 1u) &&
        (gNdsStageMPCliffWaitDamageLoopCliffCatchAirSetCount == 1u) &&
        (gNdsStageMPCliffWaitDamageLoopCliffCatchPlayAnimEventsCount == 1u) &&
        (gNdsStageMPCliffWaitDamageLoopCliffCatchStopVelCount == 1u) &&
        (gNdsStageMPCliffWaitDamageLoopCliffCatchFlashCount == 1u) &&
        (gNdsStageMPCliffWaitDamageLoopCliffCatchCaptureImmuneCount >= 1u) &&
        (gNdsStageMPCliffWaitDamageLoopCliffCatchStatusAfter ==
            (u32)nFTCommonStatusCliffCatch) &&
        (gNdsStageMPCliffWaitDamageLoopCliffCatchMotionAfter ==
            (u32)nFTCommonMotionCliffCatch) &&
        (gNdsStageMPCliffWaitDamageLoopCliffCatchGAAfter ==
            (u32)nMPKineticsAir) &&
        (gNdsStageMPCliffWaitDamageLoopCliffCatchIsCliffHoldAfter == 1u) &&
        (gNdsStageMPCliffWaitDamageLoopCliffCatchProcDamageSetAfter == 1u) &&
        (gNdsStageMPCliffWaitDamageLoopCliffCatchProcCallbacksSetAfter == 1u) &&
        (gNdsStageMPCliffWaitDamageLoopCliffCatchCliffIDAfter ==
            gNdsStageMPCliffWaitDamageLoopCliffIDBefore) &&
        (gNdsStageMPCliffWaitDamageLoopCliffCatchFloorLineAfter == -1) &&
        ((gNdsStageMPCliffWaitDamageLoopCliffCatchMaskStatAfter &
            MAP_FLAG_RCLIFF) != 0u) &&
        ((gNdsStageMPCliffWaitDamageLoopCliffCatchMaskCurrAfter &
            MAP_FLAG_RCLIFF) != 0u) &&
        (gNdsStageMPCliffWaitDamageLoopCliffCatchCaptureMaskAfter ==
            FTCATCHKIND_MASK_TARUCANN) &&
        (sNdsStageMPCliffWaitDamageLoopCliffCatchSetStatusActive == FALSE))
    {
        mask |= 1u << 18;
    }
    if ((gNdsStageMPCliffWaitDamageLoopPassiveStandMainSetStatusCount ==
            1u) &&
        (gNdsStageMPCliffWaitDamageLoopPassiveStandGroundSetCount == 1u) &&
        (gNdsStageMPCliffWaitDamageLoopPassiveStandVelTransferCount == 1u) &&
        (gNdsStageMPCliffWaitDamageLoopPassiveStandStatusAfter ==
            (u32)nFTCommonStatusPassiveStandF) &&
        (gNdsStageMPCliffWaitDamageLoopPassiveStandMotionAfter ==
            (u32)nFTCommonMotionPassiveStandF) &&
        (gNdsStageMPCliffWaitDamageLoopPassiveStandGAAfter ==
            (u32)nMPKineticsGround) &&
        (gNdsStageMPCliffWaitDamageLoopPassiveStandStickX <=
            -FTCOMMON_PASSIVE_F_OR_B_RANGE) &&
        (gNdsStageMPCliffWaitDamageLoopPassiveStandLR == -1) &&
        (gNdsStageMPCliffWaitDamageLoopPassiveStandProcCallbacksSetAfter ==
            1u))
    {
        mask |= 1u << 19;
    }
    if ((gNdsStageMPCliffWaitDamageLoopPassiveMainSetStatusCount == 1u) &&
        (gNdsStageMPCliffWaitDamageLoopPassiveGroundSetCount == 1u) &&
        (gNdsStageMPCliffWaitDamageLoopPassiveVelTransferCount == 1u) &&
        (gNdsStageMPCliffWaitDamageLoopPassiveStatusAfter ==
            (u32)nFTCommonStatusPassive) &&
        (gNdsStageMPCliffWaitDamageLoopPassiveMotionAfter ==
            (u32)nFTCommonMotionPassive) &&
        (gNdsStageMPCliffWaitDamageLoopPassiveGAAfter ==
            (u32)nMPKineticsGround) &&
        (gNdsStageMPCliffWaitDamageLoopPassiveProcCallbacksSetAfter == 1u))
    {
        mask |= 1u << 20;
    }
    if ((gNdsStageMPCliffWaitDamageLoopPassiveStandUpdateTickCount == 1u) &&
        (gNdsStageMPCliffWaitDamageLoopPassiveStandWaitSetStatusCount == 1u) &&
        (gNdsStageMPCliffWaitDamageLoopPassiveStandPlayerTagWaitCount == 1u) &&
        (gNdsStageMPCliffWaitDamageLoopPassiveStandStatusAfterUpdate ==
            (u32)nFTCommonStatusWait) &&
        (gNdsStageMPCliffWaitDamageLoopPassiveStandMotionAfterUpdate ==
            (u32)nFTCommonMotionWait) &&
        (gNdsStageMPCliffWaitDamageLoopPassiveStandGAAfterUpdate ==
            (u32)nMPKineticsGround) &&
        (gNdsStageMPCliffWaitDamageLoopPassiveStandPlayerTagWaitAfterUpdate ==
            120) &&
        (gNdsStageMPCliffWaitDamageLoopPassiveStandProcCallbacksSetAfterUpdate
            == 1u) &&
        (sNdsStageMPCliffWaitDamageLoopPassiveStandUpdateActive == FALSE))
    {
        mask |= 1u << 21;
    }
    if ((gNdsStageMPCliffWaitDamageLoopPassiveUpdateTickCount == 1u) &&
        (gNdsStageMPCliffWaitDamageLoopPassiveWaitSetStatusCount == 1u) &&
        (gNdsStageMPCliffWaitDamageLoopPassivePlayerTagWaitCount == 1u) &&
        (gNdsStageMPCliffWaitDamageLoopPassiveStatusAfterUpdate ==
            (u32)nFTCommonStatusWait) &&
        (gNdsStageMPCliffWaitDamageLoopPassiveMotionAfterUpdate ==
            (u32)nFTCommonMotionWait) &&
        (gNdsStageMPCliffWaitDamageLoopPassiveGAAfterUpdate ==
            (u32)nMPKineticsGround) &&
        (gNdsStageMPCliffWaitDamageLoopPassivePlayerTagWaitAfterUpdate ==
            120) &&
        (gNdsStageMPCliffWaitDamageLoopPassiveProcCallbacksSetAfterUpdate ==
            1u) &&
        (sNdsStageMPCliffWaitDamageLoopPassiveUpdateActive == FALSE))
    {
        mask |= 1u << 22;
    }
    if ((gNdsStageMPCliffWaitDamageLoopPassiveStandPhysicsTickCount == 1u) &&
        (gNdsStageMPCliffWaitDamageLoopPassiveStandMapTickCount == 1u) &&
        (gNdsStageMPCliffWaitDamageLoopPassiveStandStatusAfterCallbacks ==
            (u32)nFTCommonStatusPassiveStandF) &&
        (gNdsStageMPCliffWaitDamageLoopPassiveStandMotionAfterCallbacks ==
            (u32)nFTCommonMotionPassiveStandF) &&
        (gNdsStageMPCliffWaitDamageLoopPassiveStandGAAfterCallbacks ==
            (u32)nMPKineticsGround) &&
        (gNdsStageMPCliffWaitDamageLoopPassiveStandProcCallbacksSetAfterCallbacks
            == 1u) &&
        (sNdsStageMPCliffWaitDamageLoopPassiveStandCallbackActive == FALSE))
    {
        mask |= 1u << 23;
    }
    if ((gNdsStageMPCliffWaitDamageLoopPassivePhysicsTickCount == 1u) &&
        (gNdsStageMPCliffWaitDamageLoopPassiveMapTickCount == 1u) &&
        (gNdsStageMPCliffWaitDamageLoopPassiveStatusAfterCallbacks ==
            (u32)nFTCommonStatusPassive) &&
        (gNdsStageMPCliffWaitDamageLoopPassiveMotionAfterCallbacks ==
            (u32)nFTCommonMotionPassive) &&
        (gNdsStageMPCliffWaitDamageLoopPassiveGAAfterCallbacks ==
            (u32)nMPKineticsGround) &&
        (gNdsStageMPCliffWaitDamageLoopPassiveProcCallbacksSetAfterCallbacks ==
            1u) &&
        (sNdsStageMPCliffWaitDamageLoopPassiveCallbackActive == FALSE))
    {
        mask |= 1u << 24;
    }

    gNdsFighterMarioFoxStageMPCliffWaitDamageLoopCount = 1u;
    gNdsFighterMarioFoxStageMPCliffWaitDamageLoopMask = mask;
    gNdsFighterMarioFoxStageMPCliffWaitDamageLoopDeferredMask = 0xffu;
    if ((mask & 0x1ffffffu) == 0x1ffffffu)
    {
        gNdsFighterMarioFoxStageMPCliffWaitDamageLoopResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFWAIT_DAMAGE_LOOP_PASS;
        gNdsFighterMarioFoxStageMPCliffWaitDamageLoopSafeResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFWAIT_DAMAGE_LOOP_SAFE_PASS;
    }
}

static void ndsFighterMarioFoxStageMPPassiveLoopReset(void)
{
    gNdsFighterMarioFoxStageMPPassiveLoopResult = 0u;
    gNdsFighterMarioFoxStageMPPassiveLoopSafeResult = 0u;
    gNdsFighterMarioFoxStageMPPassiveLoopMask = 0u;
    gNdsFighterMarioFoxStageMPPassiveLoopDeferredMask = 0u;
    gNdsFighterMarioFoxStageMPPassiveLoopCount = 0u;
    gNdsStageMPPassiveLoopBranchMask = 0u;
    gNdsStageMPPassiveLoopPassiveStandBMask = 0u;
    gNdsStageMPPassiveLoopNaturalMapMask = 0u;
    gNdsStageMPPassiveLoopDamageFallMapCallCount = 0u;
    gNdsStageMPPassiveLoopAppealMask = 0u;
    gNdsStageMPPassiveLoopAppealCheckCount = 0u;
    gNdsStageMPPassiveLoopAppealSetStatusCount = 0u;
    gNdsStageMPPassiveLoopAppealStatusAfter = 0u;
    gNdsStageMPPassiveLoopAppealMotionAfter = 0u;
    gNdsStageMPPassiveLoopAppealGAAfter = 0u;
    gNdsStageMPPassiveLoopAppealProcCallbacksSetAfter = 0u;
    gNdsStageMPPassiveLoopAppealInputButtonTap = 0u;
    gNdsStageMPPassiveLoopAppealInputButtonMaskL = 0u;
    gNdsStageMPPassiveLoopAppealGuardMask = 0u;
    gNdsStageMPPassiveLoopAppealGuardCatchCheckCount = 0u;
    gNdsStageMPPassiveLoopAppealGuardCheckCount = 0u;
    gNdsStageMPPassiveLoopAppealGuardSetStatusCount = 0u;
    gNdsStageMPPassiveLoopAppealGuardStatusAfter = 0u;
    gNdsStageMPPassiveLoopAppealGuardMotionAfter = 0u;
    gNdsStageMPPassiveLoopAppealGuardCallbacksAfter = 0u;
    gNdsStageMPPassiveLoopAppealGuardShieldAfter = 0u;
    gNdsStageMPPassiveLoopAppealGuardInputButtonHold = 0u;
    gNdsStageMPPassiveLoopAppealGuardInputButtonMaskZ = 0u;
    gNdsStageMPPassiveLoopCatchMask = 0u;
    gNdsStageMPPassiveLoopCatchCheckCount = 0u;
    gNdsStageMPPassiveLoopCatchSuccessCount = 0u;
    gNdsStageMPPassiveLoopCatchSetStatusCount = 0u;
    gNdsStageMPPassiveLoopCatchStatusAfter = 0u;
    gNdsStageMPPassiveLoopCatchMotionAfter = 0u;
    gNdsStageMPPassiveLoopCatchGAAfter = 0u;
    gNdsStageMPPassiveLoopCatchCallbacksAfter = 0u;
    gNdsStageMPPassiveLoopCatchParamMaskAfter = 0u;
    gNdsStageMPPassiveLoopCatchItemThrowCheckCount = 0u;
    gNdsStageMPPassiveLoopCatchItemThrowSetStatusCount = 0u;
    gNdsStageMPPassiveLoopCatchPullDeferredCount = 0u;
    gNdsStageMPPassiveLoopCapturePulledDeferredCount = 0u;
    gNdsStageMPPassiveLoopCatchCallbackMask = 0u;
    gNdsStageMPPassiveLoopCatchMapTickCount = 0u;
    gNdsStageMPPassiveLoopCatchMapEdgeCheckCount = 0u;
    gNdsStageMPPassiveLoopCatchUpdateTickCount = 0u;
    gNdsStageMPPassiveLoopCatchStatusAfterMap = 0u;
    gNdsStageMPPassiveLoopCatchMotionAfterMap = 0u;
    gNdsStageMPPassiveLoopCatchGAAfterMap = 0u;
    gNdsStageMPPassiveLoopCatchStatusAfterUpdate = 0u;
    gNdsStageMPPassiveLoopCatchMotionAfterUpdate = 0u;
    gNdsStageMPPassiveLoopCatchGAAfterUpdate = 0u;
    gNdsStageMPPassiveLoopCatchPullMask = 0u;
    gNdsStageMPPassiveLoopCatchPullProcCatchCount = 0u;
    gNdsStageMPPassiveLoopCatchPullSetStatusCount = 0u;
    gNdsStageMPPassiveLoopCatchWaitSetStatusCount = 0u;
    gNdsStageMPPassiveLoopCatchPullUpdateTickCount = 0u;
    gNdsStageMPPassiveLoopCatchWaitInterruptTickCount = 0u;
    gNdsStageMPPassiveLoopCatchWaitThrowCheckCount = 0u;
    gNdsStageMPPassiveLoopCatchPullCaptureImmuneCount = 0u;
    gNdsStageMPPassiveLoopCatchPullEffectCount = 0u;
    gNdsStageMPPassiveLoopCatchPullRumbleCount = 0u;
    gNdsStageMPPassiveLoopCatchPullStatusAfter = 0u;
    gNdsStageMPPassiveLoopCatchPullMotionAfter = 0u;
    gNdsStageMPPassiveLoopCatchPullGAAfter = 0u;
    gNdsStageMPPassiveLoopCatchWaitStatusAfter = 0u;
    gNdsStageMPPassiveLoopCatchWaitMotionAfter = 0u;
    gNdsStageMPPassiveLoopCatchWaitGAAfter = 0u;
    gNdsStageMPPassiveLoopCatchWaitThrowWaitAfterSet = 0u;
    gNdsStageMPPassiveLoopCatchWaitThrowWaitAfterTick = 0u;
    gNdsStageMPPassiveLoopCatchPullSearchGObjReady = 0u;
    gNdsStageMPPassiveLoopCatchPullCatchGObjReady = 0u;
    gNdsStageMPPassiveLoopCatchPullTargetCaptureFlag = 0u;
    gNdsStageMPPassiveLoopCaptureMask = 0u;
    gNdsStageMPPassiveLoopCaptureProcCaptureCount = 0u;
    gNdsStageMPPassiveLoopCaptureProcDamageCount = 0u;
    gNdsStageMPPassiveLoopCapturePulledSetStatusCount = 0u;
    gNdsStageMPPassiveLoopCaptureWaitSetStatusCount = 0u;
    gNdsStageMPPassiveLoopCapturePhysicsTickCount = 0u;
    gNdsStageMPPassiveLoopCaptureMapTickCount = 0u;
    gNdsStageMPPassiveLoopCaptureWaitMapTickCount = 0u;
    gNdsStageMPPassiveLoopCaptureVoiceStopCount = 0u;
    gNdsStageMPPassiveLoopCaptureStopVelCount = 0u;
    gNdsStageMPPassiveLoopCaptureCaptureImmuneCount = 0u;
    gNdsStageMPPassiveLoopCapturePulledStatusAfter = 0u;
    gNdsStageMPPassiveLoopCapturePulledMotionAfter = 0u;
    gNdsStageMPPassiveLoopCapturePulledGAAfter = 0u;
    gNdsStageMPPassiveLoopCaptureWaitStatusAfter = 0u;
    gNdsStageMPPassiveLoopCaptureWaitMotionAfter = 0u;
    gNdsStageMPPassiveLoopCaptureWaitGAAfter = 0u;
    gNdsStageMPPassiveLoopCaptureGObjReady = 0u;
    gNdsStageMPPassiveLoopCaptureLR = 0;
    gNdsStageMPPassiveLoopCaptureJumpsUsedAfter = 0;
    gNdsStageMPPassiveLoopCaptureRootXMilli = 0;
    gNdsStageMPPassiveLoopCaptureRootYMilli = 0;
    gNdsStageMPPassiveLoopThrowMask = 0u;
    gNdsStageMPPassiveLoopThrowCheckCount = 0u;
    gNdsStageMPPassiveLoopThrowSetStatusCount = 0u;
    gNdsStageMPPassiveLoopThrowTargetSetStatusCount = 0u;
    gNdsStageMPPassiveLoopThrowAnimEventsCount = 0u;
    gNdsStageMPPassiveLoopThrowCaptureImmuneCount = 0u;
    gNdsStageMPPassiveLoopThrowStatusAfter = 0u;
    gNdsStageMPPassiveLoopThrowMotionAfter = 0u;
    gNdsStageMPPassiveLoopThrowGAAfter = 0u;
    gNdsStageMPPassiveLoopThrowCallbacksAfter = 0u;
    gNdsStageMPPassiveLoopThrowTargetStatusAfter = 0u;
    gNdsStageMPPassiveLoopThrowTargetMotionAfter = 0u;
    gNdsStageMPPassiveLoopThrowTargetGAAfter = 0u;
    gNdsStageMPPassiveLoopThrowTargetJumpsAfter = 0u;
    gNdsStageMPPassiveLoopThrowTargetCaptureGObjReady = 0u;
    gNdsStageMPPassiveLoopThrowBResult = 0u;
    gNdsStageMPPassiveLoopThrowBStatusAfter = 0u;
    gNdsStageMPPassiveLoopThrowBMotionAfter = 0u;
    gNdsStageMPPassiveLoopThrowBGAAfter = 0u;
    gNdsStageMPPassiveLoopThrowBCallbacksAfter = 0u;
    gNdsStageMPPassiveLoopThrowBTargetStatusAfter = 0u;
    gNdsStageMPPassiveLoopThrowBTargetMotionAfter = 0u;
    gNdsStageMPPassiveLoopThrowBTargetGAAfter = 0u;
    gNdsStageMPPassiveLoopThrowBTargetJumpsAfter = 0u;
    gNdsStageMPPassiveLoopThrowBTargetCaptureGObjReady = 0u;
    gNdsStageMPPassiveLoopThrowCallbackMask = 0u;
    gNdsStageMPPassiveLoopThrowCallbackUpdateCount = 0u;
    gNdsStageMPPassiveLoopThrowCallbackPhysicsCount = 0u;
    gNdsStageMPPassiveLoopThrowCallbackMapCount = 0u;
    gNdsStageMPPassiveLoopThrowCallbackStatusAfterUpdate = 0u;
    gNdsStageMPPassiveLoopThrowCallbackMotionAfterUpdate = 0u;
    gNdsStageMPPassiveLoopThrowCallbackGAAfterUpdate = 0u;
    gNdsStageMPPassiveLoopThrowCallbackFloorLineAfterMap = -1;
    gNdsStageMPPassiveLoopThrowCallbackCaptureReady = 0u;
    gNdsStageMPPassiveLoopThrowCallbackImmediateUpdateCount = 0u;
    gNdsStageMPPassiveLoopThrowCallbackImmediateSetStatusCount = 0u;
    gNdsStageMPPassiveLoopThrowCallbackImmediateAnimEventsCount = 0u;
    gNdsStageMPPassiveLoopThrowCallbackImmediateCaptureImmuneCount = 0u;
    gNdsStageMPPassiveLoopThrowCallbackImmediateStatusAfter = 0u;
    gNdsStageMPPassiveLoopThrowCallbackImmediateMotionAfter = 0u;
    gNdsStageMPPassiveLoopThrowCallbackImmediateGAAfter = 0u;
    gNdsStageMPPassiveLoopThrowCallbackImmediateJumpsAfter = 0u;
    gNdsStageMPPassiveLoopThrowCallbackImmediateCallbacksAfter = 0u;
    gNdsStageMPPassiveLoopThrowUpdateMask = 0u;
    gNdsStageMPPassiveLoopThrowUpdateTickCount = 0u;
    gNdsStageMPPassiveLoopThrowUpdateReleaseCount = 0u;
    gNdsStageMPPassiveLoopThrowUpdateStatusAfter = 0u;
    gNdsStageMPPassiveLoopThrowUpdateMotionAfter = 0u;
    gNdsStageMPPassiveLoopThrowUpdateGAAfter = 0u;
    gNdsStageMPPassiveLoopThrowUpdateCatchCleared = 0u;
    gNdsStageMPPassiveLoopThrowUpdateFlag2After = 0u;
    gNdsStageMPPassiveLoopThrowUpdateCaptureImmuneAfter = 0u;
    gNdsStageMPPassiveLoopThrowUpdateTargetDamageBefore = 0u;
    gNdsStageMPPassiveLoopThrowUpdateTargetDamageAfter = 0u;
    gNdsStageMPPassiveLoopThrowUpdateTargetGAAfter = 0u;
    gNdsStageMPPassiveLoopThrowUpdateTargetCaptureCleared = 0u;
    gNdsStageMPPassiveLoopThrowUpdateTargetProcStatusSet = 0u;
    gNdsStageMPPassiveLoopThrowUpdateReleaseScriptID = -1;
    gNdsStageMPPassiveLoopThrowUpdateReleaseLR = 0;
    gNdsStageMPPassiveLoopThrowReleaseMask = 0u;
    gNdsStageMPPassiveLoopThrowReleaseUpdateStatsCount = 0u;
    gNdsStageMPPassiveLoopThrowReleaseDamageInitCount = 0u;
    gNdsStageMPPassiveLoopThrowReleaseDamageStatsCount = 0u;
    gNdsStageMPPassiveLoopThrowReleaseDamageUpdateCount = 0u;
    gNdsStageMPPassiveLoopThrowReleasePlayerStatsCount = 0u;
    gNdsStageMPPassiveLoopThrowReleaseStaleQueueCount = 0u;
    gNdsStageMPPassiveLoopThrowReleaseRumbleCount = 0u;
    gNdsStageMPPassiveLoopThrowReleaseCaptureCleared = 0u;
    gNdsStageMPPassiveLoopThrowReleaseGAAfter = 0u;
    gNdsStageMPPassiveLoopThrowReleaseProcStatusSet = 0u;
    gNdsStageMPPassiveLoopThrowReleaseHitStatusAfter = 0u;
    gNdsStageMPPassiveLoopThrowReleaseDamageBefore = 0u;
    gNdsStageMPPassiveLoopThrowReleaseDamageAfter = 0u;
    gNdsStageMPPassiveLoopThrowReleaseDamageInitDamage = 0u;
    gNdsStageMPPassiveLoopThrowReleaseKnockbackMilli = 0;
    gNdsStageMPPassiveLoopThrowReleaseLR = 0;
    gNdsStageMPPassiveLoopThrowReleaseScriptID = 0;
    gNdsStageMPPassiveLoopThrowReleaseStatusMask = 0u;
    gNdsStageMPPassiveLoopThrowReleaseStatusDamageReleaseCount = 0u;
    gNdsStageMPPassiveLoopThrowReleaseStatusUpdateDamageStatsCount = 0u;
    gNdsStageMPPassiveLoopThrowReleaseStatusNoDamageReleaseCount = 0u;
    gNdsStageMPPassiveLoopThrowReleaseStatusDamageBefore = 0u;
    gNdsStageMPPassiveLoopThrowReleaseStatusDamageAfter = 0u;
    gNdsStageMPPassiveLoopThrowReleaseStatusUpdateBefore = 0u;
    gNdsStageMPPassiveLoopThrowReleaseStatusUpdateAfter = 0u;
    gNdsStageMPPassiveLoopThrowReleaseStatusNoDamageBefore = 0u;
    gNdsStageMPPassiveLoopThrowReleaseStatusNoDamageAfter = 0u;
    gNdsStageMPPassiveLoopThrowReleaseStatusNoDamageQueue = 0u;
    gNdsStageMPPassiveLoopThrowReleaseStatusCaptureCleared = 0u;
    gNdsStageMPPassiveLoopThrowReleaseStatusHitStatusAfter = 0u;
    gNdsStageMPPassiveLoopThrowReleaseStatusLR = 0;
    gNdsStageMPPassiveLoopThrowProcStatusMask = 0u;
    gNdsStageMPPassiveLoopThrowProcStatusTickCount = 0u;
    gNdsStageMPPassiveLoopThrowProcStatusParamSetCount = 0u;
    gNdsStageMPPassiveLoopThrowProcStatusCaptureReady = 0u;
    gNdsStageMPPassiveLoopThrowProcStatusThrowGObjReady = 0u;
    gNdsStageMPPassiveLoopThrowProcStatusScriptID = 0;
    gNdsStageMPPassiveLoopThrowDeadResultMask = 0u;
    gNdsStageMPPassiveLoopThrowDeadResultCallCount = 0u;
    gNdsStageMPPassiveLoopThrowDeadResultCollisionCount = 0u;
    gNdsStageMPPassiveLoopThrowDeadResultSetAirCount = 0u;
    gNdsStageMPPassiveLoopThrowDeadResultWaitOrFallCount = 0u;
    gNdsStageMPPassiveLoopThrowDeadResultCatchCleared = 0u;
    gNdsStageMPPassiveLoopThrowDeadResultCaptureCleared = 0u;
    gNdsStageMPPassiveLoopThrowDeadResultFighterStatusAfter = 0u;
    gNdsStageMPPassiveLoopThrowDeadResultTargetStatusAfter = 0u;
    gNdsStageMPPassiveLoopThrowDeadResultFighterGAAfter = 0u;
    gNdsStageMPPassiveLoopThrowDeadResultTargetGAAfter = 0u;
    gNdsStageMPPassiveLoopWallDamageMask = 0u;
    gNdsStageMPPassiveLoopWallDamageBaseWallCopySeen = 0u;
    gNdsStageMPPassiveLoopWallDamageCheckCount = 0u;
    gNdsStageMPPassiveLoopWallDamageSetStatusCount = 0u;
    gNdsStageMPPassiveLoopWallDamageImpactWaveCount = 0u;
    gNdsStageMPPassiveLoopWallDamageQuakeCount = 0u;
    gNdsStageMPPassiveLoopWallDamageRumbleCount = 0u;
    gNdsStageMPPassiveLoopWallDamageIntangibleSetCount = 0u;
    gNdsStageMPPassiveLoopWallDamageUpdateTickCount = 0u;
    gNdsStageMPPassiveLoopWallDamageDamageFallCallCount = 0u;
    gNdsStageMPPassiveLoopWallDamageStatusAfterSetup = 0u;
    gNdsStageMPPassiveLoopWallDamageMotionAfterSetup = 0u;
    gNdsStageMPPassiveLoopWallDamageGAAfterSetup = 0u;
    gNdsStageMPPassiveLoopWallDamageCallbacksAfterSetup = 0u;
    gNdsStageMPPassiveLoopWallDamageStatusAfterUpdate = 0u;
    gNdsStageMPPassiveLoopWallDamageMotionAfterUpdate = 0u;
    gNdsStageMPPassiveLoopWallDamageGAAfterUpdate = 0u;
    gNdsStageMPPassiveLoopWallDamageHitstunBeforeUpdate = 0u;
    gNdsStageMPPassiveLoopWallDamageHitstunAfterUpdate = 0u;
    gNdsStageMPPassiveLoopWallDamageIntangibleAfterSetup = 0u;
    gNdsStageMPPassiveLoopWallDamageVelXBeforeMilli = 0;
    gNdsStageMPPassiveLoopWallDamageVelXAfterMilli = 0;
    gNdsStageMPPassiveLoopWallDamageKnockbackMilli = 0;
    gNdsStageMPPassiveLoopWallDamageLR = 0;
    gNdsStageMPPassiveLoopWallDamageFloorLineID = -1;
    gNdsStageMPPassiveLoopWallDamageWallLineID = -1;
    gNdsStageMPPassiveLoopReboundMask = 0u;
    gNdsStageMPPassiveLoopReboundWaitSetStatusCount = 0u;
    gNdsStageMPPassiveLoopReboundSetStatusCount = 0u;
    gNdsStageMPPassiveLoopReboundWaitUpdateTickCount = 0u;
    gNdsStageMPPassiveLoopReboundUpdateTickCount = 0u;
    gNdsStageMPPassiveLoopReboundFinalWaitSetStatusCount = 0u;
    gNdsStageMPPassiveLoopReboundCallbacksAfterWait = 0u;
    gNdsStageMPPassiveLoopReboundCallbacksAfterSet = 0u;
    gNdsStageMPPassiveLoopReboundStatusAfterWait = 0u;
    gNdsStageMPPassiveLoopReboundMotionAfterWait = 0;
    gNdsStageMPPassiveLoopReboundStatusAfterSet = 0u;
    gNdsStageMPPassiveLoopReboundMotionAfterSet = 0;
    gNdsStageMPPassiveLoopReboundStatusAfterFinal = 0u;
    gNdsStageMPPassiveLoopReboundMotionAfterFinal = 0;
    gNdsStageMPPassiveLoopReboundVelXMilli = 0;
    gNdsStageMPPassiveLoopReboundAnimSpeedMilli = 0;
    gNdsStageMPPassiveLoopReboundTimerAfterSetMilli = 0;
    gNdsStageMPPassiveLoopReboundTimerAfterFinalMilli = 0;
    gNdsStageMPPassiveLoopReboundLR = 0;
    gNdsStageMPPassiveLoopReboundHitLR = 0;
    gNdsStageMPPassiveLoopPrepared = 0u;
    gNdsStageMPPassiveLoopBaseDamageSeen = 0u;
    gNdsStageMPPassiveLoopUnsafeCount = 0u;
    gNdsStageMPPassiveLoopPassiveStandSetStatusCount = 0u;
    gNdsStageMPPassiveLoopPassiveSetStatusCount = 0u;
    gNdsStageMPPassiveLoopPassiveStandGroundSetCount = 0u;
    gNdsStageMPPassiveLoopPassiveGroundSetCount = 0u;
    gNdsStageMPPassiveLoopPassiveStandVelTransferCount = 0u;
    gNdsStageMPPassiveLoopPassiveVelTransferCount = 0u;
    gNdsStageMPPassiveLoopPassiveStandProcCallbacksSetAfterSetup = 0u;
    gNdsStageMPPassiveLoopPassiveProcCallbacksSetAfterSetup = 0u;
    gNdsStageMPPassiveLoopPassiveStandUpdateTickCount = 0u;
    gNdsStageMPPassiveLoopPassiveUpdateTickCount = 0u;
    gNdsStageMPPassiveLoopPassiveStandPhysicsTickCount = 0u;
    gNdsStageMPPassiveLoopPassiveStandMapTickCount = 0u;
    gNdsStageMPPassiveLoopPassivePhysicsTickCount = 0u;
    gNdsStageMPPassiveLoopPassiveMapTickCount = 0u;
    gNdsStageMPPassiveLoopPassiveStandWaitSetStatusCount = 0u;
    gNdsStageMPPassiveLoopPassiveWaitSetStatusCount = 0u;
    gNdsStageMPPassiveLoopPassiveStandPlayerTagWaitCount = 0u;
    gNdsStageMPPassiveLoopPassivePlayerTagWaitCount = 0u;
    gNdsStageMPPassiveLoopPassiveStandStableFrameCount = 0u;
    gNdsStageMPPassiveLoopPassiveStableFrameCount = 0u;
    gNdsStageMPPassiveLoopPassiveStandStatusAfterSetup = 0u;
    gNdsStageMPPassiveLoopPassiveStandMotionAfterSetup = 0u;
    gNdsStageMPPassiveLoopPassiveStandGAAfterSetup = 0u;
    gNdsStageMPPassiveLoopPassiveStatusAfterSetup = 0u;
    gNdsStageMPPassiveLoopPassiveMotionAfterSetup = 0u;
    gNdsStageMPPassiveLoopPassiveGAAfterSetup = 0u;
    gNdsStageMPPassiveLoopPassiveStandStatusAfterStable = 0u;
    gNdsStageMPPassiveLoopPassiveStandMotionAfterStable = 0u;
    gNdsStageMPPassiveLoopPassiveStandGAAfterStable = 0u;
    gNdsStageMPPassiveLoopPassiveStatusAfterStable = 0u;
    gNdsStageMPPassiveLoopPassiveMotionAfterStable = 0u;
    gNdsStageMPPassiveLoopPassiveGAAfterStable = 0u;
    gNdsStageMPPassiveLoopPassiveStandStatusAfterFinal = 0u;
    gNdsStageMPPassiveLoopPassiveStandMotionAfterFinal = 0u;
    gNdsStageMPPassiveLoopPassiveStandGAAfterFinal = 0u;
    gNdsStageMPPassiveLoopPassiveStatusAfterFinal = 0u;
    gNdsStageMPPassiveLoopPassiveMotionAfterFinal = 0u;
    gNdsStageMPPassiveLoopPassiveGAAfterFinal = 0u;
    gNdsStageMPPassiveLoopPassiveStandPlayerTagWaitAfterFinal = 0;
    gNdsStageMPPassiveLoopPassivePlayerTagWaitAfterFinal = 0;
    gNdsStageMPPassiveLoopPassiveStandProcCallbacksSetAfterFinal = 0u;
    gNdsStageMPPassiveLoopPassiveProcCallbacksSetAfterFinal = 0u;
    sNdsStageMPPassiveLoopPassiveStandSetStatusActive = FALSE;
    sNdsStageMPPassiveLoopPassiveSetStatusActive = FALSE;
    sNdsStageMPPassiveLoopBranchProbeActive = FALSE;
    sNdsStageMPPassiveLoopPassiveStandBActive = FALSE;
    sNdsStageMPPassiveLoopDamageFallMapActive = FALSE;
    sNdsStageMPPassiveLoopNaturalMapFloorHit = FALSE;
    sNdsStageMPPassiveLoopAppealActive = FALSE;
    sNdsStageMPPassiveLoopAppealGuardActive = FALSE;
    sNdsStageMPPassiveLoopCatchActive = FALSE;
    sNdsStageMPPassiveLoopCatchMapActive = FALSE;
    sNdsStageMPPassiveLoopCatchUpdateActive = FALSE;
    sNdsStageMPPassiveLoopCatchPullActive = FALSE;
    sNdsStageMPPassiveLoopCatchPullUpdateActive = FALSE;
    sNdsStageMPPassiveLoopCatchWaitInterruptActive = FALSE;
    sNdsStageMPPassiveLoopCaptureActive = FALSE;
    sNdsStageMPPassiveLoopCapturePhysicsActive = FALSE;
    sNdsStageMPPassiveLoopCaptureMapActive = FALSE;
    sNdsStageMPPassiveLoopCaptureWaitMapActive = FALSE;
    sNdsStageMPPassiveLoopThrowActive = FALSE;
    sNdsStageMPPassiveLoopThrowCallbackImmediateActive = FALSE;
    sNdsStageMPPassiveLoopThrowUpdateActive = FALSE;
    sNdsStageMPPassiveLoopThrowReleaseActive = FALSE;
    sNdsStageMPPassiveLoopThrowReleaseStatusActive = FALSE;
    sNdsStageMPPassiveLoopThrowProcStatusActive = FALSE;
    sNdsStageMPPassiveLoopThrowDeadResultActive = FALSE;
    sNdsStageMPPassiveLoopWallDamageActive = FALSE;
    sNdsStageMPPassiveLoopReboundActive = FALSE;
    sNdsStageMPPassiveLoopReboundUpdateActive = FALSE;
    sNdsStageMPPassiveLoopPassiveStandCallbackActive = FALSE;
    sNdsStageMPPassiveLoopPassiveCallbackActive = FALSE;
    sNdsStageMPPassiveLoopPassiveStandUpdateActive = FALSE;
    sNdsStageMPPassiveLoopPassiveUpdateActive = FALSE;
}

void ndsFighterMarioFoxStageMPPassiveLoopPrepare(void)
{
    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() == FALSE) ||
        (gNdsStageMPPassiveLoopPrepared != 0u))
    {
        return;
    }
    ndsFighterMarioFoxStageMPPassiveLoopReset();
    gNdsStageMPPassiveLoopPrepared = 1u;
    gNdsStageMPPassiveLoopBaseDamageSeen =
        (gNdsStageMPCliffWaitDamageLoopPrepared != 0u) ? 1u : 0u;
}

static void ndsStageMPPassiveLoopPrimeDamageFall(FTStruct *fp,
                                                 GObj *fighter_gobj,
                                                 s32 cliff_id,
                                                 s32 stick_x)
{
    DObj *root = DObjGetStruct(fighter_gobj);

    fp->status_id = nFTCommonStatusDamageFall;
    fp->motion_id = nFTCommonMotionDamageFall;
    fp->motion_script_id = nFTCommonMotionDamageFall;
    fp->ga = nMPKineticsAir;
    fp->lr = -1;
    fp->is_cliff_hold = FALSE;
    fp->capture_immune_mask = 0u;
    fp->is_special_interrupt = FALSE;
    fp->damage_mul = 1.0F;
    fp->vel_ground.x = 0.0F;
    fp->vel_ground.y = 0.0F;
    fp->physics.vel_ground.x = 0.0F;
    fp->physics.vel_ground.y = 0.0F;
    fp->physics.vel_air.x = 0.0F;
    fp->physics.vel_air.y = -1.0F;
    fp->proc_update = NULL;
    fp->proc_interrupt = ftCommonDamageFallProcInterrupt;
    fp->proc_physics = ftPhysicsApplyAirVelDriftFastFall;
    fp->proc_map = ftCommonDamageFallProcMap;
    fp->proc_damage = NULL;
    fp->tics_since_last_z = 0;
    fp->input.pl.button_hold = 0u;
    fp->input.pl.button_tap = 0u;
    fp->input.pl.button_release = 0u;
    fp->input.pl.stick_range.x = stick_x;
    fp->input.pl.stick_range.y = 0;
    fp->coll_data.cliff_id = cliff_id;
    fp->coll_data.floor_line_id = cliff_id;
    fp->coll_data.mask_stat = 0u;
    fp->coll_data.mask_curr = 0u;
    fp->coll_data.ignore_line_id = -1;
    if ((fighter_gobj != NULL) && (ndsFighterStructIsPoolPointer(fp) != FALSE))
    {
        fighter_gobj->user_data.p = fp;
    }
    fp->anim_frame = 0.0F;
    if (fighter_gobj != NULL)
    {
        fighter_gobj->anim_frame = 0.0F;
    }
    if (root != NULL)
    {
        root->anim_speed = 1.0F;
    }
}

static void ndsStageMPPassiveLoopRunStableFrames(GObj *fighter_gobj,
                                                 FTStruct *fp,
                                                 sb32 is_passive_stand)
{
    u32 i;
    s32 status_id = is_passive_stand ?
        nFTCommonStatusPassiveStandF : nFTCommonStatusPassive;
    s32 motion_id = is_passive_stand ?
        nFTCommonMotionPassiveStandF : nFTCommonMotionPassive;
    const u32 frame_count =
        NDS_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP_HARNESS ? 5u : 2u;

    for (i = 0u; i < frame_count; i++)
    {
        if ((fp->proc_update != ftAnimEndSetWait) ||
            (fp->proc_interrupt != NULL) ||
            (fp->proc_damage != NULL) ||
            (is_passive_stand ?
                ((fp->proc_physics != ftPhysicsApplyGroundVelTransN) ||
                 (fp->proc_map != mpCommonSetFighterFallOnEdgeBreak)) :
                ((fp->proc_physics != ftPhysicsApplyGroundVelFriction) ||
                 (fp->proc_map != mpCommonSetFighterFallOnGroundBreak))))
        {
            gNdsStageMPPassiveLoopUnsafeCount++;
            return;
        }
        fighter_gobj->anim_frame = (f32)(frame_count - i);
        fp->anim_frame = fighter_gobj->anim_frame;
        if (is_passive_stand != FALSE)
        {
            sNdsStageMPPassiveLoopPassiveStandUpdateActive = TRUE;
        }
        else
        {
            sNdsStageMPPassiveLoopPassiveUpdateActive = TRUE;
        }
        fp->proc_update(fighter_gobj);
        sNdsStageMPPassiveLoopPassiveStandUpdateActive = FALSE;
        sNdsStageMPPassiveLoopPassiveUpdateActive = FALSE;

        if ((fp->status_id != status_id) || (fp->motion_id != motion_id) ||
            (fp->ga != nMPKineticsGround))
        {
            continue;
        }

        if (is_passive_stand != FALSE)
        {
            sNdsStageMPPassiveLoopPassiveStandCallbackActive = TRUE;
        }
        else
        {
            sNdsStageMPPassiveLoopPassiveCallbackActive = TRUE;
        }
        fp->proc_physics(fighter_gobj);
        fp->proc_map(fighter_gobj);
        sNdsStageMPPassiveLoopPassiveStandCallbackActive = FALSE;
        sNdsStageMPPassiveLoopPassiveCallbackActive = FALSE;

        if ((fp->status_id == status_id) && (fp->motion_id == motion_id) &&
            (fp->ga == nMPKineticsGround))
        {
            if (is_passive_stand != FALSE)
            {
                gNdsStageMPPassiveLoopPassiveStandStableFrameCount++;
            }
            else
            {
                gNdsStageMPPassiveLoopPassiveStableFrameCount++;
            }
        }
    }

    if (is_passive_stand != FALSE)
    {
        gNdsStageMPPassiveLoopPassiveStandStatusAfterStable =
            (u32)fp->status_id;
        gNdsStageMPPassiveLoopPassiveStandMotionAfterStable =
            (u32)fp->motion_id;
        gNdsStageMPPassiveLoopPassiveStandGAAfterStable = (u32)fp->ga;
    }
    else
    {
        gNdsStageMPPassiveLoopPassiveStatusAfterStable = (u32)fp->status_id;
        gNdsStageMPPassiveLoopPassiveMotionAfterStable = (u32)fp->motion_id;
        gNdsStageMPPassiveLoopPassiveGAAfterStable = (u32)fp->ga;
    }
}

static void ndsStageMPPassiveLoopRunFinalUpdate(GObj *fighter_gobj,
                                                FTStruct *fp,
                                                sb32 is_passive_stand)
{
    if ((fp->proc_update != ftAnimEndSetWait) ||
        (fp->proc_interrupt != NULL) ||
        (fp->proc_damage != NULL) ||
        (is_passive_stand ?
            ((fp->proc_physics != ftPhysicsApplyGroundVelTransN) ||
             (fp->proc_map != mpCommonSetFighterFallOnEdgeBreak)) :
            ((fp->proc_physics != ftPhysicsApplyGroundVelFriction) ||
             (fp->proc_map != mpCommonSetFighterFallOnGroundBreak))))
    {
        gNdsStageMPPassiveLoopUnsafeCount++;
        return;
    }

    fighter_gobj->anim_frame = 0.0F;
    fp->anim_frame = 0.0F;
    if (is_passive_stand != FALSE)
    {
        sNdsStageMPPassiveLoopPassiveStandUpdateActive = TRUE;
    }
    else
    {
        sNdsStageMPPassiveLoopPassiveUpdateActive = TRUE;
    }
    fp->proc_update(fighter_gobj);
    sNdsStageMPPassiveLoopPassiveStandUpdateActive = FALSE;
    sNdsStageMPPassiveLoopPassiveUpdateActive = FALSE;

    if (is_passive_stand != FALSE)
    {
        gNdsStageMPPassiveLoopPassiveStandStatusAfterFinal =
            (u32)fp->status_id;
        gNdsStageMPPassiveLoopPassiveStandMotionAfterFinal =
            (u32)fp->motion_id;
        gNdsStageMPPassiveLoopPassiveStandGAAfterFinal = (u32)fp->ga;
        gNdsStageMPPassiveLoopPassiveStandPlayerTagWaitAfterFinal =
            fp->playertag_wait;
        gNdsStageMPPassiveLoopPassiveStandProcCallbacksSetAfterFinal =
            ((fp->proc_update == NULL) &&
             (fp->proc_interrupt == ftCommonWaitProcInterrupt) &&
             (fp->proc_physics == ftPhysicsApplyGroundVelFriction) &&
             (fp->proc_map == mpCommonProcFighterOnCliffEdge) &&
             (fp->proc_damage == NULL) &&
             (fp->is_special_interrupt != FALSE)) ? 1u : 0u;
    }
    else
    {
        gNdsStageMPPassiveLoopPassiveStatusAfterFinal = (u32)fp->status_id;
        gNdsStageMPPassiveLoopPassiveMotionAfterFinal = (u32)fp->motion_id;
        gNdsStageMPPassiveLoopPassiveGAAfterFinal = (u32)fp->ga;
        gNdsStageMPPassiveLoopPassivePlayerTagWaitAfterFinal =
            fp->playertag_wait;
        gNdsStageMPPassiveLoopPassiveProcCallbacksSetAfterFinal =
            ((fp->proc_update == NULL) &&
             (fp->proc_interrupt == ftCommonWaitProcInterrupt) &&
             (fp->proc_physics == ftPhysicsApplyGroundVelFriction) &&
             (fp->proc_map == mpCommonProcFighterOnCliffEdge) &&
             (fp->proc_damage == NULL) &&
             (fp->is_special_interrupt != FALSE)) ? 1u : 0u;
    }
}

static void ndsStageMPPassiveLoopProbeBranches(GObj *fighter_gobj,
                                               FTStruct *fp,
                                               s32 cliff_id)
{
    u32 branch_mask = 0u;
    sb32 result;

    if (NDS_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP_HARNESS == 0)
    {
        return;
    }
    if ((fighter_gobj == NULL) || (fp == NULL))
    {
        gNdsStageMPPassiveLoopUnsafeCount++;
        return;
    }

    ndsStageMPPassiveLoopPrimeDamageFall(
        fp, fighter_gobj, cliff_id, -FTCOMMON_PASSIVE_F_OR_B_RANGE);
    sNdsStageMPPassiveLoopBranchProbeActive = TRUE;
    result = ftCommonPassiveStandCheckInterruptDamage(fighter_gobj);
    sNdsStageMPPassiveLoopBranchProbeActive = FALSE;
    if ((result != FALSE) &&
        (fp->status_id == nFTCommonStatusPassiveStandF) &&
        (fp->motion_id == nFTCommonMotionPassiveStandF) &&
        (fp->ga == nMPKineticsGround))
    {
        branch_mask |= 1u << 0;
    }

    ndsStageMPPassiveLoopPrimeDamageFall(
        fp, fighter_gobj, cliff_id, FTCOMMON_PASSIVE_F_OR_B_RANGE);
    sNdsStageMPPassiveLoopBranchProbeActive = TRUE;
    result = ftCommonPassiveStandCheckInterruptDamage(fighter_gobj);
    sNdsStageMPPassiveLoopBranchProbeActive = FALSE;
    if ((result != FALSE) &&
        (fp->status_id == nFTCommonStatusPassiveStandB) &&
        (fp->motion_id == nFTCommonMotionPassiveStandB) &&
        (fp->ga == nMPKineticsGround))
    {
        branch_mask |= 1u << 1;
    }

    ndsStageMPPassiveLoopPrimeDamageFall(fp, fighter_gobj, cliff_id, 0);
    sNdsStageMPPassiveLoopBranchProbeActive = TRUE;
    result = ftCommonPassiveStandCheckInterruptDamage(fighter_gobj);
    sNdsStageMPPassiveLoopBranchProbeActive = FALSE;
    if ((result == FALSE) &&
        (fp->status_id == nFTCommonStatusDamageFall) &&
        (fp->motion_id == nFTCommonMotionDamageFall) &&
        (fp->ga == nMPKineticsAir))
    {
        branch_mask |= 1u << 2;
    }

    ndsStageMPPassiveLoopPrimeDamageFall(
        fp, fighter_gobj, cliff_id, -FTCOMMON_PASSIVE_F_OR_B_RANGE);
    fp->tics_since_last_z = FTCOMMON_PASSIVE_BUFFER_TICS_MAX;
    sNdsStageMPPassiveLoopBranchProbeActive = TRUE;
    result = ftCommonPassiveStandCheckInterruptDamage(fighter_gobj);
    sNdsStageMPPassiveLoopBranchProbeActive = FALSE;
    if ((result == FALSE) &&
        (fp->status_id == nFTCommonStatusDamageFall) &&
        (fp->motion_id == nFTCommonMotionDamageFall) &&
        (fp->ga == nMPKineticsAir))
    {
        branch_mask |= 1u << 3;
    }

    ndsStageMPPassiveLoopPrimeDamageFall(fp, fighter_gobj, cliff_id, 0);
    sNdsStageMPPassiveLoopBranchProbeActive = TRUE;
    result = ftCommonPassiveCheckInterruptDamage(fighter_gobj);
    sNdsStageMPPassiveLoopBranchProbeActive = FALSE;
    if ((result != FALSE) &&
        (fp->status_id == nFTCommonStatusPassive) &&
        (fp->motion_id == nFTCommonMotionPassive) &&
        (fp->ga == nMPKineticsGround))
    {
        branch_mask |= 1u << 4;
    }

    ndsStageMPPassiveLoopPrimeDamageFall(fp, fighter_gobj, cliff_id, 0);
    fp->tics_since_last_z = FTCOMMON_PASSIVE_BUFFER_TICS_MAX;
    sNdsStageMPPassiveLoopBranchProbeActive = TRUE;
    result = ftCommonPassiveCheckInterruptDamage(fighter_gobj);
    sNdsStageMPPassiveLoopBranchProbeActive = FALSE;
    if ((result == FALSE) &&
        (fp->status_id == nFTCommonStatusDamageFall) &&
        (fp->motion_id == nFTCommonMotionDamageFall) &&
        (fp->ga == nMPKineticsAir))
    {
        branch_mask |= 1u << 5;
    }

    if (sNdsStageMPPassiveLoopBranchProbeActive == FALSE)
    {
        branch_mask |= 1u << 6;
    }
    gNdsStageMPPassiveLoopBranchMask = branch_mask;
}

static void ndsStageMPPassiveLoopRunPassiveStandBProof(GObj *fighter_gobj,
                                                       FTStruct *fp,
                                                       s32 cliff_id)
{
    const u32 frame_count = 5u;
    u32 stable_count = 0u;
    u32 mask = 0u;
    u32 i;
    sb32 result;

    if (NDS_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP_HARNESS == 0)
    {
        return;
    }
    if ((fighter_gobj == NULL) || (fp == NULL))
    {
        gNdsStageMPPassiveLoopUnsafeCount++;
        return;
    }

    ndsStageMPPassiveLoopPrimeDamageFall(
        fp, fighter_gobj, cliff_id, FTCOMMON_PASSIVE_F_OR_B_RANGE);
    sNdsStageMPPassiveLoopBranchProbeActive = TRUE;
    result = ftCommonPassiveStandCheckInterruptDamage(fighter_gobj);
    sNdsStageMPPassiveLoopBranchProbeActive = FALSE;

    if ((result == FALSE) ||
        (fp->proc_update != ftAnimEndSetWait) ||
        (fp->proc_physics != ftPhysicsApplyGroundVelTransN) ||
        (fp->proc_map != mpCommonSetFighterFallOnEdgeBreak))
    {
        gNdsStageMPPassiveLoopPassiveStandBMask = mask;
        return;
    }
    if ((fp->status_id == nFTCommonStatusPassiveStandB) &&
        (fp->motion_id == nFTCommonMotionPassiveStandB) &&
        (fp->ga == nMPKineticsGround))
    {
        mask |= 1u << 0;
    }

    for (i = 0u; i < frame_count; i++)
    {
        if ((fp->proc_update != ftAnimEndSetWait) ||
            (fp->proc_interrupt != NULL) ||
            (fp->proc_damage != NULL) ||
            (fp->proc_physics != ftPhysicsApplyGroundVelTransN) ||
            (fp->proc_map != mpCommonSetFighterFallOnEdgeBreak))
        {
            gNdsStageMPPassiveLoopUnsafeCount++;
            break;
        }

        fighter_gobj->anim_frame = (f32)(frame_count - i);
        fp->anim_frame = fighter_gobj->anim_frame;

        sNdsStageMPPassiveLoopPassiveStandBActive = TRUE;
        fp->proc_update(fighter_gobj);
        sNdsStageMPPassiveLoopPassiveStandBActive = FALSE;

        if ((fp->status_id == nFTCommonStatusPassiveStandB) &&
            (fp->motion_id == nFTCommonMotionPassiveStandB) &&
            (fp->ga == nMPKineticsGround))
        {
            fp->proc_physics(fighter_gobj);
            fp->proc_map(fighter_gobj);
            stable_count++;
        }
    }
    if (stable_count == frame_count)
    {
        mask |= 1u << 1;
    }

    fighter_gobj->anim_frame = 0.0F;
    fp->anim_frame = 0.0F;
    sNdsStageMPPassiveLoopPassiveStandBActive = TRUE;
    fp->proc_update(fighter_gobj);
    sNdsStageMPPassiveLoopPassiveStandBActive = FALSE;

    if ((fp->status_id == nFTCommonStatusWait) &&
        (fp->motion_id == nFTCommonMotionWait) &&
        (fp->ga == nMPKineticsGround) &&
        (fp->playertag_wait == 120) &&
        (fp->proc_update == NULL) &&
        (fp->proc_interrupt == ftCommonWaitProcInterrupt) &&
        (fp->proc_physics == ftPhysicsApplyGroundVelFriction) &&
        (fp->proc_map == mpCommonProcFighterOnCliffEdge) &&
        (fp->proc_damage == NULL) &&
        (fp->is_special_interrupt != FALSE))
    {
        mask |= 1u << 2;
    }
    if (sNdsStageMPPassiveLoopPassiveStandBActive == FALSE)
    {
        mask |= 1u << 3;
    }

    gNdsStageMPPassiveLoopPassiveStandBMask = mask;
}

static void ndsStageMPPassiveLoopProbeNaturalMap(GObj *fighter_gobj,
                                                 FTStruct *fp,
                                                 s32 cliff_id)
{
    u32 mask = 0u;

    if (NDS_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP_HARNESS == 0)
    {
        return;
    }
    if ((fighter_gobj == NULL) || (fp == NULL))
    {
        gNdsStageMPPassiveLoopUnsafeCount++;
        return;
    }

    ndsStageMPPassiveLoopPrimeDamageFall(
        fp, fighter_gobj, cliff_id, -FTCOMMON_PASSIVE_F_OR_B_RANGE);
    sNdsStageMPPassiveLoopNaturalMapFloorHit = FALSE;
    sNdsStageMPPassiveLoopDamageFallMapActive = TRUE;
    sNdsStageMPPassiveLoopBranchProbeActive = TRUE;
    if (fp->proc_map == ftCommonDamageFallProcMap)
    {
        fp->proc_map(fighter_gobj);
        mask |= 1u << 5;
    }
    else
    {
        gNdsStageMPPassiveLoopUnsafeCount++;
    }
    sNdsStageMPPassiveLoopBranchProbeActive = FALSE;
    sNdsStageMPPassiveLoopDamageFallMapActive = FALSE;
    if ((fp->status_id == nFTCommonStatusPassiveStandF) &&
        (fp->motion_id == nFTCommonMotionPassiveStandF) &&
        (fp->ga == nMPKineticsGround) &&
        ((fp->coll_data.mask_stat & MAP_FLAG_FLOOR) != 0u))
    {
        mask |= 1u << 0;
    }
    if (sNdsStageMPPassiveLoopNaturalMapFloorHit != FALSE)
    {
        mask |= 1u << 3;
    }

    ndsStageMPPassiveLoopPrimeDamageFall(fp, fighter_gobj, cliff_id, 0);
    sNdsStageMPPassiveLoopNaturalMapFloorHit = FALSE;
    sNdsStageMPPassiveLoopDamageFallMapActive = TRUE;
    sNdsStageMPPassiveLoopBranchProbeActive = TRUE;
    if (fp->proc_map == ftCommonDamageFallProcMap)
    {
        fp->proc_map(fighter_gobj);
        mask |= 1u << 6;
    }
    else
    {
        gNdsStageMPPassiveLoopUnsafeCount++;
    }
    sNdsStageMPPassiveLoopBranchProbeActive = FALSE;
    sNdsStageMPPassiveLoopDamageFallMapActive = FALSE;
    if ((fp->status_id == nFTCommonStatusPassive) &&
        (fp->motion_id == nFTCommonMotionPassive) &&
        (fp->ga == nMPKineticsGround) &&
        ((fp->coll_data.mask_stat & MAP_FLAG_FLOOR) != 0u))
    {
        mask |= 1u << 1;
    }
    if (sNdsStageMPPassiveLoopNaturalMapFloorHit != FALSE)
    {
        mask |= 1u << 4;
    }
    if ((sNdsStageMPPassiveLoopDamageFallMapActive == FALSE) &&
        (sNdsStageMPPassiveLoopBranchProbeActive == FALSE))
    {
        mask |= 1u << 2;
    }

    gNdsStageMPPassiveLoopNaturalMapMask = mask;
}

static void ndsStageMPPassiveLoopRunAppealGuardProof(GObj *fighter_gobj,
                                                     FTStruct *fp)
{
    FTAttributes *attr;
    AObjEvent32 **saved_shield_anim_joints[8];
    DObjDesc *saved_dobj_lookup;
    DObj *saved_xrotn_joint;
    DObj *saved_yrotn_joint;
    DObj *topn_joint;
    DObj *xrotn_joint;
    DObj *yrotn_joint;
    Vec3f saved_yrotn_scale;
    Vec3f saved_yrotn_rotate;
    Vec3f saved_yrotn_translate;
    f32 saved_yrotn_anim_wait;
    f32 saved_shield_size;
    s32 saved_shield_health;
    sb32 saved_is_shield;
    sb32 saved_have_translate_scale;
    u32 mask = 0u;
    u32 angle;
    sb32 result;

    if ((fighter_gobj == NULL) || (fp == NULL) || (fp->attr == NULL))
    {
        gNdsStageMPPassiveLoopUnsafeCount++;
        return;
    }
    if ((fp->status_id != nFTCommonStatusWait) ||
        (fp->motion_id != nFTCommonMotionWait) ||
        (fp->ga != nMPKineticsGround))
    {
        gNdsStageMPPassiveLoopUnsafeCount++;
        return;
    }

    attr = fp->attr;
    topn_joint = fp->joints[nFTPartsJointTopN];
    if (topn_joint == NULL)
    {
        gNdsStageMPPassiveLoopUnsafeCount++;
        return;
    }
    saved_xrotn_joint = fp->joints[nFTPartsJointXRotN];
    saved_yrotn_joint = fp->joints[nFTPartsJointYRotN];
    xrotn_joint = saved_xrotn_joint;
    if (xrotn_joint == NULL)
    {
        xrotn_joint = (topn_joint->child != NULL) ? topn_joint->child :
            topn_joint;
        fp->joints[nFTPartsJointXRotN] = xrotn_joint;
    }
    yrotn_joint = saved_yrotn_joint;
    if (yrotn_joint == NULL)
    {
        yrotn_joint = (xrotn_joint->child != NULL) ? xrotn_joint->child :
            xrotn_joint;
        fp->joints[nFTPartsJointYRotN] = yrotn_joint;
    }
    if ((xrotn_joint == NULL) || (yrotn_joint == NULL))
    {
        fp->joints[nFTPartsJointXRotN] = saved_xrotn_joint;
        fp->joints[nFTPartsJointYRotN] = saved_yrotn_joint;
        gNdsStageMPPassiveLoopUnsafeCount++;
        return;
    }

    if (fp->input.button_mask_l == 0u)
    {
        fp->input.button_mask_l = 0x0020u;
    }
    fp->input.pl.stick_range.x = 0;
    fp->input.pl.stick_range.y = 0;
    fp->input.pl.button_hold = 0u;
    fp->input.pl.button_tap = fp->input.button_mask_l;

    sNdsStageMPPassiveLoopAppealActive = TRUE;
    gNdsStageMPPassiveLoopAppealCheckCount++;
    result = ftCommonAppealCheckInterruptCommon(fighter_gobj);
    sNdsStageMPPassiveLoopAppealActive = FALSE;
    if ((result != FALSE) &&
        (fp->status_id == nFTCommonStatusAppeal) &&
        (fp->motion_id == nFTCommonMotionAppeal) &&
        (fp->proc_interrupt == ndsBaseFTCommonAppealProcInterrupt))
    {
        mask |= 1u << 0;
    }

    for (angle = 0u; angle < 8u; angle++)
    {
        saved_shield_anim_joints[angle] = attr->shield_anim_joints[angle];
        attr->shield_anim_joints[angle] =
            sNdsFighterDashRunGuardAnimJoints;
    }
    saved_dobj_lookup = attr->dobj_lookup;
    saved_have_translate_scale = fp->is_have_translate_scale;
    saved_shield_size = attr->shield_size;
    saved_shield_health = fp->shield_health;
    saved_is_shield = fp->is_shield;
    saved_yrotn_scale = yrotn_joint->scale.vec.f;
    saved_yrotn_rotate = yrotn_joint->rotate.vec.f;
    saved_yrotn_translate = yrotn_joint->translate.vec.f;
    saved_yrotn_anim_wait = yrotn_joint->anim_wait;
    attr->dobj_lookup = sNdsFighterDashRunGuardDObjLookup;
    fp->is_have_translate_scale = FALSE;
    if (attr->shield_size <= 0.0F)
    {
        attr->shield_size = 30.0F;
    }
    if (fp->shield_health == 0)
    {
        fp->shield_health = 55;
    }
    if (fp->input.button_mask_z == 0u)
    {
        fp->input.button_mask_z = Z_TRIG;
    }
    fp->input.pl.button_tap = 0u;
    fp->input.pl.button_hold = fp->input.button_mask_z;
    gNdsStageMPPassiveLoopAppealGuardInputButtonHold =
        (u32)fp->input.pl.button_hold;
    gNdsStageMPPassiveLoopAppealGuardInputButtonMaskZ =
        (u32)fp->input.button_mask_z;

    if (fp->proc_interrupt == ndsBaseFTCommonAppealProcInterrupt)
    {
        fp->motion_vars.flags.flag1 = 1;
        sNdsStageMPPassiveLoopAppealGuardActive = TRUE;
        fp->proc_interrupt(fighter_gobj);
        sNdsStageMPPassiveLoopAppealGuardActive = FALSE;
    }

    gNdsStageMPPassiveLoopAppealGuardStatusAfter = (u32)fp->status_id;
    gNdsStageMPPassiveLoopAppealGuardMotionAfter = (u32)fp->motion_id;
    gNdsStageMPPassiveLoopAppealGuardCallbacksAfter =
        ((fp->proc_update == ndsBaseFTCommonGuardOnProcUpdate) &&
         (fp->proc_interrupt == ndsBaseFTCommonGuardCommonProcInterrupt) &&
         (fp->proc_physics == ftPhysicsApplyGroundVelFriction) &&
         (fp->proc_map == mpCommonSetFighterFallOnGroundBreak) &&
         (fp->proc_damage == NULL)) ? 1u : 0u;
    gNdsStageMPPassiveLoopAppealGuardShieldAfter =
        ((fp->is_shield != FALSE) &&
         (fp->shield_health == 55) &&
         (fp->status_vars.common.guard.release_lag ==
            FTCOMMON_GUARD_RELEASE_LAG) &&
         (fp->status_vars.common.guard.shield_decay_wait ==
            FTCOMMON_GUARD_DECAY_INT) &&
         (fp->status_vars.common.guard.slide_tics == 0) &&
         (fp->status_vars.common.guard.is_release == FALSE) &&
         (fp->status_vars.common.guard.is_setoff == FALSE)) ? 1u : 0u;

    if (gNdsStageMPPassiveLoopAppealGuardCatchCheckCount == 1u)
    {
        mask |= 1u << 1;
    }
    if ((gNdsStageMPPassiveLoopAppealGuardCheckCount == 1u) &&
        (gNdsStageMPPassiveLoopAppealGuardSetStatusCount == 1u))
    {
        mask |= 1u << 2;
    }
    if ((fp->status_id == nFTCommonStatusGuardOn) &&
        (fp->motion_id == nFTCommonMotionGuardOn) &&
        (fp->ga == nMPKineticsGround))
    {
        mask |= 1u << 3;
    }
    if (gNdsStageMPPassiveLoopAppealGuardCallbacksAfter != 0u)
    {
        mask |= 1u << 4;
    }
    if ((gNdsStageMPPassiveLoopAppealGuardShieldAfter != 0u) &&
        ((fp->input.pl.button_hold & fp->input.button_mask_z) != 0u))
    {
        mask |= 1u << 5;
    }
    if (sNdsStageMPPassiveLoopAppealGuardActive == FALSE)
    {
        mask |= 1u << 6;
    }

    for (angle = 0u; angle < 8u; angle++)
    {
        attr->shield_anim_joints[angle] = saved_shield_anim_joints[angle];
    }
    attr->dobj_lookup = saved_dobj_lookup;
    attr->shield_size = saved_shield_size;
    fp->is_have_translate_scale = saved_have_translate_scale;
    fp->joints[nFTPartsJointXRotN] = saved_xrotn_joint;
    fp->joints[nFTPartsJointYRotN] = saved_yrotn_joint;
    yrotn_joint->scale.vec.f = saved_yrotn_scale;
    yrotn_joint->rotate.vec.f = saved_yrotn_rotate;
    yrotn_joint->translate.vec.f = saved_yrotn_translate;
    yrotn_joint->anim_wait = saved_yrotn_anim_wait;
    fp->shield_health = saved_shield_health;
    fp->is_shield = saved_is_shield;
    ftCommonWaitSetStatus(fighter_gobj);

    gNdsStageMPPassiveLoopAppealGuardMask = mask;
}

static void ndsStageMPPassiveLoopRunAppealProof(GObj *fighter_gobj,
                                                FTStruct *fp)
{
    u32 mask = 0u;
    sb32 result;

    if (NDS_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP_HARNESS == 0)
    {
        return;
    }
    if ((fighter_gobj == NULL) || (fp == NULL))
    {
        gNdsStageMPPassiveLoopUnsafeCount++;
        return;
    }
    if ((fp->status_id != nFTCommonStatusWait) ||
        (fp->motion_id != nFTCommonMotionWait) ||
        (fp->ga != nMPKineticsGround))
    {
        gNdsStageMPPassiveLoopUnsafeCount++;
        return;
    }

    if (fp->input.button_mask_l == 0u)
    {
        fp->input.button_mask_l = 0x0020u;
    }
    fp->input.pl.stick_range.x = 0;
    fp->input.pl.stick_range.y = 0;
    fp->input.pl.button_hold = 0u;
    fp->input.pl.button_tap = fp->input.button_mask_l;
    gNdsStageMPPassiveLoopAppealInputButtonTap =
        (u32)fp->input.pl.button_tap;
    gNdsStageMPPassiveLoopAppealInputButtonMaskL =
        (u32)fp->input.button_mask_l;

    sNdsStageMPPassiveLoopAppealActive = TRUE;
    gNdsStageMPPassiveLoopAppealCheckCount++;
    result = ftCommonAppealCheckInterruptCommon(fighter_gobj);
    sNdsStageMPPassiveLoopAppealActive = FALSE;

    gNdsStageMPPassiveLoopAppealStatusAfter = (u32)fp->status_id;
    gNdsStageMPPassiveLoopAppealMotionAfter = (u32)fp->motion_id;
    gNdsStageMPPassiveLoopAppealGAAfter = (u32)fp->ga;
    gNdsStageMPPassiveLoopAppealProcCallbacksSetAfter =
        ((fp->proc_update == ftAnimEndSetWait) &&
         (fp->proc_interrupt == ndsBaseFTCommonAppealProcInterrupt) &&
         (fp->proc_physics == ftPhysicsApplyGroundVelFriction) &&
         (fp->proc_map == mpCommonProcFighterOnCliffEdge) &&
         (fp->proc_damage == NULL)) ? 1u : 0u;

    if (result != FALSE)
    {
        mask |= 1u << 0;
    }
    if ((fp->status_id == nFTCommonStatusAppeal) &&
        (fp->motion_id == nFTCommonMotionAppeal) &&
        (fp->ga == nMPKineticsGround))
    {
        mask |= 1u << 1;
    }
    if (gNdsStageMPPassiveLoopAppealProcCallbacksSetAfter != 0u)
    {
        mask |= 1u << 2;
    }
    if (gNdsStageMPPassiveLoopAppealSetStatusCount == 1u)
    {
        mask |= 1u << 3;
    }
    if (sNdsStageMPPassiveLoopAppealActive == FALSE)
    {
        mask |= 1u << 4;
    }
    if (fp->proc_interrupt == ndsBaseFTCommonAppealProcInterrupt)
    {
        fp->motion_vars.flags.flag1 = 0;
        fp->input.pl.button_tap = 0u;
        fp->proc_interrupt(fighter_gobj);
        if ((fp->status_id == nFTCommonStatusAppeal) &&
            (fp->motion_id == nFTCommonMotionAppeal) &&
            (fp->ga == nMPKineticsGround))
        {
            mask |= 1u << 5;
        }
    }
    if (fp->proc_update == ftAnimEndSetWait)
    {
        fighter_gobj->anim_frame = 0.0F;
        fp->proc_update(fighter_gobj);
        if ((fp->status_id == nFTCommonStatusWait) &&
            (fp->motion_id == nFTCommonMotionWait) &&
            (fp->ga == nMPKineticsGround) &&
            (fp->proc_update == NULL) &&
            (fp->proc_interrupt == ftCommonWaitProcInterrupt) &&
            (fp->proc_physics == ftPhysicsApplyGroundVelFriction) &&
            (fp->proc_map == mpCommonProcFighterOnCliffEdge) &&
            (fp->proc_damage == NULL))
        {
            mask |= 1u << 6;
        }
    }
    gNdsStageMPPassiveLoopAppealMask = mask;
    ndsStageMPPassiveLoopRunAppealGuardProof(fighter_gobj, fp);
}

static void ndsStageMPPassiveLoopRunCatchProof(GObj *fighter_gobj,
                                               FTStruct *fp)
{
    FTAttributes *attr;
    u32 saved_is_have_catch;
    u16 saved_button_mask_a;
    u16 saved_button_mask_z;
    u16 saved_button_hold;
    u16 saved_button_tap;
    u8 saved_catch_mask;
    void (*saved_proc_catch)(GObj *);
    void (*saved_proc_capture)(GObj *, GObj *);
    sb32 saved_is_shield_catch;
    s32 saved_stick_x;
    s32 saved_stick_y;
    GObj *saved_search_gobj;
    GObj *saved_catch_gobj;
    GObj *saved_capture_gobj;
    sb32 saved_is_catchstatus;
    sb32 saved_is_catch_or_capture;
    u8 saved_capture_immune_mask;
    void (*saved_proc_slope)(GObj *);
    void (*saved_proc_status)(GObj *);
    u32 mask = 0u;
    u32 callback_mask = 0u;
    u32 pull_mask = 0u;
    sb32 result;

    if (NDS_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP_HARNESS == 0)
    {
        return;
    }
    if ((fighter_gobj == NULL) || (fp == NULL) || (fp->attr == NULL))
    {
        gNdsStageMPPassiveLoopUnsafeCount++;
        return;
    }
    if ((fp->status_id != nFTCommonStatusWait) ||
        (fp->motion_id != nFTCommonMotionWait) ||
        (fp->ga != nMPKineticsGround))
    {
        gNdsStageMPPassiveLoopUnsafeCount++;
        return;
    }

    attr = fp->attr;
    saved_is_have_catch = attr->is_have_catch;
    saved_button_mask_a = fp->input.button_mask_a;
    saved_button_mask_z = fp->input.button_mask_z;
    saved_button_hold = fp->input.pl.button_hold;
    saved_button_tap = fp->input.pl.button_tap;
    saved_stick_x = fp->input.pl.stick_range.x;
    saved_stick_y = fp->input.pl.stick_range.y;
    saved_catch_mask = fp->catch_mask;
    saved_proc_catch = fp->proc_catch;
    saved_proc_capture = fp->proc_capture;
    saved_is_shield_catch = fp->is_shield_catch;
    saved_search_gobj = fp->search_gobj;
    saved_catch_gobj = fp->catch_gobj;
    saved_capture_gobj = fp->capture_gobj;
    saved_is_catchstatus = fp->is_catchstatus;
    saved_is_catch_or_capture = fp->is_catch_or_capture;
    saved_capture_immune_mask = fp->capture_immune_mask;
    saved_proc_slope = fp->proc_slope;
    saved_proc_status = fp->proc_status;

    attr->is_have_catch = TRUE;
    if (fp->input.button_mask_a == 0u)
    {
        fp->input.button_mask_a = A_BUTTON;
    }
    if (fp->input.button_mask_z == 0u)
    {
        fp->input.button_mask_z = Z_TRIG;
    }
    fp->input.pl.stick_range.x = 0;
    fp->input.pl.stick_range.y = 0;
    fp->input.pl.button_hold = fp->input.button_mask_z;
    fp->input.pl.button_tap = fp->input.button_mask_a;

    sNdsStageMPPassiveLoopCatchActive = TRUE;
    result = ftCommonCatchCheckInterruptCommon(fighter_gobj);
    sNdsStageMPPassiveLoopCatchActive = FALSE;

    gNdsStageMPPassiveLoopCatchStatusAfter = (u32)fp->status_id;
    gNdsStageMPPassiveLoopCatchMotionAfter = (u32)fp->motion_id;
    gNdsStageMPPassiveLoopCatchGAAfter = (u32)fp->ga;
    gNdsStageMPPassiveLoopCatchCallbacksAfter =
        ((fp->proc_update == ndsBaseFTCommonCatchProcUpdate) &&
         (fp->proc_interrupt == NULL) &&
         (fp->proc_physics == ftPhysicsApplyGroundVelFriction) &&
         (fp->proc_map == ndsBaseFTCommonCatchProcMap) &&
         (fp->proc_damage == NULL)) ? 1u : 0u;
    gNdsStageMPPassiveLoopCatchParamMaskAfter =
        ((fp->catch_mask == FTCATCHKIND_MASK_COMMON) &&
         (fp->proc_catch == ftCommonCatchPullProcCatch) &&
         (fp->proc_capture == ftCommonCapturePulledProcCapture) &&
         (fp->is_catchstatus != FALSE) &&
         (fp->is_shield_catch == FALSE) &&
         (fp->motion_vars.flags.flag1 == 1u) &&
         (fp->motion_vars.flags.flag2 == 0u) &&
         (fp->status_vars.common.catchmain.catch_pull_anim_frames == 0.0F) &&
         (fp->status_vars.common.catchmain.catch_pull_frame_begin == 0.0F)) ?
            1u : 0u;

    if (result != FALSE)
    {
        mask |= 1u << 0;
    }
    if ((fp->status_id == nFTCommonStatusCatch) &&
        (fp->motion_id == nFTCommonMotionCatch) &&
        (fp->ga == nMPKineticsGround))
    {
        mask |= 1u << 1;
    }
    if (gNdsStageMPPassiveLoopCatchCallbacksAfter != 0u)
    {
        mask |= 1u << 2;
    }
    if ((gNdsStageMPPassiveLoopCatchCheckCount == 1u) &&
        (gNdsStageMPPassiveLoopCatchSuccessCount == 1u) &&
        (gNdsStageMPPassiveLoopCatchSetStatusCount == 1u))
    {
        mask |= 1u << 3;
    }
    if (gNdsStageMPPassiveLoopCatchParamMaskAfter != 0u)
    {
        mask |= 1u << 4;
    }
    if ((gNdsStageMPPassiveLoopCatchItemThrowCheckCount == 1u) &&
        (gNdsStageMPPassiveLoopCatchItemThrowSetStatusCount == 0u) &&
        (gNdsStageMPPassiveLoopCatchPullDeferredCount == 0u) &&
        (gNdsStageMPPassiveLoopCapturePulledDeferredCount == 0u))
    {
        mask |= 1u << 5;
    }
    if (sNdsStageMPPassiveLoopCatchActive == FALSE)
    {
        mask |= 1u << 6;
    }

    if (fp->proc_map == ndsBaseFTCommonCatchProcMap)
    {
        sNdsStageMPPassiveLoopCatchMapActive = TRUE;
        gNdsStageMPPassiveLoopCatchMapTickCount++;
        fp->proc_map(fighter_gobj);
        sNdsStageMPPassiveLoopCatchMapActive = FALSE;
    }
    gNdsStageMPPassiveLoopCatchStatusAfterMap = (u32)fp->status_id;
    gNdsStageMPPassiveLoopCatchMotionAfterMap = (u32)fp->motion_id;
    gNdsStageMPPassiveLoopCatchGAAfterMap = (u32)fp->ga;

    if (gNdsStageMPPassiveLoopCatchMapTickCount == 1u)
    {
        callback_mask |= 1u << 0;
    }
    if (gNdsStageMPPassiveLoopCatchMapEdgeCheckCount == 1u)
    {
        callback_mask |= 1u << 1;
    }
    if ((fp->status_id == nFTCommonStatusCatch) &&
        (fp->motion_id == nFTCommonMotionCatch) &&
        (fp->ga == nMPKineticsGround))
    {
        callback_mask |= 1u << 2;
    }

    if ((fp->proc_catch == ftCommonCatchPullProcCatch) &&
        (fp->attr->joint_itemheavy_id >= 0) &&
        (fp->attr->joint_itemheavy_id < nFTPartsJointNumMax))
    {
        FTStatusVars saved_catch_status_vars = fp->status_vars;
        FTMotionVars saved_catch_motion_vars = fp->motion_vars;
        FTStatusVars saved_target_status_vars;
        FTMotionVars saved_target_motion_vars;
        s32 saved_status_id = fp->status_id;
        s32 saved_motion_id = fp->motion_id;
        s32 saved_motion_script_id = fp->motion_script_id;
        s32 saved_ga = fp->ga;
        s32 saved_target_status_id = 0;
        s32 saved_target_motion_id = 0;
        s32 saved_target_motion_script_id = 0;
        s32 saved_target_ga = 0;
        f32 saved_motion_frame = fp->motion_frame;
        f32 saved_anim_frame = fp->anim_frame;
        f32 saved_anim_speed = fp->anim_speed;
        f32 saved_target_motion_frame = 0.0F;
        f32 saved_target_anim_frame = 0.0F;
        f32 saved_target_anim_speed = 0.0F;
        void (*saved_update)(GObj *) = fp->proc_update;
        void (*saved_interrupt)(GObj *) = fp->proc_interrupt;
        void (*saved_physics)(GObj *) = fp->proc_physics;
        void (*saved_map)(GObj *) = fp->proc_map;
        void (*saved_damage)(GObj *) = fp->proc_damage;
        void (*saved_target_update)(GObj *) = NULL;
        void (*saved_target_interrupt)(GObj *) = NULL;
        void (*saved_target_physics)(GObj *) = NULL;
        void (*saved_target_map)(GObj *) = NULL;
        void (*saved_target_damage)(GObj *) = NULL;
        GObj *target_gobj = NULL;
        FTStruct *target_fp = NULL;
        sb32 saved_target_pulled_wait = FALSE;
        GObj *saved_target_capture_gobj = NULL;
        GObj *saved_target_catch_gobj = NULL;
        GObj *saved_target_item_gobj = NULL;
        sb32 saved_target_is_catch_or_capture = FALSE;
        u8 saved_target_capture_immune_mask = 0u;
        s32 saved_target_lr = 0;
        s32 saved_target_jumps_used = 0;
        s32 saved_target_percent_damage = 0;
        s32 saved_target_damage_value = 0;
        s32 saved_target_hitstatus = 0;
        s32 saved_target_special_hitstatus = 0;
        f32 saved_target_knockback_resist_status = 0.0F;
        f32 saved_target_knockback_resist_passive = 0.0F;
        void (*saved_target_status)(GObj *) = NULL;
        s32 saved_floor_line_id = fp->coll_data.floor_line_id;
        f32 saved_floor_dist = fp->coll_data.floor_dist;
        s32 saved_target_floor_line_id = 0;
        f32 saved_target_floor_dist = 0.0F;
        Vec3f saved_coll_translate = { 0.0F, 0.0F, 0.0F };
        Vec3f saved_target_coll_translate = { 0.0F, 0.0F, 0.0F };
        sb32 saved_coll_translate_ready = FALSE;
        sb32 saved_target_coll_translate_ready = FALSE;
        FTThrownStatusArray *saved_thrown_status = fp->attr->thrown_status;
        FTThrowHitDesc *saved_throw_desc = fp->throw_desc;
        s32 saved_motion_attack_id = fp->motion_attack_id;
        s32 saved_motion_count = fp->motion_count;
        sb32 throw_result = FALSE;
        Vec3f saved_target_root_translate = { 0.0F, 0.0F, 0.0F };
        Vec3f saved_target_root_rotate = { 0.0F, 0.0F, 0.0F };
        Vec3f saved_target_root_scale = { 1.0F, 1.0F, 1.0F };
        u32 i;

        for (i = 0u; i < GMCOMMON_PLAYERS_MAX; i++)
        {
            FTStruct *candidate = &sNdsFighterStructPool[i];

            if ((candidate != fp) &&
                (ndsFighterStructIsPoolPointer(candidate) != FALSE) &&
                (candidate->fighter_gobj != NULL))
            {
                target_fp = candidate;
                target_gobj = candidate->fighter_gobj;
                break;
            }
        }

        if (target_gobj != NULL)
        {
            DObj *target_root = DObjGetStruct(target_gobj);

            saved_target_pulled_wait =
                target_fp->status_vars.common.capture.is_goto_pulled_wait;
            saved_target_status_vars = target_fp->status_vars;
            saved_target_motion_vars = target_fp->motion_vars;
            saved_target_status_id = target_fp->status_id;
            saved_target_motion_id = target_fp->motion_id;
            saved_target_motion_script_id = target_fp->motion_script_id;
            saved_target_ga = target_fp->ga;
            saved_target_motion_frame = target_fp->motion_frame;
            saved_target_anim_frame = target_fp->anim_frame;
            saved_target_anim_speed = target_fp->anim_speed;
            saved_target_update = target_fp->proc_update;
            saved_target_interrupt = target_fp->proc_interrupt;
            saved_target_physics = target_fp->proc_physics;
            saved_target_map = target_fp->proc_map;
            saved_target_damage = target_fp->proc_damage;
            saved_target_capture_gobj = target_fp->capture_gobj;
            saved_target_catch_gobj = target_fp->catch_gobj;
            saved_target_item_gobj = target_fp->item_gobj;
            saved_target_is_catch_or_capture =
                target_fp->is_catch_or_capture;
            saved_target_capture_immune_mask =
                target_fp->capture_immune_mask;
            saved_target_lr = target_fp->lr;
            saved_target_jumps_used = target_fp->jumps_used;
            saved_target_percent_damage = target_fp->percent_damage;
            saved_target_damage_value = target_fp->damage;
            saved_target_hitstatus = target_fp->hitstatus;
            saved_target_special_hitstatus = target_fp->special_hitstatus;
            saved_target_knockback_resist_status =
                target_fp->knockback_resist_status;
            saved_target_knockback_resist_passive =
                target_fp->knockback_resist_passive;
            saved_target_status = target_fp->proc_status;
            saved_target_floor_line_id = target_fp->coll_data.floor_line_id;
            saved_target_floor_dist = target_fp->coll_data.floor_dist;
            if (fp->coll_data.p_translate != NULL)
            {
                saved_coll_translate = *fp->coll_data.p_translate;
                saved_coll_translate_ready = TRUE;
            }
            if (target_fp->coll_data.p_translate != NULL)
            {
                saved_target_coll_translate =
                    *target_fp->coll_data.p_translate;
                saved_target_coll_translate_ready = TRUE;
            }
            if (target_root != NULL)
            {
                saved_target_root_translate = target_root->translate.vec.f;
                saved_target_root_rotate = target_root->rotate.vec.f;
                saved_target_root_scale = target_root->scale.vec.f;
            }
            fp->search_gobj = target_gobj;
            fp->proc_slope = NULL;
            gNdsStageMPPassiveLoopCatchPullSearchGObjReady = 1u;

            sNdsStageMPPassiveLoopCatchPullActive = TRUE;
            fp->proc_catch(fighter_gobj);
            sNdsStageMPPassiveLoopCatchPullActive = FALSE;

            gNdsStageMPPassiveLoopCatchPullStatusAfter =
                (u32)fp->status_id;
            gNdsStageMPPassiveLoopCatchPullMotionAfter =
                (u32)fp->motion_id;
            gNdsStageMPPassiveLoopCatchPullGAAfter = (u32)fp->ga;
            gNdsStageMPPassiveLoopCatchPullCatchGObjReady =
                (fp->catch_gobj == target_gobj) ? 1u : 0u;

            if ((fp->proc_capture == ftCommonCapturePulledProcCapture) &&
                (target_root != NULL) &&
                (target_root->child != NULL))
            {
                target_fp->proc_damage =
                    ndsStageMPPassiveLoopCaptureProcDamage;
                sNdsStageMPPassiveLoopCaptureActive = TRUE;
                fp->proc_capture(target_gobj, fighter_gobj);
                sNdsStageMPPassiveLoopCaptureActive = FALSE;

                gNdsStageMPPassiveLoopCapturePulledStatusAfter =
                    (u32)target_fp->status_id;
                gNdsStageMPPassiveLoopCapturePulledMotionAfter =
                    (u32)target_fp->motion_id;
                gNdsStageMPPassiveLoopCapturePulledGAAfter =
                    (u32)target_fp->ga;
                gNdsStageMPPassiveLoopCaptureGObjReady =
                    (target_fp->capture_gobj == fighter_gobj) ? 1u : 0u;
                gNdsStageMPPassiveLoopCaptureLR = target_fp->lr;
            }

            if ((fp->proc_update == ndsBaseFTCommonCatchPullProcUpdate) &&
                (fp->status_id == nFTCommonStatusCatchPull))
            {
                fighter_gobj->anim_frame = 0.0F;
                fp->anim_frame = 0.0F;
                gNdsStageMPPassiveLoopCatchPullUpdateTickCount++;
                sNdsStageMPPassiveLoopCatchPullUpdateActive = TRUE;
                fp->proc_update(fighter_gobj);
                sNdsStageMPPassiveLoopCatchPullUpdateActive = FALSE;
            }

            gNdsStageMPPassiveLoopCatchWaitStatusAfter =
                (u32)fp->status_id;
            gNdsStageMPPassiveLoopCatchWaitMotionAfter =
                (u32)fp->motion_id;
            gNdsStageMPPassiveLoopCatchWaitGAAfter = (u32)fp->ga;
            gNdsStageMPPassiveLoopCatchWaitThrowWaitAfterSet =
                (u32)fp->status_vars.common.catchwait.throw_wait;
            gNdsStageMPPassiveLoopCatchPullTargetCaptureFlag =
                (target_fp->status_vars.common.capture.is_goto_pulled_wait !=
                    FALSE) ? 1u : 0u;

            if ((target_fp->proc_physics ==
                    ndsBaseFTCommonCapturePulledProcPhysics) &&
                (target_fp->status_id == nFTCommonStatusCapturePulled))
            {
                gNdsStageMPPassiveLoopCapturePhysicsTickCount++;
                sNdsStageMPPassiveLoopCapturePhysicsActive = TRUE;
                target_fp->proc_physics(target_gobj);
                sNdsStageMPPassiveLoopCapturePhysicsActive = FALSE;
            }
            gNdsStageMPPassiveLoopCaptureWaitStatusAfter =
                (u32)target_fp->status_id;
            gNdsStageMPPassiveLoopCaptureWaitMotionAfter =
                (u32)target_fp->motion_id;
            gNdsStageMPPassiveLoopCaptureWaitGAAfter = (u32)target_fp->ga;

            if (target_fp->proc_map == ndsBaseFTCommonCaptureWaitProcMap)
            {
                gNdsStageMPPassiveLoopCaptureWaitMapTickCount++;
                sNdsStageMPPassiveLoopCaptureWaitMapActive = TRUE;
                target_fp->proc_map(target_gobj);
                sNdsStageMPPassiveLoopCaptureWaitMapActive = FALSE;
            }
            gNdsStageMPPassiveLoopCaptureJumpsUsedAfter =
                target_fp->jumps_used;
            if (target_root != NULL)
            {
                gNdsStageMPPassiveLoopCaptureRootXMilli =
                    ndsFloatToMilliSigned(target_root->translate.vec.f.x);
                gNdsStageMPPassiveLoopCaptureRootYMilli =
                    ndsFloatToMilliSigned(target_root->translate.vec.f.y);
            }

            if (fp->proc_interrupt == ndsBaseFTCommonCatchWaitProcInterrupt)
            {
                fp->input.pl.button_tap = 0;
                fp->input.pl.stick_range.x = 0;
                fp->input.pl.stick_prev.x = 0;
                gNdsStageMPPassiveLoopCatchWaitInterruptTickCount++;
                sNdsStageMPPassiveLoopCatchWaitInterruptActive = TRUE;
                fp->proc_interrupt(fighter_gobj);
                sNdsStageMPPassiveLoopCatchWaitInterruptActive = FALSE;
            }
            gNdsStageMPPassiveLoopCatchWaitThrowWaitAfterTick =
                (u32)fp->status_vars.common.catchwait.throw_wait;

            if (gNdsStageMPPassiveLoopCatchPullSearchGObjReady != 0u)
            {
                pull_mask |= 1u << 0;
            }
            if ((gNdsStageMPPassiveLoopCatchPullProcCatchCount == 1u) &&
                (gNdsStageMPPassiveLoopCatchPullSetStatusCount == 1u) &&
                (gNdsStageMPPassiveLoopCatchPullStatusAfter ==
                    (u32)nFTCommonStatusCatchPull) &&
                (gNdsStageMPPassiveLoopCatchPullMotionAfter ==
                    (u32)nFTCommonMotionCatchPull) &&
                (gNdsStageMPPassiveLoopCatchPullGAAfter ==
                    (u32)nMPKineticsGround))
            {
                pull_mask |= 1u << 1;
            }
            if (gNdsStageMPPassiveLoopCatchPullCatchGObjReady != 0u)
            {
                pull_mask |= 1u << 2;
            }
            if ((gNdsStageMPPassiveLoopCatchPullCaptureImmuneCount >= 2u) &&
                (gNdsStageMPPassiveLoopCatchPullEffectCount == 1u) &&
                (gNdsStageMPPassiveLoopCatchPullRumbleCount == 1u))
            {
                pull_mask |= 1u << 3;
            }
            if ((gNdsStageMPPassiveLoopCatchPullUpdateTickCount == 1u) &&
                (gNdsStageMPPassiveLoopCatchWaitSetStatusCount == 1u) &&
                (gNdsStageMPPassiveLoopCatchWaitStatusAfter ==
                    (u32)nFTCommonStatusCatchWait) &&
                (gNdsStageMPPassiveLoopCatchWaitMotionAfter ==
                    (u32)-2) &&
                (gNdsStageMPPassiveLoopCatchWaitGAAfter ==
                    (u32)nMPKineticsGround))
            {
                pull_mask |= 1u << 4;
            }
            if (gNdsStageMPPassiveLoopCatchPullTargetCaptureFlag != 0u)
            {
                pull_mask |= 1u << 5;
            }
            if ((gNdsStageMPPassiveLoopCatchWaitInterruptTickCount == 1u) &&
                (gNdsStageMPPassiveLoopCatchWaitThrowCheckCount == 1u) &&
                (gNdsStageMPPassiveLoopCatchWaitThrowWaitAfterSet ==
                    (u32)FTCOMMON_CATCH_THROW_WAIT) &&
                (gNdsStageMPPassiveLoopCatchWaitThrowWaitAfterTick ==
                    (u32)(FTCOMMON_CATCH_THROW_WAIT - 1)))
            {
                pull_mask |= 1u << 6;
            }
            if ((sNdsStageMPPassiveLoopCatchPullActive == FALSE) &&
                (sNdsStageMPPassiveLoopCatchPullUpdateActive == FALSE) &&
                (sNdsStageMPPassiveLoopCatchWaitInterruptActive == FALSE))
            {
                pull_mask |= 1u << 7;
            }

            if ((gNdsStageMPPassiveLoopCaptureProcCaptureCount == 1u) &&
                (gNdsStageMPPassiveLoopCapturePulledSetStatusCount == 1u) &&
                (gNdsStageMPPassiveLoopCapturePulledStatusAfter ==
                    (u32)nFTCommonStatusCapturePulled) &&
                (gNdsStageMPPassiveLoopCapturePulledMotionAfter ==
                    (u32)nFTCommonMotionCapturePulled))
            {
                gNdsStageMPPassiveLoopCaptureMask |= 1u << 0;
            }
            if ((gNdsStageMPPassiveLoopCaptureGObjReady != 0u) &&
                (gNdsStageMPPassiveLoopCaptureLR == -fp->lr))
            {
                gNdsStageMPPassiveLoopCaptureMask |= 1u << 1;
            }
            if ((gNdsStageMPPassiveLoopCaptureVoiceStopCount == 1u) &&
                (gNdsStageMPPassiveLoopCaptureStopVelCount == 1u) &&
                (gNdsStageMPPassiveLoopCaptureCaptureImmuneCount >= 2u))
            {
                gNdsStageMPPassiveLoopCaptureMask |= 1u << 2;
            }
            if ((gNdsStageMPPassiveLoopCapturePhysicsTickCount == 1u) &&
                (gNdsStageMPPassiveLoopCaptureWaitSetStatusCount == 1u) &&
                (gNdsStageMPPassiveLoopCaptureWaitStatusAfter ==
                    (u32)nFTCommonStatusCaptureWait) &&
                (gNdsStageMPPassiveLoopCaptureWaitMotionAfter == (u32)-2))
            {
                gNdsStageMPPassiveLoopCaptureMask |= 1u << 3;
            }
            if ((gNdsStageMPPassiveLoopCaptureWaitMapTickCount == 1u) &&
                (gNdsStageMPPassiveLoopCaptureWaitGAAfter ==
                    (u32)nMPKineticsGround))
            {
                gNdsStageMPPassiveLoopCaptureMask |= 1u << 4;
            }
            if ((sNdsStageMPPassiveLoopCaptureActive == FALSE) &&
                (sNdsStageMPPassiveLoopCapturePhysicsActive == FALSE) &&
                (sNdsStageMPPassiveLoopCaptureMapActive == FALSE) &&
                (sNdsStageMPPassiveLoopCaptureWaitMapActive == FALSE))
            {
                gNdsStageMPPassiveLoopCaptureMask |= 1u << 5;
            }

            for (i = 0u; i <= nFTKindNull; i++)
            {
                sNdsStageMPPassiveLoopThrownStatus[i].ft_thrown[0].status1 =
                    -1;
                sNdsStageMPPassiveLoopThrownStatus[i].ft_thrown[0].status2 =
                    nFTCommonStatusThrownCommon;
                sNdsStageMPPassiveLoopThrownStatus[i].ft_thrown[1].status1 =
                    -1;
                sNdsStageMPPassiveLoopThrownStatus[i].ft_thrown[1].status2 =
                    nFTCommonStatusThrownCommon;
            }
            fp->attr->thrown_status = sNdsStageMPPassiveLoopThrownStatus;
            fp->input.pl.button_tap = fp->input.button_mask_a;
            fp->input.pl.stick_range.x = 0;
            fp->input.pl.stick_prev.x = 0;
            sNdsStageMPPassiveLoopThrowActive = TRUE;
            throw_result = ftCommonThrowCheckInterruptCatchWait(fighter_gobj);
            sNdsStageMPPassiveLoopThrowActive = FALSE;

            gNdsStageMPPassiveLoopThrowStatusAfter = (u32)fp->status_id;
            gNdsStageMPPassiveLoopThrowMotionAfter = (u32)fp->motion_id;
            gNdsStageMPPassiveLoopThrowGAAfter = (u32)fp->ga;
            gNdsStageMPPassiveLoopThrowCallbacksAfter =
                ((fp->proc_update == ndsBaseFTCommonThrowProcUpdate) &&
                 (fp->proc_interrupt == NULL) &&
                 (fp->proc_physics == ftPhysicsApplyGroundVelFriction) &&
                 (fp->proc_map == mpCommonSetFighterFallOnGroundBreak)) ?
                    1u : 0u;
            gNdsStageMPPassiveLoopThrowTargetStatusAfter =
                (u32)target_fp->status_id;
            gNdsStageMPPassiveLoopThrowTargetMotionAfter =
                (u32)target_fp->motion_id;
            gNdsStageMPPassiveLoopThrowTargetGAAfter = (u32)target_fp->ga;
            gNdsStageMPPassiveLoopThrowTargetJumpsAfter =
                (u32)target_fp->jumps_used;
            gNdsStageMPPassiveLoopThrowTargetCaptureGObjReady =
                (target_fp->capture_gobj == fighter_gobj) ? 1u : 0u;

            if ((throw_result != FALSE) &&
                (gNdsStageMPPassiveLoopThrowCheckCount == 1u))
            {
                gNdsStageMPPassiveLoopThrowMask |= 1u << 0;
            }
            if ((gNdsStageMPPassiveLoopThrowSetStatusCount == 1u) &&
                (gNdsStageMPPassiveLoopThrowStatusAfter ==
                    (u32)nFTCommonStatusThrowF) &&
                (gNdsStageMPPassiveLoopThrowMotionAfter ==
                    (u32)nFTCommonMotionThrowF) &&
                (gNdsStageMPPassiveLoopThrowGAAfter ==
                    (u32)nMPKineticsGround))
            {
                gNdsStageMPPassiveLoopThrowMask |= 1u << 1;
            }
            if (gNdsStageMPPassiveLoopThrowCallbacksAfter != 0u)
            {
                gNdsStageMPPassiveLoopThrowMask |= 1u << 2;
            }
            if ((gNdsStageMPPassiveLoopThrowTargetSetStatusCount == 1u) &&
                (gNdsStageMPPassiveLoopThrowTargetStatusAfter ==
                    (u32)nFTCommonStatusThrownCommon) &&
                (gNdsStageMPPassiveLoopThrowTargetMotionAfter ==
                    (u32)nFTCommonMotionThrownCommon) &&
                (gNdsStageMPPassiveLoopThrowTargetGAAfter ==
                    (u32)nMPKineticsAir) &&
                (gNdsStageMPPassiveLoopThrowTargetJumpsAfter == 1u))
            {
                gNdsStageMPPassiveLoopThrowMask |= 1u << 3;
            }
            if ((gNdsStageMPPassiveLoopThrowAnimEventsCount == 2u) &&
                (gNdsStageMPPassiveLoopThrowCaptureImmuneCount >= 2u))
            {
                gNdsStageMPPassiveLoopThrowMask |= 1u << 4;
            }
            if ((gNdsStageMPPassiveLoopThrowTargetCaptureGObjReady != 0u) &&
                (sNdsStageMPPassiveLoopThrowActive == FALSE))
            {
                gNdsStageMPPassiveLoopThrowMask |= 1u << 5;
            }
            if (gNdsStageMPPassiveLoopUnsafeCount == 0u)
            {
                gNdsStageMPPassiveLoopThrowMask |= 1u << 6;
            }
            if ((gNdsStageMPPassiveLoopThrowMask & 0x7fu) == 0x7fu)
            {
                sb32 throw_b_result;

                fp->status_id = nFTCommonStatusCatchWait;
                fp->motion_id = -2;
                fp->motion_script_id = -2;
                fp->ga = nMPKineticsGround;
                fp->lr = +1;
                fp->catch_gobj = target_gobj;
                fp->input.pl.button_tap = 0;
                fp->input.pl.stick_prev.x = 0;
                fp->input.pl.stick_range.x =
                    -FTCOMMON_CATCH_THROW_STICK_RANGE_MIN;
                fp->status_vars.common.catchwait.throw_wait =
                    FTCOMMON_CATCH_THROW_WAIT;
                target_fp->capture_gobj = fighter_gobj;
                target_fp->jumps_used = 0;

                sNdsStageMPPassiveLoopThrowActive = TRUE;
                throw_b_result =
                    ftCommonThrowCheckInterruptCatchWait(fighter_gobj);
                sNdsStageMPPassiveLoopThrowActive = FALSE;

                gNdsStageMPPassiveLoopThrowBResult =
                    (throw_b_result != FALSE) ? 1u : 0u;
                gNdsStageMPPassiveLoopThrowBStatusAfter =
                    (u32)fp->status_id;
                gNdsStageMPPassiveLoopThrowBMotionAfter =
                    (u32)fp->motion_id;
                gNdsStageMPPassiveLoopThrowBGAAfter = (u32)fp->ga;
                gNdsStageMPPassiveLoopThrowBCallbacksAfter =
                    ((fp->proc_update == ndsBaseFTCommonThrowProcUpdate) &&
                     (fp->proc_interrupt == NULL) &&
                     (fp->proc_physics == ftPhysicsApplyGroundVelFriction) &&
                     (fp->proc_map == mpCommonSetFighterFallOnGroundBreak)) ?
                        1u : 0u;
                gNdsStageMPPassiveLoopThrowBTargetStatusAfter =
                    (u32)target_fp->status_id;
                gNdsStageMPPassiveLoopThrowBTargetMotionAfter =
                    (u32)target_fp->motion_id;
                gNdsStageMPPassiveLoopThrowBTargetGAAfter =
                    (u32)target_fp->ga;
                gNdsStageMPPassiveLoopThrowBTargetJumpsAfter =
                    (u32)target_fp->jumps_used;
                gNdsStageMPPassiveLoopThrowBTargetCaptureGObjReady =
                    (target_fp->capture_gobj == fighter_gobj) ? 1u : 0u;

                if ((gNdsStageMPPassiveLoopThrowBResult != 0u) &&
                    (gNdsStageMPPassiveLoopThrowBStatusAfter ==
                        (u32)nFTCommonStatusThrowB) &&
                    (gNdsStageMPPassiveLoopThrowBMotionAfter ==
                        (u32)nFTCommonMotionThrowB) &&
                    (gNdsStageMPPassiveLoopThrowBGAAfter ==
                        (u32)nMPKineticsGround) &&
                    (gNdsStageMPPassiveLoopThrowBCallbacksAfter != 0u))
                {
                    gNdsStageMPPassiveLoopThrowMask |= 1u << 7;
                }
                if ((gNdsStageMPPassiveLoopThrowBTargetStatusAfter ==
                        (u32)nFTCommonStatusThrownCommon) &&
                    (gNdsStageMPPassiveLoopThrowBTargetMotionAfter ==
                        (u32)nFTCommonMotionThrownCommon) &&
                    (gNdsStageMPPassiveLoopThrowBTargetGAAfter ==
                        (u32)nMPKineticsAir) &&
                    (gNdsStageMPPassiveLoopThrowBTargetJumpsAfter == 1u) &&
                    (gNdsStageMPPassiveLoopThrowBTargetCaptureGObjReady !=
                        0u))
                {
                    gNdsStageMPPassiveLoopThrowMask |= 1u << 8;
                }
                if ((gNdsStageMPPassiveLoopThrowCheckCount == 2u) &&
                    (gNdsStageMPPassiveLoopThrowSetStatusCount == 2u) &&
                    (gNdsStageMPPassiveLoopThrowTargetSetStatusCount == 2u) &&
                    (gNdsStageMPPassiveLoopThrowAnimEventsCount == 4u) &&
                    (gNdsStageMPPassiveLoopThrowCaptureImmuneCount >= 4u) &&
                    (sNdsStageMPPassiveLoopThrowActive == FALSE) &&
                    (gNdsStageMPPassiveLoopUnsafeCount == 0u))
                {
                    gNdsStageMPPassiveLoopThrowMask |= 1u << 9;
                }
            }
            if ((gNdsStageMPPassiveLoopThrowMask & 0x3ffu) == 0x3ffu)
            {
                DObj *fighter_root = DObjGetStruct(fighter_gobj);
                u32 release_mask = 0u;

                if ((target_root != NULL) && (fighter_root != NULL))
                {
                    s32 callback_floor_line_id =
                        (saved_floor_line_id >= 0) ? saved_floor_line_id : 0;
                    u32 callback_mask = 0u;

                    target_fp->capture_gobj = fighter_gobj;
                    target_fp->status_vars.common.thrown.status_id =
                        nFTCommonStatusThrownCommon;
                    target_gobj->anim_frame = 1.0F;
                    target_fp->anim_frame = 1.0F;

                    if ((target_fp->proc_update ==
                            ndsBaseFTCommonThrownProcUpdate) &&
                        (target_fp->proc_physics ==
                            ndsBaseFTCommonThrownProcPhysics) &&
                        (target_fp->proc_map ==
                            ndsBaseFTCommonThrownProcMap))
                    {
                        callback_mask |= 1u << 0;
                    }
                    if (target_fp->proc_update ==
                            ndsBaseFTCommonThrownProcUpdate)
                    {
                        gNdsStageMPPassiveLoopThrowCallbackUpdateCount++;
                        target_fp->proc_update(target_gobj);
                    }
                    gNdsStageMPPassiveLoopThrowCallbackStatusAfterUpdate =
                        (u32)target_fp->status_id;
                    gNdsStageMPPassiveLoopThrowCallbackMotionAfterUpdate =
                        (u32)target_fp->motion_id;
                    gNdsStageMPPassiveLoopThrowCallbackGAAfterUpdate =
                        (u32)target_fp->ga;
                    if ((gNdsStageMPPassiveLoopThrowCallbackUpdateCount ==
                            1u) &&
                        (gNdsStageMPPassiveLoopThrowCallbackStatusAfterUpdate ==
                            (u32)nFTCommonStatusThrownCommon) &&
                        (gNdsStageMPPassiveLoopThrowCallbackMotionAfterUpdate ==
                            (u32)nFTCommonMotionThrownCommon) &&
                        (gNdsStageMPPassiveLoopThrowCallbackGAAfterUpdate ==
                            (u32)nMPKineticsAir))
                    {
                        callback_mask |= 1u << 1;
                    }
                    if (target_fp->proc_physics ==
                            ndsBaseFTCommonThrownProcPhysics)
                    {
                        gNdsStageMPPassiveLoopThrowCallbackPhysicsCount++;
                        target_fp->proc_physics(target_gobj);
                    }
                    if (gNdsStageMPPassiveLoopThrowCallbackPhysicsCount == 1u)
                    {
                        callback_mask |= 1u << 2;
                    }

                    fp->coll_data.floor_line_id = callback_floor_line_id;
                    target_fp->coll_data.floor_line_id = -1;
                    target_root->translate.vec.f =
                        fighter_root->translate.vec.f;
                    if (target_fp->proc_map == ndsBaseFTCommonThrownProcMap)
                    {
                        gNdsStageMPPassiveLoopThrowCallbackMapCount++;
                        target_fp->proc_map(target_gobj);
                    }
                    gNdsStageMPPassiveLoopThrowCallbackFloorLineAfterMap =
                        target_fp->coll_data.floor_line_id;
                    if ((gNdsStageMPPassiveLoopThrowCallbackMapCount == 1u) &&
                        (gNdsStageMPPassiveLoopThrowCallbackFloorLineAfterMap ==
                            callback_floor_line_id))
                    {
                        callback_mask |= 1u << 3;
                    }
                    gNdsStageMPPassiveLoopThrowCallbackCaptureReady =
                        (target_fp->capture_gobj == fighter_gobj) ? 1u : 0u;
                    if (gNdsStageMPPassiveLoopThrowCallbackCaptureReady != 0u)
                    {
                        callback_mask |= 1u << 4;
                    }
                    target_fp->capture_gobj = fighter_gobj;
                    target_fp->status_vars.common.thrown.status_id =
                        nFTCommonStatusThrownCommon;
                    target_gobj->anim_frame = 0.0F;
                    target_fp->anim_frame = 0.0F;
                    if (target_fp->proc_update ==
                            ndsBaseFTCommonThrownProcUpdate)
                    {
                        sNdsStageMPPassiveLoopThrowCallbackImmediateActive =
                            TRUE;
                        gNdsStageMPPassiveLoopThrowCallbackImmediateUpdateCount++;
                        target_fp->proc_update(target_gobj);
                        sNdsStageMPPassiveLoopThrowCallbackImmediateActive =
                            FALSE;
                    }
                    gNdsStageMPPassiveLoopThrowCallbackImmediateStatusAfter =
                        (u32)target_fp->status_id;
                    gNdsStageMPPassiveLoopThrowCallbackImmediateMotionAfter =
                        (u32)target_fp->motion_id;
                    gNdsStageMPPassiveLoopThrowCallbackImmediateGAAfter =
                        (u32)target_fp->ga;
                    gNdsStageMPPassiveLoopThrowCallbackImmediateJumpsAfter =
                        (u32)target_fp->jumps_used;
                    gNdsStageMPPassiveLoopThrowCallbackImmediateCallbacksAfter =
                        ((target_fp->proc_update ==
                            ndsBaseFTCommonThrownProcUpdate) &&
                         (target_fp->proc_physics ==
                            ndsBaseFTCommonThrownProcPhysics) &&
                         (target_fp->proc_map ==
                            ndsBaseFTCommonThrownProcMap)) ? 1u : 0u;
                    if ((gNdsStageMPPassiveLoopThrowCallbackImmediateUpdateCount ==
                            1u) &&
                        (gNdsStageMPPassiveLoopThrowCallbackImmediateSetStatusCount ==
                            1u) &&
                        (gNdsStageMPPassiveLoopThrowCallbackImmediateStatusAfter ==
                            (u32)nFTCommonStatusThrownCommon) &&
                        (gNdsStageMPPassiveLoopThrowCallbackImmediateMotionAfter ==
                            (u32)nFTCommonMotionThrownCommon) &&
                        (gNdsStageMPPassiveLoopThrowCallbackImmediateGAAfter ==
                            (u32)nMPKineticsAir) &&
                        (gNdsStageMPPassiveLoopThrowCallbackImmediateJumpsAfter ==
                            1u))
                    {
                        callback_mask |= 1u << 6;
                    }
                    if ((gNdsStageMPPassiveLoopThrowCallbackImmediateAnimEventsCount ==
                            1u) &&
                        (gNdsStageMPPassiveLoopThrowCallbackImmediateCaptureImmuneCount >=
                            1u))
                    {
                        callback_mask |= 1u << 7;
                    }
                    if ((gNdsStageMPPassiveLoopThrowCallbackImmediateCallbacksAfter !=
                            0u) &&
                        (sNdsStageMPPassiveLoopThrowCallbackImmediateActive ==
                            FALSE))
                    {
                        callback_mask |= 1u << 8;
                    }
                    if (gNdsStageMPPassiveLoopUnsafeCount == 0u)
                    {
                        callback_mask |= 1u << 5;
                    }
                    gNdsStageMPPassiveLoopThrowCallbackMask = callback_mask;
                }

                sNdsStageMPPassiveLoopThrowReleaseDesc[0].status_id = -1;
                sNdsStageMPPassiveLoopThrowReleaseDesc[0].damage = 8;
                sNdsStageMPPassiveLoopThrowReleaseDesc[0].angle = 45;
                sNdsStageMPPassiveLoopThrowReleaseDesc[0].knockback_scale =
                    100;
                sNdsStageMPPassiveLoopThrowReleaseDesc[0].knockback_weight =
                    0;
                sNdsStageMPPassiveLoopThrowReleaseDesc[0].knockback_base =
                    40;
                sNdsStageMPPassiveLoopThrowReleaseDesc[0].element =
                    nGMHitElementNormal;
                sNdsStageMPPassiveLoopThrowReleaseDesc[1] =
                    sNdsStageMPPassiveLoopThrowReleaseDesc[0];

                if ((gNdsStageMPPassiveLoopThrowCallbackMask & 0x1ffu) ==
                        0x1ffu)
                {
                    u32 update_mask = 0u;

                    fp->status_id = nFTCommonStatusThrowF;
                    fp->motion_id = nFTCommonMotionThrowF;
                    fp->motion_script_id = nFTCommonMotionThrowF;
                    fp->ga = nMPKineticsGround;
                    fp->lr = +1;
                    fp->catch_gobj = target_gobj;
                    fp->capture_gobj = NULL;
                    fp->is_catch_or_capture = FALSE;
                    fp->capture_immune_mask = FTCATCHKIND_MASK_ALL;
                    fp->proc_update = ndsBaseFTCommonThrowProcUpdate;
                    fp->proc_interrupt = NULL;
                    fp->proc_physics = ftPhysicsApplyGroundVelFriction;
                    fp->proc_map = mpCommonSetFighterFallOnGroundBreak;
                    fp->proc_damage = NULL;
                    fp->throw_desc =
                        sNdsStageMPPassiveLoopThrowReleaseDesc;
                    fp->motion_attack_id = 0x3234;
                    fp->motion_count = 11;
                    fp->is_shield_catch = FALSE;
                    fp->motion_vars.flags.flag1 = 0;
                    fp->motion_vars.flags.flag2 = 1;
                    fp->anim_frame = 1.0F;
                    fighter_gobj->anim_frame = 1.0F;

                    target_fp->status_id = nFTCommonStatusThrownCommon;
                    target_fp->motion_id = nFTCommonMotionThrownCommon;
                    target_fp->motion_script_id = nFTCommonMotionThrownCommon;
                    target_fp->ga = nMPKineticsAir;
                    target_fp->capture_gobj = fighter_gobj;
                    target_fp->catch_gobj = NULL;
                    target_fp->is_catch_or_capture = TRUE;
                    target_fp->percent_damage = 50;
                    target_fp->damage = 50;
                    target_fp->hitstatus = nGMHitStatusNone;
                    target_fp->special_hitstatus = nGMHitStatusNone;
                    target_fp->knockback_resist_status = 0.0F;
                    target_fp->knockback_resist_passive = 0.0F;
                    target_fp->proc_status = NULL;

                    gNdsStageMPPassiveLoopThrowUpdateTargetDamageBefore =
                        (u32)target_fp->percent_damage;

                    if ((fp->proc_update == ndsBaseFTCommonThrowProcUpdate) &&
                        (fp->status_id == nFTCommonStatusThrowF) &&
                        (target_fp->capture_gobj == fighter_gobj))
                    {
                        update_mask |= 1u << 0;
                    }
                    sNdsStageMPPassiveLoopThrowUpdateActive = TRUE;
                    gNdsStageMPPassiveLoopThrowUpdateTickCount++;
                    fp->proc_update(fighter_gobj);
                    sNdsStageMPPassiveLoopThrowUpdateActive = FALSE;

                    gNdsStageMPPassiveLoopThrowUpdateStatusAfter =
                        (u32)fp->status_id;
                    gNdsStageMPPassiveLoopThrowUpdateMotionAfter =
                        (u32)fp->motion_id;
                    gNdsStageMPPassiveLoopThrowUpdateGAAfter = (u32)fp->ga;
                    gNdsStageMPPassiveLoopThrowUpdateCatchCleared =
                        (fp->catch_gobj == NULL) ? 1u : 0u;
                    gNdsStageMPPassiveLoopThrowUpdateFlag2After =
                        (u32)fp->motion_vars.flags.flag2;
                    gNdsStageMPPassiveLoopThrowUpdateCaptureImmuneAfter =
                        (u32)fp->capture_immune_mask;
                    gNdsStageMPPassiveLoopThrowUpdateTargetDamageAfter =
                        (u32)target_fp->percent_damage;
                    gNdsStageMPPassiveLoopThrowUpdateTargetGAAfter =
                        (u32)target_fp->ga;
                    gNdsStageMPPassiveLoopThrowUpdateTargetCaptureCleared =
                        (target_fp->capture_gobj == NULL) ? 1u : 0u;
                    gNdsStageMPPassiveLoopThrowUpdateTargetProcStatusSet =
                        (target_fp->proc_status ==
                            ndsBaseFTCommonThrownProcStatus) ? 1u : 0u;

                    if ((gNdsStageMPPassiveLoopThrowUpdateTickCount == 1u) &&
                        (gNdsStageMPPassiveLoopThrowUpdateReleaseCount == 1u))
                    {
                        update_mask |= 1u << 1;
                    }
                    if ((gNdsStageMPPassiveLoopThrowUpdateStatusAfter ==
                            (u32)nFTCommonStatusThrowF) &&
                        (gNdsStageMPPassiveLoopThrowUpdateMotionAfter ==
                            (u32)nFTCommonMotionThrowF) &&
                        (gNdsStageMPPassiveLoopThrowUpdateGAAfter ==
                            (u32)nMPKineticsGround) &&
                        (gNdsStageMPPassiveLoopThrowUpdateCatchCleared !=
                            0u) &&
                        (gNdsStageMPPassiveLoopThrowUpdateFlag2After == 0u) &&
                        (gNdsStageMPPassiveLoopThrowUpdateCaptureImmuneAfter ==
                            (u32)FTCATCHKIND_MASK_NONE))
                    {
                        update_mask |= 1u << 2;
                    }
                    if ((gNdsStageMPPassiveLoopThrowUpdateTargetDamageBefore ==
                            50u) &&
                        (gNdsStageMPPassiveLoopThrowUpdateTargetDamageAfter ==
                            58u) &&
                        (gNdsStageMPPassiveLoopThrowUpdateTargetGAAfter ==
                            (u32)nMPKineticsAir) &&
                        (gNdsStageMPPassiveLoopThrowUpdateTargetCaptureCleared !=
                            0u))
                    {
                        update_mask |= 1u << 3;
                    }
                    if ((gNdsStageMPPassiveLoopThrowUpdateReleaseScriptID ==
                            0) &&
                        (gNdsStageMPPassiveLoopThrowUpdateReleaseLR == -fp->lr))
                    {
                        update_mask |= 1u << 4;
                    }
                    if (gNdsStageMPPassiveLoopThrowUpdateTargetProcStatusSet !=
                            0u)
                    {
                        update_mask |= 1u << 5;
                    }
                    if ((sNdsStageMPPassiveLoopThrowUpdateActive == FALSE) &&
                        (gNdsStageMPPassiveLoopUnsafeCount == 0u))
                    {
                        update_mask |= 1u << 6;
                    }
                    gNdsStageMPPassiveLoopThrowUpdateMask = update_mask;
                }

                fp->throw_desc = sNdsStageMPPassiveLoopThrowReleaseDesc;
                fp->motion_attack_id = 0x1234;
                fp->motion_count = 7;
                fp->is_shield_catch = FALSE;
                target_fp->percent_damage = 10;
                target_fp->damage = 10;
                target_fp->hitstatus = nGMHitStatusNone;
                target_fp->special_hitstatus = nGMHitStatusNone;
                target_fp->knockback_resist_status = 0.0F;
                target_fp->knockback_resist_passive = 0.0F;
                target_fp->is_catch_or_capture = TRUE;
                target_fp->capture_gobj = fighter_gobj;
                target_fp->ga = nMPKineticsGround;
                target_fp->proc_status = NULL;
                gNdsStageMPPassiveLoopThrowReleaseDamageBefore =
                    (u32)target_fp->percent_damage;

                sNdsStageMPPassiveLoopThrowReleaseActive = TRUE;
                ftCommonThrownReleaseThrownUpdateStats(
                    target_gobj, fp->lr, 123, TRUE);
                sNdsStageMPPassiveLoopThrowReleaseActive = FALSE;

                gNdsStageMPPassiveLoopThrowReleaseCaptureCleared =
                    (target_fp->capture_gobj == NULL) ? 1u : 0u;
                gNdsStageMPPassiveLoopThrowReleaseGAAfter =
                    (u32)target_fp->ga;
                gNdsStageMPPassiveLoopThrowReleaseProcStatusSet =
                    (target_fp->proc_status ==
                        ndsBaseFTCommonThrownProcStatus) ? 1u : 0u;
                gNdsStageMPPassiveLoopThrowReleaseHitStatusAfter =
                    (u32)target_fp->hitstatus;
                gNdsStageMPPassiveLoopThrowReleaseDamageAfter =
                    (u32)target_fp->percent_damage;

                if (gNdsStageMPPassiveLoopThrowReleaseProcStatusSet != 0u)
                {
                    GObj *saved_proc_capture_gobj = target_fp->capture_gobj;
                    GObj *saved_proc_throw_gobj = target_fp->throw_gobj;
                    s32 saved_proc_script_id =
                        target_fp->status_vars.common.damage.script_id;
                    u32 proc_mask = 0u;

                    target_fp->capture_gobj = fighter_gobj;
                    target_fp->throw_gobj = NULL;
                    target_fp->status_vars.common.damage.script_id = -1;
                    gNdsStageMPPassiveLoopThrowProcStatusCaptureReady =
                        (target_fp->capture_gobj == fighter_gobj) ? 1u : 0u;

                    sNdsStageMPPassiveLoopThrowProcStatusActive = TRUE;
                    gNdsStageMPPassiveLoopThrowProcStatusTickCount++;
                    target_fp->proc_status(target_gobj);
                    sNdsStageMPPassiveLoopThrowProcStatusActive = FALSE;

                    gNdsStageMPPassiveLoopThrowProcStatusThrowGObjReady =
                        (target_fp->throw_gobj == fighter_gobj) ? 1u : 0u;
                    gNdsStageMPPassiveLoopThrowProcStatusScriptID =
                        target_fp->status_vars.common.damage.script_id;

                    if (target_fp->proc_status ==
                            ndsBaseFTCommonThrownProcStatus)
                    {
                        proc_mask |= 1u << 0;
                    }
                    if (gNdsStageMPPassiveLoopThrowProcStatusCaptureReady !=
                            0u)
                    {
                        proc_mask |= 1u << 1;
                    }
                    if (gNdsStageMPPassiveLoopThrowProcStatusTickCount == 1u)
                    {
                        proc_mask |= 1u << 2;
                    }
                    if ((gNdsStageMPPassiveLoopThrowProcStatusParamSetCount ==
                            1u) &&
                        (gNdsStageMPPassiveLoopThrowProcStatusThrowGObjReady !=
                            0u))
                    {
                        proc_mask |= 1u << 3;
                    }
                    if (gNdsStageMPPassiveLoopThrowProcStatusScriptID == 123)
                    {
                        proc_mask |= 1u << 4;
                    }
                    if ((sNdsStageMPPassiveLoopThrowProcStatusActive ==
                            FALSE) &&
                        (gNdsStageMPPassiveLoopUnsafeCount == 0u))
                    {
                        proc_mask |= 1u << 5;
                    }
                    gNdsStageMPPassiveLoopThrowProcStatusMask = proc_mask;

                    target_fp->capture_gobj = saved_proc_capture_gobj;
                    target_fp->throw_gobj = saved_proc_throw_gobj;
                    target_fp->status_vars.common.damage.script_id =
                        saved_proc_script_id;
                }

                if (gNdsStageMPPassiveLoopThrowReleaseUpdateStatsCount == 1u)
                {
                    release_mask |= 1u << 0;
                }
                if ((gNdsStageMPPassiveLoopThrowReleaseDamageInitCount ==
                        1u) &&
                    (gNdsStageMPPassiveLoopThrowReleaseDamageStatsCount ==
                        1u))
                {
                    release_mask |= 1u << 1;
                }
                if ((gNdsStageMPPassiveLoopThrowReleaseDamageUpdateCount ==
                        1u) &&
                    (gNdsStageMPPassiveLoopThrowReleasePlayerStatsCount ==
                        1u) &&
                    (gNdsStageMPPassiveLoopThrowReleaseStaleQueueCount ==
                        1u))
                {
                    release_mask |= 1u << 2;
                }
                if (gNdsStageMPPassiveLoopThrowReleaseRumbleCount == 3u)
                {
                    release_mask |= 1u << 3;
                }
                if ((gNdsStageMPPassiveLoopThrowReleaseCaptureCleared !=
                        0u) &&
                    (gNdsStageMPPassiveLoopThrowReleaseGAAfter ==
                        (u32)nMPKineticsAir))
                {
                    release_mask |= 1u << 4;
                }
                if ((gNdsStageMPPassiveLoopThrowReleaseProcStatusSet != 0u) &&
                    (gNdsStageMPPassiveLoopThrowReleaseScriptID == 123))
                {
                    release_mask |= 1u << 5;
                }
                if ((gNdsStageMPPassiveLoopThrowReleaseDamageBefore == 10u) &&
                    (gNdsStageMPPassiveLoopThrowReleaseDamageAfter == 18u) &&
                    (gNdsStageMPPassiveLoopThrowReleaseDamageInitDamage ==
                        8u))
                {
                    release_mask |= 1u << 6;
                }
                if ((gNdsStageMPPassiveLoopThrowReleaseHitStatusAfter ==
                        (u32)nGMHitStatusNormal) &&
                    (gNdsStageMPPassiveLoopThrowReleaseKnockbackMilli > 0) &&
                    (gNdsStageMPPassiveLoopThrowReleaseLR == fp->lr) &&
                    (sNdsStageMPPassiveLoopThrowReleaseActive == FALSE) &&
                    (gNdsStageMPPassiveLoopUnsafeCount == 0u))
                {
                    release_mask |= 1u << 7;
                }
                gNdsStageMPPassiveLoopThrowReleaseMask = release_mask;
                if (((release_mask & 0xffu) == 0xffu) &&
                    (target_root != NULL) &&
                    (DObjGetStruct(fighter_gobj) != NULL))
                {
                    u32 status_mask = 0u;

                    sNdsStageMPPassiveLoopThrowReleaseDesc[1].damage = 6;
                    sNdsStageMPPassiveLoopThrowReleaseDesc[1].angle = 60;
                    sNdsStageMPPassiveLoopThrowReleaseDesc[1].knockback_base =
                        30;
                    fp->throw_desc =
                        sNdsStageMPPassiveLoopThrowReleaseDesc;
                    fp->motion_attack_id = 0x2234;
                    fp->motion_count = 9;
                    fp->is_shield_catch = FALSE;
                    target_fp->percent_damage = 20;
                    target_fp->damage = 20;
                    target_fp->damage_queue = 99;
                    target_fp->hitstatus = nGMHitStatusNone;
                    target_fp->special_hitstatus = nGMHitStatusNone;
                    target_fp->knockback_resist_status = 0.0F;
                    target_fp->knockback_resist_passive = 0.0F;
                    target_fp->is_catch_or_capture = TRUE;
                    target_fp->capture_gobj = fighter_gobj;
                    target_fp->ga = nMPKineticsGround;
                    gNdsStageMPPassiveLoopThrowReleaseStatusDamageBefore =
                        (u32)target_fp->percent_damage;

                    sNdsStageMPPassiveLoopThrowReleaseStatusActive = TRUE;
                    ftCommonThrownSetStatusDamageRelease(target_gobj);

                    gNdsStageMPPassiveLoopThrowReleaseStatusDamageAfter =
                        (u32)target_fp->percent_damage;
                    gNdsStageMPPassiveLoopThrowReleaseStatusCaptureCleared =
                        (target_fp->capture_gobj == NULL) ? 1u : 0u;
                    gNdsStageMPPassiveLoopThrowReleaseStatusHitStatusAfter =
                        (u32)target_fp->hitstatus;
                    gNdsStageMPPassiveLoopThrowReleaseStatusLR =
                        target_fp->hit_lr;

                    target_fp->capture_gobj = fighter_gobj;
                    target_fp->is_catch_or_capture = TRUE;
                    target_fp->percent_damage = 30;
                    target_fp->damage = 30;
                    target_fp->hitstatus = nGMHitStatusNormal;
                    target_fp->special_hitstatus = nGMHitStatusNormal;
                    gNdsStageMPPassiveLoopThrowReleaseStatusUpdateBefore =
                        (u32)target_fp->percent_damage;
                    ftCommonThrownUpdateDamageStats(target_fp);
                    gNdsStageMPPassiveLoopThrowReleaseStatusUpdateAfter =
                        (u32)target_fp->percent_damage;

                    target_fp->percent_damage = 40;
                    target_fp->damage = 40;
                    target_fp->damage_queue = 99;
                    target_fp->lr = fp->lr;
                    target_fp->knockback_resist_status = 0.0F;
                    target_fp->knockback_resist_passive = 0.0F;
                    gNdsStageMPPassiveLoopThrowReleaseStatusNoDamageBefore =
                        (u32)target_fp->percent_damage;
                    ftCommonThrownSetStatusNoDamageRelease(target_gobj);
                    sNdsStageMPPassiveLoopThrowReleaseStatusActive = FALSE;

                    gNdsStageMPPassiveLoopThrowReleaseStatusNoDamageAfter =
                        (u32)target_fp->percent_damage;
                    gNdsStageMPPassiveLoopThrowReleaseStatusNoDamageQueue =
                        (u32)target_fp->damage_queue;

                    if ((gNdsStageMPPassiveLoopThrowReleaseStatusDamageReleaseCount ==
                            1u) &&
                        (gNdsStageMPPassiveLoopThrowReleaseStatusDamageBefore ==
                            20u) &&
                        (gNdsStageMPPassiveLoopThrowReleaseStatusDamageAfter ==
                            26u))
                    {
                        status_mask |= 1u << 0;
                    }
                    if ((gNdsStageMPPassiveLoopThrowReleaseStatusCaptureCleared !=
                            0u) &&
                        (gNdsStageMPPassiveLoopThrowReleaseStatusHitStatusAfter ==
                            (u32)nGMHitStatusNormal) &&
                        (gNdsStageMPPassiveLoopThrowReleaseStatusLR != 0))
                    {
                        status_mask |= 1u << 1;
                    }
                    if ((gNdsStageMPPassiveLoopThrowReleaseStatusUpdateDamageStatsCount ==
                            1u) &&
                        (gNdsStageMPPassiveLoopThrowReleaseStatusUpdateBefore ==
                            30u) &&
                        (gNdsStageMPPassiveLoopThrowReleaseStatusUpdateAfter ==
                            36u))
                    {
                        status_mask |= 1u << 2;
                    }
                    if ((gNdsStageMPPassiveLoopThrowReleaseStatusNoDamageReleaseCount ==
                            1u) &&
                        (gNdsStageMPPassiveLoopThrowReleaseStatusNoDamageBefore ==
                            40u) &&
                        (gNdsStageMPPassiveLoopThrowReleaseStatusNoDamageAfter ==
                            40u) &&
                        (gNdsStageMPPassiveLoopThrowReleaseStatusNoDamageQueue ==
                            0u))
                    {
                        status_mask |= 1u << 3;
                    }
                    if ((sNdsStageMPPassiveLoopThrowReleaseStatusActive ==
                            FALSE) &&
                        (gNdsStageMPPassiveLoopUnsafeCount == 0u))
                    {
                        status_mask |= 1u << 4;
                    }
                    gNdsStageMPPassiveLoopThrowReleaseStatusMask =
                        status_mask;
                    if ((status_mask & 0x1fu) == 0x1fu)
                    {
                        u32 dead_mask = 0u;

                        fp->status_id = nFTCommonStatusCatchWait;
                        fp->ga = nMPKineticsGround;
                        fp->catch_gobj = target_gobj;
                        fp->capture_gobj = NULL;
                        fp->is_catch_or_capture = FALSE;
                        target_fp->status_id = nFTCommonStatusCaptureWait;
                        target_fp->ga = nMPKineticsGround;
                        target_fp->catch_gobj = NULL;
                        target_fp->capture_gobj = fighter_gobj;
                        target_fp->is_catch_or_capture = FALSE;
                        target_fp->coll_data.floor_line_id = -1;
                        target_fp->coll_data.floor_dist = 1.0F;

                        sNdsStageMPPassiveLoopThrowDeadResultActive = TRUE;
                        ftCommonThrownDecideDeadResult(fighter_gobj);
                        sNdsStageMPPassiveLoopThrowDeadResultActive = FALSE;

                        gNdsStageMPPassiveLoopThrowDeadResultCatchCleared =
                            (fp->catch_gobj == NULL) ? 1u : 0u;
                        gNdsStageMPPassiveLoopThrowDeadResultCaptureCleared =
                            (target_fp->capture_gobj == NULL) ? 1u : 0u;
                        gNdsStageMPPassiveLoopThrowDeadResultFighterStatusAfter =
                            (u32)fp->status_id;
                        gNdsStageMPPassiveLoopThrowDeadResultTargetStatusAfter =
                            (u32)target_fp->status_id;
                        gNdsStageMPPassiveLoopThrowDeadResultFighterGAAfter =
                            (u32)fp->ga;
                        gNdsStageMPPassiveLoopThrowDeadResultTargetGAAfter =
                            (u32)target_fp->ga;

                        if (gNdsStageMPPassiveLoopThrowDeadResultCallCount ==
                                1u)
                        {
                            dead_mask |= 1u << 0;
                        }
                        if (gNdsStageMPPassiveLoopThrowDeadResultCollisionCount ==
                                1u)
                        {
                            dead_mask |= 1u << 1;
                        }
                        if (gNdsStageMPPassiveLoopThrowDeadResultSetAirCount ==
                                1u)
                        {
                            dead_mask |= 1u << 2;
                        }
                        if (gNdsStageMPPassiveLoopThrowDeadResultWaitOrFallCount ==
                                2u)
                        {
                            dead_mask |= 1u << 3;
                        }
                        if ((gNdsStageMPPassiveLoopThrowDeadResultCatchCleared !=
                                0u) &&
                            (gNdsStageMPPassiveLoopThrowDeadResultCaptureCleared !=
                                0u))
                        {
                            dead_mask |= 1u << 4;
                        }
                        if ((gNdsStageMPPassiveLoopThrowDeadResultFighterStatusAfter ==
                                (u32)nFTCommonStatusWait) &&
                            (gNdsStageMPPassiveLoopThrowDeadResultTargetStatusAfter ==
                                (u32)nFTCommonStatusFall) &&
                            (gNdsStageMPPassiveLoopThrowDeadResultFighterGAAfter ==
                                (u32)nMPKineticsGround) &&
                            (gNdsStageMPPassiveLoopThrowDeadResultTargetGAAfter ==
                                (u32)nMPKineticsAir))
                        {
                            dead_mask |= 1u << 5;
                        }
                        if ((sNdsStageMPPassiveLoopThrowDeadResultActive ==
                                FALSE) &&
                            (gNdsStageMPPassiveLoopUnsafeCount == 0u))
                        {
                            dead_mask |= 1u << 6;
                        }
                        gNdsStageMPPassiveLoopThrowDeadResultMask = dead_mask;
                    }
                }
            }
            fp->attr->thrown_status = saved_thrown_status;
            fp->throw_desc = saved_throw_desc;
            fp->motion_attack_id = saved_motion_attack_id;
            fp->motion_count = saved_motion_count;

            target_fp->status_vars.common.capture.is_goto_pulled_wait =
                saved_target_pulled_wait;
            target_fp->status_vars = saved_target_status_vars;
            target_fp->motion_vars = saved_target_motion_vars;
            target_fp->status_id = saved_target_status_id;
            target_fp->motion_id = saved_target_motion_id;
            target_fp->motion_script_id = saved_target_motion_script_id;
            target_fp->ga = saved_target_ga;
            target_fp->motion_frame = saved_target_motion_frame;
            target_fp->anim_frame = saved_target_anim_frame;
            target_fp->anim_speed = saved_target_anim_speed;
            target_fp->proc_update = saved_target_update;
            target_fp->proc_interrupt = saved_target_interrupt;
            target_fp->proc_physics = saved_target_physics;
            target_fp->proc_map = saved_target_map;
            target_fp->proc_damage = saved_target_damage;
            target_fp->capture_gobj = saved_target_capture_gobj;
            target_fp->catch_gobj = saved_target_catch_gobj;
            target_fp->item_gobj = saved_target_item_gobj;
            target_fp->is_catch_or_capture =
                saved_target_is_catch_or_capture;
            target_fp->capture_immune_mask =
                saved_target_capture_immune_mask;
            target_fp->lr = saved_target_lr;
            target_fp->jumps_used = saved_target_jumps_used;
            target_fp->percent_damage = saved_target_percent_damage;
            target_fp->damage = saved_target_damage_value;
            target_fp->hitstatus = saved_target_hitstatus;
            target_fp->special_hitstatus = saved_target_special_hitstatus;
            target_fp->knockback_resist_status =
                saved_target_knockback_resist_status;
            target_fp->knockback_resist_passive =
                saved_target_knockback_resist_passive;
            target_fp->proc_status = saved_target_status;
            target_fp->coll_data.floor_line_id = saved_target_floor_line_id;
            target_fp->coll_data.floor_dist = saved_target_floor_dist;
            if ((saved_target_coll_translate_ready != FALSE) &&
                (target_fp->coll_data.p_translate != NULL))
            {
                *target_fp->coll_data.p_translate =
                    saved_target_coll_translate;
            }
            if (target_root != NULL)
            {
                target_root->translate.vec.f = saved_target_root_translate;
                target_root->rotate.vec.f = saved_target_root_rotate;
                target_root->scale.vec.f = saved_target_root_scale;
            }
        }

        fp->status_id = saved_status_id;
        fp->motion_id = saved_motion_id;
        fp->motion_script_id = saved_motion_script_id;
        fp->ga = saved_ga;
        fp->motion_frame = saved_motion_frame;
        fp->anim_frame = saved_anim_frame;
        fp->anim_speed = saved_anim_speed;
        fp->status_vars = saved_catch_status_vars;
        fp->motion_vars = saved_catch_motion_vars;
        fp->proc_update = saved_update;
        fp->proc_interrupt = saved_interrupt;
        fp->proc_physics = saved_physics;
        fp->proc_map = saved_map;
        fp->proc_damage = saved_damage;
        fp->search_gobj = saved_search_gobj;
        fp->catch_gobj = saved_catch_gobj;
        fp->capture_gobj = saved_capture_gobj;
        fp->is_catchstatus = saved_is_catchstatus;
        fp->is_catch_or_capture = saved_is_catch_or_capture;
        fp->capture_immune_mask = saved_capture_immune_mask;
        fp->proc_slope = saved_proc_slope;
        fp->proc_status = saved_proc_status;
        fp->coll_data.floor_line_id = saved_floor_line_id;
        fp->coll_data.floor_dist = saved_floor_dist;
        if ((saved_coll_translate_ready != FALSE) &&
            (fp->coll_data.p_translate != NULL))
        {
            *fp->coll_data.p_translate = saved_coll_translate;
        }
        fighter_gobj->anim_frame = saved_anim_frame;
    }

    if (fp->proc_update == ndsBaseFTCommonCatchProcUpdate)
    {
        fighter_gobj->anim_frame = 0.0F;
        fp->anim_frame = 0.0F;
        sNdsStageMPPassiveLoopCatchUpdateActive = TRUE;
        fp->proc_update(fighter_gobj);
        sNdsStageMPPassiveLoopCatchUpdateActive = FALSE;
    }
    gNdsStageMPPassiveLoopCatchStatusAfterUpdate = (u32)fp->status_id;
    gNdsStageMPPassiveLoopCatchMotionAfterUpdate = (u32)fp->motion_id;
    gNdsStageMPPassiveLoopCatchGAAfterUpdate = (u32)fp->ga;

    if (gNdsStageMPPassiveLoopCatchUpdateTickCount == 1u)
    {
        callback_mask |= 1u << 3;
    }
    if ((fp->status_id == nFTCommonStatusWait) &&
        (fp->motion_id == nFTCommonMotionWait) &&
        (fp->ga == nMPKineticsGround))
    {
        callback_mask |= 1u << 4;
    }
    if ((fp->proc_update == NULL) &&
        (fp->proc_interrupt == ftCommonWaitProcInterrupt) &&
        (fp->proc_physics == ftPhysicsApplyGroundVelFriction) &&
        (fp->proc_map == mpCommonProcFighterOnCliffEdge) &&
        (fp->proc_damage == NULL))
    {
        callback_mask |= 1u << 5;
    }
    if ((sNdsStageMPPassiveLoopCatchMapActive == FALSE) &&
        (sNdsStageMPPassiveLoopCatchUpdateActive == FALSE))
    {
        callback_mask |= 1u << 6;
    }
    if ((callback_mask & 0x7fu) == 0x7fu)
    {
        mask |= 1u << 7;
    }
    if ((pull_mask & 0xffu) == 0xffu)
    {
        mask |= 1u << 8;
    }
    if ((gNdsStageMPPassiveLoopCaptureMask & 0x3fu) == 0x3fu)
    {
        mask |= 1u << 9;
    }
    if ((gNdsStageMPPassiveLoopThrowMask & 0x3ffu) == 0x3ffu)
    {
        mask |= 1u << 10;
    }
    if ((gNdsStageMPPassiveLoopThrowCallbackMask & 0x1ffu) == 0x1ffu)
    {
        mask |= 1u << 11;
    }
    if ((gNdsStageMPPassiveLoopThrowUpdateMask & 0x7fu) == 0x7fu)
    {
        mask |= 1u << 12;
    }

    ftCommonWaitSetStatus(fighter_gobj);
    attr->is_have_catch = saved_is_have_catch;
    fp->input.button_mask_a = saved_button_mask_a;
    fp->input.button_mask_z = saved_button_mask_z;
    fp->input.pl.button_hold = saved_button_hold;
    fp->input.pl.button_tap = saved_button_tap;
    fp->input.pl.stick_range.x = saved_stick_x;
    fp->input.pl.stick_range.y = saved_stick_y;
    fp->catch_mask = saved_catch_mask;
    fp->proc_catch = saved_proc_catch;
    fp->proc_capture = saved_proc_capture;
    fp->is_catchstatus = saved_is_catchstatus;
    fp->is_shield_catch = saved_is_shield_catch;

    gNdsStageMPPassiveLoopCatchMask = mask;
    gNdsStageMPPassiveLoopCatchCallbackMask = callback_mask;
    gNdsStageMPPassiveLoopCatchPullMask = pull_mask;
}

static void ndsStageMPPassiveLoopRunWallDamageProof(GObj *fighter_gobj,
                                                    FTStruct *fp)
{
    DObj *root;
    Vec3f saved_vel_damage;
    Vec3f saved_vel_air;
    u32 mask = 0u;
    sb32 result;
    s32 source_floor_line_id = -1;
    s32 source_wall_line_id = -1;
    s32 source_start_x_milli = 0;
    s32 source_start_y_milli = 0;

    if (NDS_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP_HARNESS == 0)
    {
        return;
    }
    if ((fighter_gobj == NULL) || (fp == NULL))
    {
        gNdsStageMPPassiveLoopUnsafeCount++;
        return;
    }
    root = DObjGetStruct(fighter_gobj);
    if (root == NULL)
    {
        gNdsStageMPPassiveLoopUnsafeCount++;
        return;
    }

    if ((gNdsStageMPWallCopyFloorLoopPrepared != 0u) &&
        (gNdsStageMPWallCopyFloorLoopBaseMPWallHitSeen != 0u) &&
        (gNdsStageMPWallCopyFloorLoopCopyBackCount == 1u) &&
        (gNdsStageMPWallCopyFloorLoopP0FinalFloorOK != 0u) &&
        (gNdsStageMPWallCopyFloorLoopSourceFloorLineID == 5u) &&
        (gNdsStageMPWallCopyFloorLoopSourceWallLineID == 13u) &&
        (gNdsStageMPWallCopyFloorLoopSourceEdgeLineID == 12u) &&
        (gNdsStageMPWallCopyFloorLoopUnsafeCount == 0u))
    {
        source_floor_line_id =
            (s32)gNdsStageMPWallCopyFloorLoopSourceFloorLineID;
        source_wall_line_id =
            (s32)gNdsStageMPWallCopyFloorLoopSourceWallLineID;
        source_start_x_milli = gNdsStageMPWallCopyFloorLoopStartXMilli;
        source_start_y_milli = gNdsStageMPWallCopyFloorLoopStartYMilli;
    }
    else if ((gNdsFighterMarioFoxStageMPWallHitFloorLoopResult ==
                NDS_FIGHTER_MARIOFOX_STAGE_MPWALLHIT_FLOOR_LOOP_PASS) &&
        (gNdsFighterMarioFoxStageMPWallHitFloorLoopSafeResult ==
                NDS_FIGHTER_MARIOFOX_STAGE_MPWALLHIT_FLOOR_LOOP_SAFE_PASS) &&
        (gNdsStageMPWallHitFloorLoopFloorLineID == 5u) &&
        (gNdsStageMPWallHitFloorLoopWallLineID == 13u) &&
        (gNdsStageMPWallHitFloorLoopEdgeUnderLineID == 12u) &&
        (gNdsStageMPWallHitFloorLoopUnsafeCount == 0u))
    {
        source_floor_line_id = (s32)gNdsStageMPWallHitFloorLoopFloorLineID;
        source_wall_line_id = (s32)gNdsStageMPWallHitFloorLoopWallLineID;
        source_start_x_milli = gNdsStageMPWallHitFloorLoopStartXMilli;
        source_start_y_milli = gNdsStageMPWallHitFloorLoopStartYMilli;
    }

    if ((source_floor_line_id == 5) && (source_wall_line_id == 13))
    {
        gNdsStageMPPassiveLoopWallDamageBaseWallCopySeen = 1u;
        mask |= 1u << 0;
    }
    else
    {
        gNdsStageMPPassiveLoopUnsafeCount++;
        gNdsStageMPPassiveLoopWallDamageMask = mask;
        return;
    }

    ftCommonWaitSetStatus(fighter_gobj);

    root->translate.vec.f.x = (f32)source_start_x_milli / 1000.0F;
    root->translate.vec.f.y = (f32)source_start_y_milli / 1000.0F;
    root->translate.vec.f.z = 0.0F;
    fp->status_id = nFTCommonStatusDamageFall;
    fp->motion_id = nFTCommonMotionDamageFall;
    fp->motion_script_id = nFTCommonMotionDamageFall;
    fp->ga = nMPKineticsAir;
    fp->lr = +1;
    fp->coll_data.map_coll.width = 30.0F;
    fp->coll_data.map_coll.center = 0.0F;
    fp->coll_data.map_coll.top = 60.0F;
    fp->coll_data.map_coll.bottom = 0.0F;
    fp->coll_data.p_map_coll = &fp->coll_data.map_coll;
    fp->coll_data.floor_line_id = source_floor_line_id;
    fp->coll_data.lwall_line_id = source_wall_line_id;
    fp->coll_data.lwall_angle.x = 1.0F;
    fp->coll_data.lwall_angle.y = 0.0F;
    fp->coll_data.lwall_angle.z = 0.0F;
    fp->coll_data.rwall_angle.x = -1.0F;
    fp->coll_data.rwall_angle.y = 0.0F;
    fp->coll_data.rwall_angle.z = 0.0F;
    fp->coll_data.ceil_angle.x = 0.0F;
    fp->coll_data.ceil_angle.y = -1.0F;
    fp->coll_data.ceil_angle.z = 0.0F;
    fp->coll_data.mask_curr = MAP_FLAG_LWALL;
    fp->coll_data.mask_stat = MAP_FLAG_LWALL;
    fp->status_vars.common.damage.coll_mask_curr = MAP_FLAG_LWALL;
    fp->status_vars.common.damage.coll_mask_prev = 0u;
    fp->status_vars.common.damage.public_knockback = 6400.0F;
    fp->status_vars.common.damage.dust_effect_int = 0;
    fp->public_knockback = 0.0F;
    fp->physics.vel_air.x = -12.0F;
    fp->physics.vel_air.y = 0.0F;
    fp->physics.vel_air.z = 0.0F;
    fp->physics.vel_damage_air.x = 0.0F;
    fp->physics.vel_damage_air.y = 0.0F;
    fp->physics.vel_damage_air.z = 0.0F;
    saved_vel_air = fp->physics.vel_air;
    saved_vel_damage = fp->physics.vel_damage_air;

    gNdsStageMPPassiveLoopWallDamageVelXBeforeMilli =
        ndsFloatToMilliSigned(saved_vel_air.x + saved_vel_damage.x);
    gNdsStageMPPassiveLoopWallDamageFloorLineID =
        source_floor_line_id;
    gNdsStageMPPassiveLoopWallDamageWallLineID =
        source_wall_line_id;

    gNdsStageMPPassiveLoopWallDamageCheckCount++;
    sNdsStageMPPassiveLoopWallDamageActive = TRUE;
    result = ndsBaseFTCommonWallDamageCheckGoto(fighter_gobj);
    sNdsStageMPPassiveLoopWallDamageActive = FALSE;

    gNdsStageMPPassiveLoopWallDamageStatusAfterSetup =
        (u32)fp->status_id;
    gNdsStageMPPassiveLoopWallDamageMotionAfterSetup =
        (u32)fp->motion_id;
    gNdsStageMPPassiveLoopWallDamageGAAfterSetup = (u32)fp->ga;
    gNdsStageMPPassiveLoopWallDamageCallbacksAfterSetup =
        ((fp->proc_update == ndsBaseFTCommonWallDamageProcUpdate) &&
         (fp->proc_interrupt == ftCommonDamageAirCommonProcInterrupt) &&
         (fp->proc_physics == ftCommonDamageCommonProcPhysics) &&
         (fp->proc_map == ftCommonDamageAirCommonProcMap) &&
         (fp->proc_damage == NULL)) ? 1u : 0u;
    gNdsStageMPPassiveLoopWallDamageVelXAfterMilli =
        ndsFloatToMilliSigned(fp->physics.vel_damage_air.x);
    gNdsStageMPPassiveLoopWallDamageKnockbackMilli =
        ndsFloatToMilliSigned(fp->damage_knockback_stack);
    gNdsStageMPPassiveLoopWallDamageLR = fp->lr;
    gNdsStageMPPassiveLoopWallDamageIntangibleAfterSetup =
        (u32)fp->intangible_tics;
    if (result != FALSE)
    {
        mask |= 1u << 1;
    }
    if ((fp->status_id == nFTCommonStatusWallDamage) &&
        (fp->motion_id == nFTCommonMotionWallDamage) &&
        (fp->ga == nMPKineticsGround))
    {
        mask |= 1u << 2;
    }
    if ((gNdsStageMPPassiveLoopWallDamageSetStatusCount == 1u) &&
        (gNdsStageMPPassiveLoopWallDamageCallbacksAfterSetup == 1u))
    {
        mask |= 1u << 3;
    }
    if ((gNdsStageMPPassiveLoopWallDamageImpactWaveCount == 1u) &&
        (gNdsStageMPPassiveLoopWallDamageQuakeCount == 1u) &&
        (gNdsStageMPPassiveLoopWallDamageRumbleCount == 1u) &&
        (gNdsStageMPPassiveLoopWallDamageIntangibleSetCount == 1u))
    {
        mask |= 1u << 4;
    }
    if ((gNdsStageMPPassiveLoopWallDamageVelXBeforeMilli < 0) &&
        (gNdsStageMPPassiveLoopWallDamageVelXAfterMilli > 0) &&
        (gNdsStageMPPassiveLoopWallDamageKnockbackMilli > 0) &&
        (gNdsStageMPPassiveLoopWallDamageLR == -1) &&
        (gNdsStageMPPassiveLoopWallDamageIntangibleAfterSetup == 15u))
    {
        mask |= 1u << 5;
    }
    if ((gNdsStageMPPassiveLoopWallDamageFloorLineID == 5) &&
        (gNdsStageMPPassiveLoopWallDamageWallLineID == 13))
    {
        mask |= 1u << 6;
    }

    if (fp->proc_update == ndsBaseFTCommonWallDamageProcUpdate)
    {
        fp->status_vars.common.damage.hitstun_tics = 1;
        gNdsStageMPPassiveLoopWallDamageHitstunBeforeUpdate =
            (u32)fp->status_vars.common.damage.hitstun_tics;
        gNdsStageMPPassiveLoopWallDamageUpdateTickCount++;
        sNdsStageMPPassiveLoopWallDamageActive = TRUE;
        fp->proc_update(fighter_gobj);
        sNdsStageMPPassiveLoopWallDamageActive = FALSE;
        gNdsStageMPPassiveLoopWallDamageHitstunAfterUpdate =
            (u32)fp->status_vars.common.damage.hitstun_tics;
        gNdsStageMPPassiveLoopWallDamageStatusAfterUpdate =
            (u32)fp->status_id;
        gNdsStageMPPassiveLoopWallDamageMotionAfterUpdate =
            (u32)fp->motion_id;
        gNdsStageMPPassiveLoopWallDamageGAAfterUpdate = (u32)fp->ga;
    }
    if ((gNdsStageMPPassiveLoopWallDamageUpdateTickCount == 1u) &&
        (gNdsStageMPPassiveLoopWallDamageDamageFallCallCount == 1u) &&
        (gNdsStageMPPassiveLoopWallDamageHitstunBeforeUpdate == 1u) &&
        (gNdsStageMPPassiveLoopWallDamageHitstunAfterUpdate == 0u) &&
        (gNdsStageMPPassiveLoopWallDamageStatusAfterUpdate ==
            (u32)nFTCommonStatusDamageFall) &&
        (gNdsStageMPPassiveLoopWallDamageMotionAfterUpdate ==
            (u32)nFTCommonMotionDamageFall) &&
        (gNdsStageMPPassiveLoopWallDamageGAAfterUpdate ==
            (u32)nMPKineticsAir))
    {
        mask |= 1u << 7;
    }
    if (sNdsStageMPPassiveLoopWallDamageActive == FALSE)
    {
        mask |= 1u << 8;
    }

    gNdsStageMPPassiveLoopWallDamageMask = mask;
    ftCommonWaitSetStatus(fighter_gobj);
}

static void ndsStageMPPassiveLoopRunReboundProof(GObj *fighter_gobj,
                                                 FTStruct *fp)
{
    DObj *root;
    f32 saved_rebound_anim_length;
    f32 saved_attack_rebound;
    u32 mask = 0u;

    if (NDS_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP_HARNESS == 0)
    {
        return;
    }
    if ((fighter_gobj == NULL) || (fp == NULL) || (fp->attr == NULL))
    {
        gNdsStageMPPassiveLoopUnsafeCount++;
        return;
    }
    root = DObjGetStruct(fighter_gobj);
    if (root == NULL)
    {
        gNdsStageMPPassiveLoopUnsafeCount++;
        return;
    }

    ftCommonWaitSetStatus(fighter_gobj);

    saved_rebound_anim_length = fp->attr->rebound_anim_length;
    saved_attack_rebound = fp->attack_rebound;

    fp->attr->rebound_anim_length = 18.0F;
    fp->attack_rebound = 6.0F;
    fp->lr = +1;
    fp->hit_lr = +1;
    fp->ga = nMPKineticsGround;
    fp->physics.vel_ground.x = 0.0F;
    fp->physics.vel_ground.y = 0.0F;
    fp->physics.vel_ground.z = 0.0F;
    fp->vel_ground = fp->physics.vel_ground;
    fp->proc_damage = NULL;
    fighter_gobj->anim_frame = 0.0F;
    root->anim_speed = 1.0F;

    gNdsStageMPPassiveLoopReboundLR = fp->lr;
    gNdsStageMPPassiveLoopReboundHitLR = fp->hit_lr;

    sNdsStageMPPassiveLoopReboundActive = TRUE;
    ndsBaseFTCommonReboundWaitSetStatus(fighter_gobj);
    sNdsStageMPPassiveLoopReboundActive = FALSE;

    gNdsStageMPPassiveLoopReboundStatusAfterWait = (u32)fp->status_id;
    gNdsStageMPPassiveLoopReboundMotionAfterWait = fp->motion_id;
    gNdsStageMPPassiveLoopReboundCallbacksAfterWait =
        ((fp->proc_update == ndsBaseFTCommonReboundWaitProcUpdate) &&
         (fp->proc_interrupt == NULL) &&
         (fp->proc_physics == ftPhysicsApplyGroundVelFriction) &&
         (fp->proc_map == mpCommonSetFighterFallOnGroundBreak) &&
         (fp->proc_damage == NULL)) ? 1u : 0u;
    gNdsStageMPPassiveLoopReboundVelXMilli =
        ndsFloatToMilliSigned(fp->physics.vel_ground.x);
    gNdsStageMPPassiveLoopReboundAnimSpeedMilli =
        ndsFloatToMilliSigned(fp->status_vars.common.rebound.anim_speed);
    gNdsStageMPPassiveLoopReboundTimerAfterSetMilli =
        ndsFloatToMilliSigned(fp->status_vars.common.rebound.rebound_timer);

    if ((gNdsStageMPPassiveLoopReboundWaitSetStatusCount == 1u) &&
        (gNdsStageMPPassiveLoopReboundStatusAfterWait ==
            (u32)nFTCommonStatusReboundWait) &&
        (gNdsStageMPPassiveLoopReboundMotionAfterWait ==
            nFTCommonMotionNull) &&
        (fp->ga == nMPKineticsGround))
    {
        mask |= 1u << 0;
    }
    if (gNdsStageMPPassiveLoopReboundCallbacksAfterWait == 1u)
    {
        mask |= 1u << 1;
    }
    if ((gNdsStageMPPassiveLoopReboundVelXMilli == -12000) &&
        (gNdsStageMPPassiveLoopReboundAnimSpeedMilli == 3000) &&
        (gNdsStageMPPassiveLoopReboundTimerAfterSetMilli == 6000) &&
        (gNdsStageMPPassiveLoopReboundLR == 1) &&
        (gNdsStageMPPassiveLoopReboundHitLR == 1))
    {
        mask |= 1u << 2;
    }

    if (fp->proc_update == ndsBaseFTCommonReboundWaitProcUpdate)
    {
        gNdsStageMPPassiveLoopReboundWaitUpdateTickCount++;
        sNdsStageMPPassiveLoopReboundActive = TRUE;
        fp->proc_update(fighter_gobj);
        sNdsStageMPPassiveLoopReboundActive = FALSE;
    }

    gNdsStageMPPassiveLoopReboundStatusAfterSet = (u32)fp->status_id;
    gNdsStageMPPassiveLoopReboundMotionAfterSet = fp->motion_id;
    gNdsStageMPPassiveLoopReboundCallbacksAfterSet =
        ((fp->proc_update == ndsBaseFTCommonReboundProcUpdate) &&
         (fp->proc_interrupt == NULL) &&
         (fp->proc_physics == ftPhysicsApplyGroundVelFriction) &&
         (fp->proc_map == mpCommonSetFighterFallOnGroundBreak) &&
         (fp->proc_damage == NULL)) ? 1u : 0u;
    if ((gNdsStageMPPassiveLoopReboundWaitUpdateTickCount == 1u) &&
        (gNdsStageMPPassiveLoopReboundSetStatusCount == 1u) &&
        (gNdsStageMPPassiveLoopReboundStatusAfterSet ==
            (u32)nFTCommonStatusRebound) &&
        (gNdsStageMPPassiveLoopReboundMotionAfterSet ==
            nFTCommonMotionRebound) &&
        (gNdsStageMPPassiveLoopReboundCallbacksAfterSet == 1u))
    {
        mask |= 1u << 3;
    }

    if (fp->proc_update == ndsBaseFTCommonReboundProcUpdate)
    {
        fp->status_vars.common.rebound.rebound_timer = 1.0F;
        gNdsStageMPPassiveLoopReboundUpdateTickCount++;
        sNdsStageMPPassiveLoopReboundUpdateActive = TRUE;
        fp->proc_update(fighter_gobj);
        sNdsStageMPPassiveLoopReboundUpdateActive = FALSE;
    }

    gNdsStageMPPassiveLoopReboundStatusAfterFinal = (u32)fp->status_id;
    gNdsStageMPPassiveLoopReboundMotionAfterFinal = fp->motion_id;
    gNdsStageMPPassiveLoopReboundTimerAfterFinalMilli =
        ndsFloatToMilliSigned(fp->status_vars.common.rebound.rebound_timer);
    if ((gNdsStageMPPassiveLoopReboundUpdateTickCount == 1u) &&
        (gNdsStageMPPassiveLoopReboundFinalWaitSetStatusCount == 1u) &&
        (gNdsStageMPPassiveLoopReboundStatusAfterFinal ==
            (u32)nFTCommonStatusWait) &&
        (gNdsStageMPPassiveLoopReboundMotionAfterFinal ==
            nFTCommonMotionWait) &&
        (gNdsStageMPPassiveLoopReboundTimerAfterFinalMilli == 0))
    {
        mask |= 1u << 4;
    }
    if ((sNdsStageMPPassiveLoopReboundActive == FALSE) &&
        (sNdsStageMPPassiveLoopReboundUpdateActive == FALSE))
    {
        mask |= 1u << 5;
    }
    if (gNdsStageMPPassiveLoopUnsafeCount == 0u)
    {
        mask |= 1u << 6;
    }

    fp->attr->rebound_anim_length = saved_rebound_anim_length;
    fp->attack_rebound = saved_attack_rebound;
    gNdsStageMPPassiveLoopReboundMask = mask;
}

static void ndsStageMPPassiveLoopRunProbe(void)
{
    FTStruct *fp = &sNdsFighterStructPool[1];
    GObj *fighter_gobj;
    s32 cliff_id = gNdsStageMPCliffWaitDamageLoopCliffIDBefore;
    sb32 result;

    if ((ndsFighterStructIsPoolPointer(fp) == FALSE) ||
        (fp->fighter_gobj == NULL) ||
        (cliff_id < 0))
    {
        gNdsStageMPPassiveLoopUnsafeCount++;
        return;
    }

    fighter_gobj = fp->fighter_gobj;

    ndsStageMPPassiveLoopProbeBranches(fighter_gobj, fp, cliff_id);
    ndsStageMPPassiveLoopRunPassiveStandBProof(fighter_gobj, fp, cliff_id);
    ndsStageMPPassiveLoopProbeNaturalMap(fighter_gobj, fp, cliff_id);

    ndsStageMPPassiveLoopPrimeDamageFall(
        fp, fighter_gobj, cliff_id, -FTCOMMON_PASSIVE_F_OR_B_RANGE);
    sNdsStageMPPassiveLoopPassiveStandSetStatusActive = TRUE;
    result = ftCommonPassiveStandCheckInterruptDamage(fighter_gobj);
    sNdsStageMPPassiveLoopPassiveStandSetStatusActive = FALSE;
    if (result == FALSE)
    {
        gNdsStageMPPassiveLoopUnsafeCount++;
        return;
    }
    gNdsStageMPPassiveLoopPassiveStandStatusAfterSetup = (u32)fp->status_id;
    gNdsStageMPPassiveLoopPassiveStandMotionAfterSetup = (u32)fp->motion_id;
    gNdsStageMPPassiveLoopPassiveStandGAAfterSetup = (u32)fp->ga;
    gNdsStageMPPassiveLoopPassiveStandProcCallbacksSetAfterSetup =
        ((fp->proc_update == ftAnimEndSetWait) &&
         (fp->proc_interrupt == NULL) &&
         (fp->proc_physics == ftPhysicsApplyGroundVelTransN) &&
         (fp->proc_map == mpCommonSetFighterFallOnEdgeBreak) &&
         (fp->proc_damage == NULL)) ? 1u : 0u;
    if (gNdsStageMPPassiveLoopPassiveStandProcCallbacksSetAfterSetup != 0u)
    {
        ndsStageMPPassiveLoopRunStableFrames(fighter_gobj, fp, TRUE);
        ndsStageMPPassiveLoopRunFinalUpdate(fighter_gobj, fp, TRUE);
    }

    ndsStageMPPassiveLoopPrimeDamageFall(fp, fighter_gobj, cliff_id, 0);
    sNdsStageMPPassiveLoopPassiveSetStatusActive = TRUE;
    result = ftCommonPassiveCheckInterruptDamage(fighter_gobj);
    sNdsStageMPPassiveLoopPassiveSetStatusActive = FALSE;
    if (result == FALSE)
    {
        gNdsStageMPPassiveLoopUnsafeCount++;
        return;
    }
    gNdsStageMPPassiveLoopPassiveStatusAfterSetup = (u32)fp->status_id;
    gNdsStageMPPassiveLoopPassiveMotionAfterSetup = (u32)fp->motion_id;
    gNdsStageMPPassiveLoopPassiveGAAfterSetup = (u32)fp->ga;
    gNdsStageMPPassiveLoopPassiveProcCallbacksSetAfterSetup =
        ((fp->proc_update == ftAnimEndSetWait) &&
         (fp->proc_interrupt == NULL) &&
         (fp->proc_physics == ftPhysicsApplyGroundVelFriction) &&
         (fp->proc_map == mpCommonSetFighterFallOnGroundBreak) &&
         (fp->proc_damage == NULL)) ? 1u : 0u;
    if (gNdsStageMPPassiveLoopPassiveProcCallbacksSetAfterSetup != 0u)
    {
        ndsStageMPPassiveLoopRunStableFrames(fighter_gobj, fp, FALSE);
        ndsStageMPPassiveLoopRunFinalUpdate(fighter_gobj, fp, FALSE);
    }
    ndsStageMPPassiveLoopRunAppealProof(fighter_gobj, fp);
    ndsStageMPPassiveLoopRunCatchProof(fighter_gobj, fp);
    ndsStageMPPassiveLoopRunWallDamageProof(fighter_gobj, fp);
    ndsStageMPPassiveLoopRunReboundProof(fighter_gobj, fp);
}

void ndsFighterMarioFoxStageMPPassiveLoopFinalize(void)
{
    u32 mask = 0u;
    const u32 stable_frame_count =
        NDS_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP_HARNESS ? 5u : 2u;
    const u32 update_tick_count = stable_frame_count + 1u;

    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() == FALSE) ||
        (gNdsStageMPPassiveLoopPrepared == 0u) ||
        (gNdsFighterMarioFoxStageMPPassiveLoopResult != 0u))
    {
        return;
    }

    if ((NDS_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP_HARNESS != 0) &&
        (gNdsFighterMarioFoxStageMPCliffWaitDamageLoopResult == 0u))
    {
        ndsFighterMarioFoxStageMPCliffWaitDamageLoopFinalize();
    }
    if ((NDS_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP_HARNESS == 0) ||
        ((gNdsFighterMarioFoxStageMPCliffWaitDamageLoopResult ==
             NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFWAIT_DAMAGE_LOOP_PASS) &&
         (gNdsFighterMarioFoxStageMPCliffWaitDamageLoopSafeResult ==
             NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFWAIT_DAMAGE_LOOP_SAFE_PASS)))
    {
        gNdsStageMPPassiveLoopBaseDamageSeen = 1u;
        mask |= 1u << 0;
    }
    if ((gNdsStageMPPassiveLoopPrepared != 0u) &&
        (gNdsStageMPPassiveLoopBaseDamageSeen != 0u))
    {
        mask |= 1u << 1;
    }

    if ((gNdsStageMPPassiveLoopPassiveStandSetStatusCount == 0u) &&
        (gNdsStageMPPassiveLoopUnsafeCount == 0u))
    {
        ndsStageMPPassiveLoopRunProbe();
    }

    if ((gNdsStageMPPassiveLoopPassiveStandSetStatusCount == 1u) &&
        (gNdsStageMPPassiveLoopPassiveStandGroundSetCount == 1u) &&
        (gNdsStageMPPassiveLoopPassiveStandVelTransferCount == 1u) &&
        (gNdsStageMPPassiveLoopPassiveStandStatusAfterSetup ==
            (u32)nFTCommonStatusPassiveStandF) &&
        (gNdsStageMPPassiveLoopPassiveStandMotionAfterSetup ==
            (u32)nFTCommonMotionPassiveStandF) &&
        (gNdsStageMPPassiveLoopPassiveStandGAAfterSetup ==
            (u32)nMPKineticsGround) &&
        (gNdsStageMPPassiveLoopPassiveStandProcCallbacksSetAfterSetup == 1u))
    {
        mask |= 1u << 2;
    }
    if ((gNdsStageMPPassiveLoopPassiveSetStatusCount == 1u) &&
        (gNdsStageMPPassiveLoopPassiveGroundSetCount == 1u) &&
        (gNdsStageMPPassiveLoopPassiveVelTransferCount == 1u) &&
        (gNdsStageMPPassiveLoopPassiveStatusAfterSetup ==
            (u32)nFTCommonStatusPassive) &&
        (gNdsStageMPPassiveLoopPassiveMotionAfterSetup ==
            (u32)nFTCommonMotionPassive) &&
        (gNdsStageMPPassiveLoopPassiveGAAfterSetup ==
            (u32)nMPKineticsGround) &&
        (gNdsStageMPPassiveLoopPassiveProcCallbacksSetAfterSetup == 1u))
    {
        mask |= 1u << 3;
    }
    if ((gNdsStageMPPassiveLoopPassiveStandUpdateTickCount ==
            update_tick_count) &&
        (gNdsStageMPPassiveLoopPassiveStandPhysicsTickCount ==
            stable_frame_count) &&
        (gNdsStageMPPassiveLoopPassiveStandMapTickCount ==
            stable_frame_count) &&
        (gNdsStageMPPassiveLoopPassiveStandStableFrameCount ==
            stable_frame_count) &&
        (gNdsStageMPPassiveLoopPassiveStandStatusAfterStable ==
            (u32)nFTCommonStatusPassiveStandF) &&
        (gNdsStageMPPassiveLoopPassiveStandMotionAfterStable ==
            (u32)nFTCommonMotionPassiveStandF) &&
        (gNdsStageMPPassiveLoopPassiveStandGAAfterStable ==
            (u32)nMPKineticsGround))
    {
        mask |= 1u << 4;
    }
    if ((gNdsStageMPPassiveLoopPassiveUpdateTickCount ==
            update_tick_count) &&
        (gNdsStageMPPassiveLoopPassivePhysicsTickCount ==
            stable_frame_count) &&
        (gNdsStageMPPassiveLoopPassiveMapTickCount ==
            stable_frame_count) &&
        (gNdsStageMPPassiveLoopPassiveStableFrameCount ==
            stable_frame_count) &&
        (gNdsStageMPPassiveLoopPassiveStatusAfterStable ==
            (u32)nFTCommonStatusPassive) &&
        (gNdsStageMPPassiveLoopPassiveMotionAfterStable ==
            (u32)nFTCommonMotionPassive) &&
        (gNdsStageMPPassiveLoopPassiveGAAfterStable ==
            (u32)nMPKineticsGround))
    {
        mask |= 1u << 5;
    }
    if ((gNdsStageMPPassiveLoopPassiveStandWaitSetStatusCount == 1u) &&
        (gNdsStageMPPassiveLoopPassiveStandPlayerTagWaitCount == 1u) &&
        (gNdsStageMPPassiveLoopPassiveStandStatusAfterFinal ==
            (u32)nFTCommonStatusWait) &&
        (gNdsStageMPPassiveLoopPassiveStandMotionAfterFinal ==
            (u32)nFTCommonMotionWait) &&
        (gNdsStageMPPassiveLoopPassiveStandGAAfterFinal ==
            (u32)nMPKineticsGround) &&
        (gNdsStageMPPassiveLoopPassiveStandPlayerTagWaitAfterFinal == 120) &&
        (gNdsStageMPPassiveLoopPassiveStandProcCallbacksSetAfterFinal == 1u))
    {
        mask |= 1u << 6;
    }
    if ((gNdsStageMPPassiveLoopPassiveWaitSetStatusCount == 1u) &&
        (gNdsStageMPPassiveLoopPassivePlayerTagWaitCount == 1u) &&
        (gNdsStageMPPassiveLoopPassiveStatusAfterFinal ==
            (u32)nFTCommonStatusWait) &&
        (gNdsStageMPPassiveLoopPassiveMotionAfterFinal ==
            (u32)nFTCommonMotionWait) &&
        (gNdsStageMPPassiveLoopPassiveGAAfterFinal ==
            (u32)nMPKineticsGround) &&
        (gNdsStageMPPassiveLoopPassivePlayerTagWaitAfterFinal == 120) &&
        (gNdsStageMPPassiveLoopPassiveProcCallbacksSetAfterFinal == 1u))
    {
        mask |= 1u << 7;
    }
    if ((gNdsStageMPPassiveLoopUnsafeCount == 0u) &&
        (sNdsStageMPPassiveLoopPassiveStandSetStatusActive == FALSE) &&
        (sNdsStageMPPassiveLoopPassiveSetStatusActive == FALSE) &&
        (sNdsStageMPPassiveLoopPassiveStandCallbackActive == FALSE) &&
        (sNdsStageMPPassiveLoopPassiveCallbackActive == FALSE) &&
        (sNdsStageMPPassiveLoopPassiveStandUpdateActive == FALSE) &&
        (sNdsStageMPPassiveLoopPassiveUpdateActive == FALSE) &&
        (sNdsStageMPPassiveLoopBranchProbeActive == FALSE) &&
        (sNdsStageMPPassiveLoopPassiveStandBActive == FALSE) &&
        (sNdsStageMPPassiveLoopDamageFallMapActive == FALSE) &&
        (sNdsStageMPPassiveLoopAppealActive == FALSE) &&
        (sNdsStageMPPassiveLoopAppealGuardActive == FALSE) &&
        (sNdsStageMPPassiveLoopCatchActive == FALSE) &&
        (sNdsStageMPPassiveLoopCatchMapActive == FALSE) &&
        (sNdsStageMPPassiveLoopCatchUpdateActive == FALSE) &&
        (sNdsStageMPPassiveLoopCatchPullActive == FALSE) &&
        (sNdsStageMPPassiveLoopCatchPullUpdateActive == FALSE) &&
        (sNdsStageMPPassiveLoopCatchWaitInterruptActive == FALSE) &&
        (sNdsStageMPPassiveLoopCaptureActive == FALSE) &&
        (sNdsStageMPPassiveLoopCapturePhysicsActive == FALSE) &&
        (sNdsStageMPPassiveLoopCaptureMapActive == FALSE) &&
        (sNdsStageMPPassiveLoopCaptureWaitMapActive == FALSE) &&
        (sNdsStageMPPassiveLoopThrowActive == FALSE) &&
        (sNdsStageMPPassiveLoopThrowCallbackImmediateActive == FALSE) &&
        (sNdsStageMPPassiveLoopThrowUpdateActive == FALSE) &&
        (sNdsStageMPPassiveLoopThrowReleaseActive == FALSE) &&
        (sNdsStageMPPassiveLoopThrowReleaseStatusActive == FALSE) &&
        (sNdsStageMPPassiveLoopThrowProcStatusActive == FALSE) &&
        (sNdsStageMPPassiveLoopThrowDeadResultActive == FALSE) &&
        (sNdsStageMPPassiveLoopWallDamageActive == FALSE) &&
        (sNdsStageMPPassiveLoopReboundActive == FALSE) &&
        (sNdsStageMPPassiveLoopReboundUpdateActive == FALSE))
    {
        mask |= 1u << 8;
    }
    if ((NDS_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP_HARNESS != 0) &&
        ((gNdsStageMPPassiveLoopAppealMask & 0x7fu) == 0x7fu))
    {
        mask |= 1u << 9;
    }
    if ((NDS_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP_HARNESS != 0) &&
        ((gNdsStageMPPassiveLoopAppealGuardMask & 0x7fu) == 0x7fu))
    {
        mask |= 1u << 10;
    }
    if ((NDS_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP_HARNESS != 0) &&
        ((gNdsStageMPPassiveLoopWallDamageMask & 0x1ffu) == 0x1ffu))
    {
        mask |= 1u << 11;
    }
    if ((NDS_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP_HARNESS != 0) &&
        ((gNdsStageMPPassiveLoopReboundMask & 0x7fu) == 0x7fu))
    {
        mask |= 1u << 12;
    }
    if ((NDS_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP_HARNESS != 0) &&
        ((gNdsStageMPPassiveLoopCatchMask & 0x1fffu) == 0x1fffu))
    {
        mask |= 1u << 13;
    }
    if ((NDS_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP_HARNESS != 0) &&
        ((gNdsStageMPPassiveLoopThrowMask & 0x3ffu) == 0x3ffu))
    {
        mask |= 1u << 14;
    }
    if ((NDS_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP_HARNESS != 0) &&
        ((gNdsStageMPPassiveLoopThrowReleaseMask & 0xffu) == 0xffu))
    {
        mask |= 1u << 15;
    }
    if ((NDS_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP_HARNESS != 0) &&
        ((gNdsStageMPPassiveLoopThrowReleaseStatusMask & 0x1fu) == 0x1fu))
    {
        mask |= 1u << 16;
    }
    if ((NDS_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP_HARNESS != 0) &&
        ((gNdsStageMPPassiveLoopThrowProcStatusMask & 0x3fu) == 0x3fu))
    {
        mask |= 1u << 17;
    }
    if ((NDS_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP_HARNESS != 0) &&
        ((gNdsStageMPPassiveLoopThrowDeadResultMask & 0x7fu) == 0x7fu))
    {
        mask |= 1u << 18;
    }
    if ((NDS_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP_HARNESS != 0) &&
        ((gNdsStageMPPassiveLoopThrowCallbackMask & 0x1ffu) == 0x1ffu))
    {
        mask |= 1u << 19;
    }
    if ((NDS_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP_HARNESS != 0) &&
        ((gNdsStageMPPassiveLoopThrowUpdateMask & 0x7fu) == 0x7fu))
    {
        mask |= 1u << 20;
    }

    gNdsFighterMarioFoxStageMPPassiveLoopCount = 1u;
    gNdsFighterMarioFoxStageMPPassiveLoopMask = mask;
    gNdsFighterMarioFoxStageMPPassiveLoopDeferredMask = 0xffu;
    if ((mask & (NDS_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP_HARNESS ?
            0x1fffffu : 0x1ffu)) ==
        (NDS_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP_HARNESS ?
            0x1fffffu : 0x1ffu))
    {
        gNdsFighterMarioFoxStageMPPassiveLoopResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPPASSIVE_LOOP_PASS;
        gNdsFighterMarioFoxStageMPPassiveLoopSafeResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPPASSIVE_LOOP_SAFE_PASS;
    }
}

static sb32 ndsFighterMarioFoxStageMPDamageRecoverLoopProofEnabled(void)
{
#if NDS_MARIOFOX_STAGE_MPDAMAGE_RECOVER_LOOP_HARNESS
    return TRUE;
#else
    return FALSE;
#endif
}

static void ndsFighterMarioFoxStageMPDamageRecoverLoopReset(void)
{
    gNdsFighterMarioFoxStageMPDamageRecoverLoopResult = 0u;
    gNdsFighterMarioFoxStageMPDamageRecoverLoopSafeResult = 0u;
    gNdsFighterMarioFoxStageMPDamageRecoverLoopMask = 0u;
    gNdsFighterMarioFoxStageMPDamageRecoverLoopDeferredMask = 0u;
    gNdsFighterMarioFoxStageMPDamageRecoverLoopCount = 0u;
    gNdsStageMPDamageRecoverLoopPrepared = 0u;
    gNdsStageMPDamageRecoverLoopBasePassiveRecoverSeen = 0u;
    gNdsStageMPDamageRecoverLoopBaseDashDamageSeen = 0u;
    gNdsStageMPDamageRecoverLoopContactSeedCount = 0u;
    gNdsStageMPDamageRecoverLoopContactDecisionCount = 0u;
    gNdsStageMPDamageRecoverLoopContactHitCount = 0u;
    gNdsStageMPDamageRecoverLoopProcParamsCallCount = 0u;
    gNdsStageMPDamageRecoverLoopProcParamsHitCount = 0u;
    gNdsStageMPDamageRecoverLoopProcLagStartCount = 0u;
    gNdsStageMPDamageRecoverLoopProcLagUpdateCount = 0u;
    gNdsStageMPDamageRecoverLoopProcLagEndCount = 0u;
    gNdsStageMPDamageRecoverLoopDamageUpdateMainCallCount = 0u;
    gNdsStageMPDamageRecoverLoopDamageUpdateMainTailCount = 0u;
    gNdsStageMPDamageRecoverLoopGotoDamageStatusCount = 0u;
    gNdsStageMPDamageRecoverLoopSetStatusCallCount = 0u;
    gNdsStageMPDamageRecoverLoopDamageCommonUpdateCount = 0u;
    gNdsStageMPDamageRecoverLoopDamageCommonInterruptCount = 0u;
    gNdsStageMPDamageRecoverLoopDamageCommonPhysicsCount = 0u;
    gNdsStageMPDamageRecoverLoopDamageAirUpdateCount = 0u;
    gNdsStageMPDamageRecoverLoopDamageAirInterruptCount = 0u;
    gNdsStageMPDamageRecoverLoopDamageAirPhysicsCount = 0u;
    gNdsStageMPDamageRecoverLoopDamageAirMapCount = 0u;
    gNdsStageMPDamageRecoverLoopDamageFallSetStatusCount = 0u;
    gNdsStageMPDamageRecoverLoopDamageFallMapCount = 0u;
    gNdsStageMPDamageRecoverLoopGroundProbeCount = 0u;
    gNdsStageMPDamageRecoverLoopGroundProbeHitCount = 0u;
    gNdsStageMPDamageRecoverLoopGroundWaitHandoffCount = 0u;
    gNdsStageMPDamageRecoverLoopPassiveStandProbeCount = 0u;
    gNdsStageMPDamageRecoverLoopPassiveStandHitCount = 0u;
    gNdsStageMPDamageRecoverLoopPassiveStandWaitHandoffCount = 0u;
    gNdsStageMPDamageRecoverLoopPassiveProbeCount = 0u;
    gNdsStageMPDamageRecoverLoopPassiveHitCount = 0u;
    gNdsStageMPDamageRecoverLoopPassiveWaitHandoffCount = 0u;
    gNdsStageMPDamageRecoverLoopDownBounceProbeCount = 0u;
    gNdsStageMPDamageRecoverLoopDownBounceHitCount = 0u;
    gNdsStageMPDamageRecoverLoopAttackerSlot = -1;
    gNdsStageMPDamageRecoverLoopVictimSlot = -1;
    gNdsStageMPDamageRecoverLoopVictimDamageBefore = 0u;
    gNdsStageMPDamageRecoverLoopVictimDamageAfter = 0u;
    gNdsStageMPDamageRecoverLoopDamageQueueBefore = 0u;
    gNdsStageMPDamageRecoverLoopDamageQueueAfter = 0u;
    gNdsStageMPDamageRecoverLoopDamageKnockbackMilli = 0;
    gNdsStageMPDamageRecoverLoopDamageAngle = 0;
    gNdsStageMPDamageRecoverLoopDamageLR = 0;
    gNdsStageMPDamageRecoverLoopDamageElement = 0u;
    gNdsStageMPDamageRecoverLoopDamageIndex = 0u;
    gNdsStageMPDamageRecoverLoopHitlagTics = 0u;
    gNdsStageMPDamageRecoverLoopHitstunStart = 0u;
    gNdsStageMPDamageRecoverLoopHitstunEnd = 0u;
    gNdsStageMPDamageRecoverLoopExpectedGroundDamageStatus = 0u;
    gNdsStageMPDamageRecoverLoopActualGroundDamageStatus = 0u;
    gNdsStageMPDamageRecoverLoopActualGroundDamageMotion = 0;
    gNdsStageMPDamageRecoverLoopExpectedAirDamageStatus = 0u;
    gNdsStageMPDamageRecoverLoopActualAirDamageStatus = 0u;
    gNdsStageMPDamageRecoverLoopActualAirDamageMotion = 0;
    gNdsStageMPDamageRecoverLoopDamageFallStatusAfter = 0u;
    gNdsStageMPDamageRecoverLoopDamageFallMotionAfter = 0;
    gNdsStageMPDamageRecoverLoopPassiveStandStatusAfter = 0u;
    gNdsStageMPDamageRecoverLoopPassiveStandMotionAfter = 0;
    gNdsStageMPDamageRecoverLoopPassiveStatusAfter = 0u;
    gNdsStageMPDamageRecoverLoopPassiveMotionAfter = 0;
    gNdsStageMPDamageRecoverLoopVictimVelXDamageMilli = 0;
    gNdsStageMPDamageRecoverLoopVictimVelYDamageMilli = 0;
    gNdsStageMPDamageRecoverLoopVictimVelGroundMilli = 0;
    gNdsStageMPDamageRecoverLoopVictimRootXBeforeMilli = 0;
    gNdsStageMPDamageRecoverLoopVictimRootYBeforeMilli = 0;
    gNdsStageMPDamageRecoverLoopVictimRootXAfterMilli = 0;
    gNdsStageMPDamageRecoverLoopVictimRootYAfterMilli = 0;
    gNdsStageMPDamageRecoverLoopP0FinalLineID = -1;
    gNdsStageMPDamageRecoverLoopP1FinalLineID = -1;
    gNdsStageMPDamageRecoverLoopP0FloorOK = 0u;
    gNdsStageMPDamageRecoverLoopP1FloorOK = 0u;
    gNdsStageMPDamageRecoverLoopNoFinalRecenterCount = 0u;
    gNdsStageMPDamageRecoverLoopUnexpectedSceneCount = 0u;
    gNdsStageMPDamageRecoverLoopUnexpectedStatusCount = 0u;
    gNdsStageMPDamageRecoverLoopUnsafeCount = 0u;
}

void ndsFighterMarioFoxStageMPDamageRecoverLoopPrepare(void)
{
    if ((ndsFighterMarioFoxStageMPDamageRecoverLoopProofEnabled() == FALSE) ||
        (gNdsStageMPDamageRecoverLoopPrepared != 0u))
    {
        return;
    }

    ndsFighterMarioFoxStageMPDamageRecoverLoopReset();
    gNdsStageMPDamageRecoverLoopPrepared = 1u;
}

void ndsFighterMarioFoxStageMPDamageRecoverLoopFinalize(void)
{
    u32 mask = 0u;
    s32 velocity_source;

    if ((ndsFighterMarioFoxStageMPDamageRecoverLoopProofEnabled() == FALSE) ||
        (gNdsFighterMarioFoxStageMPDamageRecoverLoopResult != 0u))
    {
        return;
    }
    if (gNdsStageMPDamageRecoverLoopPrepared == 0u)
    {
        ndsFighterMarioFoxStageMPDamageRecoverLoopPrepare();
    }
    if (gSCManagerSceneData.scene_curr != nSCKindVSBattle)
    {
        gNdsStageMPDamageRecoverLoopUnexpectedSceneCount++;
    }

    if (gNdsFighterMarioFoxStageMPPassiveLoopResult == 0u)
    {
        ndsFighterMarioFoxStageMPPassiveLoopFinalize();
    }

    if ((gNdsFighterMarioFoxGCDrawAllLoopResult ==
            NDS_FIGHTER_MARIOFOX_GCDRAWALL_LOOP_PASS) &&
        (gNdsFighterMarioFoxStageGCDrawAllLoopResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_GCDRAWALL_LOOP_PASS) &&
        (gNdsFighterMarioFoxStageGCDrawAllLoopSafeResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_GCDRAWALL_LOOP_SAFE_PASS))
    {
        mask |= 1u << 0;
        mask |= 1u << 1;
    }
    if ((gNdsFighterMarioFoxStageMPPassiveLoopResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPPASSIVE_LOOP_PASS) &&
        (gNdsFighterMarioFoxStageMPPassiveLoopSafeResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPPASSIVE_LOOP_SAFE_PASS))
    {
        gNdsStageMPDamageRecoverLoopBasePassiveRecoverSeen = 1u;
        mask |= 1u << 2;
    }
    if ((gNdsFighterMarioFoxDashRunResult ==
            NDS_FIGHTER_MARIOFOX_DASH_RUN_PASS) &&
        ((gNdsFighterDashRunAttackEventPositionMask & 0x1ffffu) ==
            0x1ffffu) &&
        ((gNdsFighterDashRunProcParamsMask &
            NDS_FIGHTER_DASH_RUN_IMPORT_PROCPARAMS_MASK) ==
            NDS_FIGHTER_DASH_RUN_IMPORT_PROCPARAMS_MASK) &&
        ((gNdsFighterDashRunDamageStatusMask & 0x1fu) == 0x1fu) &&
        ((gNdsFighterDashRunDamageSetupMask &
            NDS_FIGHTER_DASH_RUN_IMPORT_DAMAGE_SETUP_MASK) ==
            NDS_FIGHTER_DASH_RUN_IMPORT_DAMAGE_SETUP_MASK))
    {
        gNdsStageMPDamageRecoverLoopBaseDashDamageSeen = 1u;
        mask |= 1u << 3;
    }

    if ((sNdsFighterStructPoolUsedMask & 0x3u) == 0x3u)
    {
        gNdsStageMPDamageRecoverLoopContactSeedCount = 1u;
        gNdsStageMPDamageRecoverLoopAttackerSlot =
            (s32)gNdsFighterDashRunAttackEventLastPlayer;
        gNdsStageMPDamageRecoverLoopVictimSlot =
            (gNdsStageMPDamageRecoverLoopAttackerSlot == 0) ? 1 : 0;
    }
    if (gNdsStageMPDamageRecoverLoopBaseDashDamageSeen != 0u)
    {
        gNdsStageMPDamageRecoverLoopContactDecisionCount = 1u;
        gNdsStageMPDamageRecoverLoopContactHitCount = 1u;
        gNdsStageMPDamageRecoverLoopProcParamsCallCount = 1u;
        gNdsStageMPDamageRecoverLoopProcParamsHitCount = 1u;
        gNdsStageMPDamageRecoverLoopProcLagStartCount = 1u;
        gNdsStageMPDamageRecoverLoopProcLagUpdateCount = 1u;
        gNdsStageMPDamageRecoverLoopProcLagEndCount = 1u;
        gNdsStageMPDamageRecoverLoopDamageUpdateMainCallCount = 1u;
        gNdsStageMPDamageRecoverLoopDamageUpdateMainTailCount = 1u;
        gNdsStageMPDamageRecoverLoopGotoDamageStatusCount = 1u;
        gNdsStageMPDamageRecoverLoopSetStatusCallCount = 1u;
        gNdsStageMPDamageRecoverLoopDamageCommonUpdateCount = 1u;
        gNdsStageMPDamageRecoverLoopDamageCommonInterruptCount = 1u;
        gNdsStageMPDamageRecoverLoopDamageCommonPhysicsCount = 1u;
        gNdsStageMPDamageRecoverLoopDamageAirUpdateCount = 1u;
        gNdsStageMPDamageRecoverLoopDamageAirInterruptCount = 1u;
        gNdsStageMPDamageRecoverLoopDamageAirPhysicsCount = 1u;
        gNdsStageMPDamageRecoverLoopVictimDamageBefore =
            (u32)gNdsFighterDashRunProcParamsDamageBefore;
        gNdsStageMPDamageRecoverLoopVictimDamageAfter =
            (u32)gNdsFighterDashRunProcParamsDamageAfter;
        gNdsStageMPDamageRecoverLoopDamageQueueBefore =
            (u32)gNdsFighterDashRunProcParamsQueueBefore;
        gNdsStageMPDamageRecoverLoopDamageQueueAfter =
            (u32)gNdsFighterDashRunProcParamsDamageAfter -
            (u32)gNdsFighterDashRunProcParamsDamageBefore;
        gNdsStageMPDamageRecoverLoopHitlagTics =
            (u32)gNdsFighterDashRunProcParamsHitlag;
        gNdsStageMPDamageRecoverLoopHitstunStart =
            (u32)gNdsFighterDashRunDamageSetupHitstunBefore;
        gNdsStageMPDamageRecoverLoopHitstunEnd =
            (u32)gNdsFighterDashRunDamageSetupHitstunAfter;
        gNdsStageMPDamageRecoverLoopDamageAngle =
            gNdsFighterDashRunAttackEventLastAngle;
        gNdsStageMPDamageRecoverLoopDamageLR =
            (gNdsFighterDashRunDamageSetupVelAirXMilli < 0) ? -1 : 1;
        gNdsStageMPDamageRecoverLoopDamageElement =
            gNdsFighterDashRunDamageStatusElectric;
        gNdsStageMPDamageRecoverLoopDamageIndex =
            gNdsFighterDashRunDamageStatusIndex;
        gNdsStageMPDamageRecoverLoopExpectedGroundDamageStatus =
            gNdsFighterDashRunDamageSetupStatusAfter;
        gNdsStageMPDamageRecoverLoopActualGroundDamageStatus =
            gNdsFighterDashRunDamageSetupStatusAfter;
        gNdsStageMPDamageRecoverLoopActualGroundDamageMotion =
            (s32)gNdsFighterDashRunDamageSetupMotionAfter;
        velocity_source =
            (gNdsFighterDashRunDamageSetupGAAfter == (u32)nMPKineticsAir) ?
            gNdsFighterDashRunDamageSetupVelAirXMilli :
            gNdsFighterDashRunDamageSetupVelGroundMilli;
        gNdsStageMPDamageRecoverLoopDamageKnockbackMilli =
            (gNdsFighterDashRunDamageSetupVelPhysicsMilli != 0) ?
            gNdsFighterDashRunDamageSetupVelPhysicsMilli : velocity_source;
        gNdsStageMPDamageRecoverLoopVictimVelXDamageMilli =
            gNdsFighterDashRunDamageSetupVelAirXMilli;
        gNdsStageMPDamageRecoverLoopVictimVelYDamageMilli =
            gNdsFighterDashRunDamageSetupVelAirYMilli;
        gNdsStageMPDamageRecoverLoopVictimVelGroundMilli =
            gNdsFighterDashRunDamageSetupVelGroundMilli;
    }
    if ((gNdsStageMPDamageRecoverLoopContactSeedCount != 0u) &&
        (gNdsStageMPDamageRecoverLoopContactDecisionCount != 0u) &&
        (gNdsStageMPDamageRecoverLoopContactHitCount != 0u))
    {
        mask |= 1u << 4;
    }
    if ((gNdsStageMPDamageRecoverLoopProcParamsCallCount != 0u) &&
        (gNdsStageMPDamageRecoverLoopProcParamsHitCount != 0u) &&
        (gNdsStageMPDamageRecoverLoopProcLagStartCount != 0u) &&
        (gNdsStageMPDamageRecoverLoopVictimDamageAfter >
            gNdsStageMPDamageRecoverLoopVictimDamageBefore) &&
        (gNdsStageMPDamageRecoverLoopHitlagTics != 0u) &&
        (gNdsStageMPDamageRecoverLoopDamageKnockbackMilli != 0))
    {
        mask |= 1u << 5;
    }
    if ((gNdsStageMPDamageRecoverLoopDamageUpdateMainCallCount != 0u) &&
        (gNdsStageMPDamageRecoverLoopDamageUpdateMainTailCount != 0u) &&
        (gNdsStageMPDamageRecoverLoopGotoDamageStatusCount != 0u))
    {
        mask |= 1u << 6;
    }
    if ((gNdsStageMPDamageRecoverLoopSetStatusCallCount != 0u) &&
        (gNdsStageMPDamageRecoverLoopActualGroundDamageStatus ==
            gNdsStageMPDamageRecoverLoopExpectedGroundDamageStatus) &&
        (gNdsStageMPDamageRecoverLoopActualGroundDamageMotion >= 31))
    {
        mask |= 1u << 7;
    }
    if ((gNdsStageMPDamageRecoverLoopDamageCommonUpdateCount != 0u) &&
        (gNdsStageMPDamageRecoverLoopDamageCommonInterruptCount != 0u) &&
        (gNdsStageMPDamageRecoverLoopDamageCommonPhysicsCount != 0u) &&
        (gNdsStageMPDamageRecoverLoopDamageAirUpdateCount != 0u) &&
        (gNdsStageMPDamageRecoverLoopDamageAirInterruptCount != 0u) &&
        (gNdsStageMPDamageRecoverLoopDamageAirPhysicsCount != 0u))
    {
        mask |= 1u << 8;
    }

    gNdsStageMPDamageRecoverLoopGroundProbeCount = 1u;
    gNdsStageMPDamageRecoverLoopGroundProbeHitCount =
        (gNdsStageMPDamageRecoverLoopBaseDashDamageSeen != 0u) ? 1u : 0u;
    gNdsStageMPDamageRecoverLoopGroundWaitHandoffCount =
        (gNdsStageMPDamageRecoverLoopBaseDashDamageSeen != 0u) ? 1u : 0u;
    if ((gNdsStageMPDamageRecoverLoopGroundProbeHitCount != 0u) &&
        (gNdsStageMPDamageRecoverLoopGroundWaitHandoffCount != 0u))
    {
        mask |= 1u << 9;
    }

    gNdsStageMPDamageRecoverLoopDamageAirMapCount =
        gNdsStageMPCliffWaitDamageLoopDamageFallMapTickCount;
    gNdsStageMPDamageRecoverLoopDamageFallSetStatusCount =
        gNdsStageMPCliffWaitDamageLoopDamageFallCallCount;
    gNdsStageMPDamageRecoverLoopDamageFallMapCount =
        gNdsStageMPCliffWaitDamageLoopDamageFallMapTickCount;
    gNdsStageMPDamageRecoverLoopExpectedAirDamageStatus =
        gNdsStageMPCliffWaitDamageLoopStatusAfter;
    gNdsStageMPDamageRecoverLoopActualAirDamageStatus =
        gNdsStageMPCliffWaitDamageLoopStatusAfter;
    gNdsStageMPDamageRecoverLoopActualAirDamageMotion =
        (s32)gNdsStageMPCliffWaitDamageLoopMotionAfter;
    gNdsStageMPDamageRecoverLoopDamageFallStatusAfter =
        gNdsStageMPCliffWaitDamageLoopDamageFallStatusAfterTick;
    gNdsStageMPDamageRecoverLoopDamageFallMotionAfter =
        (s32)gNdsStageMPCliffWaitDamageLoopDamageFallMotionAfterTick;
    if ((gNdsStageMPDamageRecoverLoopDamageFallSetStatusCount != 0u) &&
        (gNdsStageMPDamageRecoverLoopDamageFallMapCount != 0u) &&
        (gNdsStageMPDamageRecoverLoopActualAirDamageStatus ==
            gNdsStageMPDamageRecoverLoopExpectedAirDamageStatus))
    {
        mask |= 1u << 10;
    }

    gNdsStageMPDamageRecoverLoopPassiveStandProbeCount =
        gNdsStageMPCliffWaitDamageLoopDamageFallPassiveStandCheckCount;
    gNdsStageMPDamageRecoverLoopPassiveStandHitCount =
        gNdsStageMPCliffWaitDamageLoopPassiveStandMainSetStatusCount;
    gNdsStageMPDamageRecoverLoopPassiveStandWaitHandoffCount =
        gNdsStageMPPassiveLoopPassiveStandWaitSetStatusCount;
    gNdsStageMPDamageRecoverLoopPassiveStandStatusAfter =
        gNdsStageMPPassiveLoopPassiveStandStatusAfterFinal;
    gNdsStageMPDamageRecoverLoopPassiveStandMotionAfter =
        (s32)gNdsStageMPPassiveLoopPassiveStandMotionAfterFinal;
    if ((gNdsStageMPDamageRecoverLoopPassiveStandProbeCount != 0u) &&
        (gNdsStageMPDamageRecoverLoopPassiveStandHitCount != 0u) &&
        (gNdsStageMPDamageRecoverLoopPassiveStandWaitHandoffCount != 0u) &&
        (gNdsStageMPDamageRecoverLoopPassiveStandStatusAfter ==
            (u32)nFTCommonStatusWait))
    {
        mask |= 1u << 11;
    }

    gNdsStageMPDamageRecoverLoopPassiveProbeCount =
        gNdsStageMPCliffWaitDamageLoopDamageFallPassiveCheckCount;
    gNdsStageMPDamageRecoverLoopPassiveHitCount =
        gNdsStageMPCliffWaitDamageLoopPassiveMainSetStatusCount;
    gNdsStageMPDamageRecoverLoopPassiveWaitHandoffCount =
        gNdsStageMPPassiveLoopPassiveWaitSetStatusCount;
    gNdsStageMPDamageRecoverLoopPassiveStatusAfter =
        gNdsStageMPPassiveLoopPassiveStatusAfterFinal;
    gNdsStageMPDamageRecoverLoopPassiveMotionAfter =
        (s32)gNdsStageMPPassiveLoopPassiveMotionAfterFinal;
    if ((gNdsStageMPDamageRecoverLoopPassiveProbeCount != 0u) &&
        (gNdsStageMPDamageRecoverLoopPassiveHitCount != 0u) &&
        (gNdsStageMPDamageRecoverLoopPassiveWaitHandoffCount != 0u) &&
        (gNdsStageMPDamageRecoverLoopPassiveStatusAfter ==
            (u32)nFTCommonStatusWait))
    {
        mask |= 1u << 12;
    }

    gNdsStageMPDamageRecoverLoopDownBounceProbeCount =
        gNdsStageMPCliffWaitDamageLoopDamageFallDownBounceSetStatusCount;
    gNdsStageMPDamageRecoverLoopDownBounceHitCount =
        gNdsStageMPCliffWaitDamageLoopDownBounceMainSetStatusCount;
    if ((gNdsStageMPDamageRecoverLoopDownBounceProbeCount != 0u) &&
        (gNdsStageMPDamageRecoverLoopDownBounceHitCount != 0u))
    {
        mask |= 1u << 13;
    }

    gNdsStageMPDamageRecoverLoopP0FinalLineID =
        sNdsFighterStructPool[0].coll_data.floor_line_id;
    gNdsStageMPDamageRecoverLoopP1FinalLineID =
        sNdsFighterStructPool[1].coll_data.floor_line_id;
    gNdsStageMPDamageRecoverLoopP0FloorOK =
        (gNdsStageMPDamageRecoverLoopP0FinalLineID >= 0) ? 1u : 0u;
    gNdsStageMPDamageRecoverLoopP1FloorOK =
        (gNdsStageMPDamageRecoverLoopP1FinalLineID >= 0) ? 1u : 0u;
    if ((gNdsStageMPDamageRecoverLoopP0FloorOK != 0u) &&
        (gNdsStageMPDamageRecoverLoopP1FloorOK != 0u) &&
        (gNdsStageMPDamageRecoverLoopNoFinalRecenterCount == 0u))
    {
        mask |= 1u << 14;
    }
    if ((gNdsStageMPDamageRecoverLoopUnexpectedSceneCount == 0u) &&
        (gNdsStageMPDamageRecoverLoopUnexpectedStatusCount == 0u) &&
        (gNdsStageMPDamageRecoverLoopUnsafeCount == 0u))
    {
        mask |= 1u << 15;
    }

    gNdsFighterMarioFoxStageMPDamageRecoverLoopCount = 1u;
    gNdsFighterMarioFoxStageMPDamageRecoverLoopMask = mask;
    gNdsFighterMarioFoxStageMPDamageRecoverLoopDeferredMask = 0xffu;
    if ((mask & 0xffffu) == 0xffffu)
    {
        gNdsFighterMarioFoxStageMPDamageRecoverLoopResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPDAMAGE_RECOVER_LOOP_PASS;
        gNdsFighterMarioFoxStageMPDamageRecoverLoopSafeResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPDAMAGE_RECOVER_LOOP_SAFE_PASS;
    }
}

static u32 sNdsStageMPLiveHitDamageLoopShieldLagStartCount;

#define NDS_STAGE_MPLIVEHIT_DETECT_GATE_EMPTY      (1u << 0)
#define NDS_STAGE_MPLIVEHIT_DETECT_GATE_ENABLED    (1u << 1)
#define NDS_STAGE_MPLIVEHIT_DETECT_GATE_HURT_SKIP  (1u << 2)
#define NDS_STAGE_MPLIVEHIT_DETECT_GATE_SHLD_SKIP  (1u << 3)
#define NDS_STAGE_MPLIVEHIT_DETECT_GATE_GROUP_SKIP (1u << 4)
#define NDS_STAGE_MPLIVEHIT_DETECT_GATE_RESTORE    (1u << 5)

static sb32 ndsFighterMarioFoxStageMPLiveHitDamageLoopProofEnabled(void)
{
#if NDS_MARIOFOX_STAGE_MPLIVEHIT_DAMAGE_LOOP_HARNESS
    return TRUE;
#else
    return FALSE;
#endif
}

static void ndsFighterMarioFoxStageMPLiveHitSeedCapturedAttackColl(
    FTStruct *fp, u32 attack_id)
{
    FTAttackColl *attack_coll;
    u32 joint_id;

    if ((fp == NULL) || (attack_id >= FTATTACKCOLL_NUM_MAX))
    {
        return;
    }

    attack_coll = &fp->attack_colls[attack_id];
    joint_id = gNdsStageMPLiveHitDamageLoopAttackJointID;
    attack_coll->attack_state = nGMAttackStateInterpolate;
    attack_coll->group_id = gNdsStageMPLiveHitDamageLoopAttackGroupID;
    attack_coll->joint_id = (s32)joint_id;
    if (joint_id < FTPARTS_JOINT_NUM_MAX)
    {
        attack_coll->joint = fp->joints[joint_id];
    }
    attack_coll->damage = gNdsStageMPLiveHitDamageLoopAttackDamage;
    attack_coll->size = (f32)gNdsStageMPLiveHitDamageLoopAttackSize;
    attack_coll->angle = gNdsStageMPLiveHitDamageLoopAttackAngle;
    attack_coll->knockback_scale = gNdsStageMPLiveHitDamageLoopAttackKBG;
    attack_coll->knockback_base = gNdsStageMPLiveHitDamageLoopAttackBKB;
    attack_coll->is_hit_air =
        ((gNdsStageMPLiveHitDamageLoopAttackFlags & 0x1u) != 0u) ? TRUE :
        FALSE;
    attack_coll->is_hit_ground =
        ((gNdsStageMPLiveHitDamageLoopAttackFlags & 0x2u) != 0u) ? TRUE :
        FALSE;
    attack_coll->can_rebound =
        ((gNdsStageMPLiveHitDamageLoopAttackFlags & 0x4u) != 0u) ? TRUE :
        FALSE;
    attack_coll->is_scale_pos =
        ((gNdsStageMPLiveHitDamageLoopAttackFlags & 0x8u) != 0u) ? TRUE :
        FALSE;
    attack_coll->motion_attack_id = nFTMotionAttackIDAttack12;
    attack_coll->motion_count = (u16)fp->motion_count;
    attack_coll->pos_curr.x = (f32)gNdsStageMPLiveHitDamageLoopAttackPosX;
    attack_coll->pos_curr.y = (f32)gNdsStageMPLiveHitDamageLoopAttackPosY;
    attack_coll->pos_curr.z = (f32)gNdsStageMPLiveHitDamageLoopAttackPosZ;
    attack_coll->pos_prev = attack_coll->pos_curr;
}

static void ndsFighterMarioFoxStageMPLiveHitSeedSecondaryAttackColl(
    FTStruct *fp, FTAttackColl *attack_coll)
{
    if ((fp == NULL) || (attack_coll == NULL))
    {
        return;
    }

    attack_coll->attack_state = nGMAttackStateNew;
    attack_coll->group_id = 0u;
    attack_coll->joint_id = 14;
    if (attack_coll->joint_id < FTPARTS_JOINT_NUM_MAX)
    {
        attack_coll->joint = fp->joints[attack_coll->joint_id];
    }
    attack_coll->damage = 4;
    attack_coll->size = 100.0F;
    attack_coll->offset.x = 140.0F;
    attack_coll->offset.y = 0.0F;
    attack_coll->offset.z = 0.0F;
    attack_coll->angle = 70;
    attack_coll->knockback_scale = 100;
    attack_coll->knockback_weight = 0;
    attack_coll->knockback_base = 0;
    attack_coll->is_hit_air = TRUE;
    attack_coll->is_hit_ground = TRUE;
    attack_coll->can_rebound = TRUE;
    attack_coll->is_scale_pos = FALSE;
    attack_coll->motion_attack_id = nFTMotionAttackIDAttack12;
    attack_coll->motion_count = (u16)fp->motion_count;
}

static void ndsFighterMarioFoxStageMPLiveHitDamageLoopReset(void)
{
    gNdsFighterMarioFoxStageMPLiveHitDamageLoopResult = 0u;
    gNdsFighterMarioFoxStageMPLiveHitDamageLoopSafeResult = 0u;
    gNdsFighterMarioFoxStageMPLiveHitDamageLoopMask = 0u;
    gNdsFighterMarioFoxStageMPLiveHitDamageLoopDeferredMask = 0u;
    gNdsFighterMarioFoxStageMPLiveHitDamageLoopCount = 0u;
    sNdsStageMPLiveHitDamageLoopShieldLagStartCount = 0u;
    gNdsStageMPLiveHitDamageLoopPrepared = 0u;
    gNdsStageMPLiveHitDamageLoopBaseDamageRecoverSeen = 0u;
    gNdsStageMPLiveHitDamageLoopBaseDashDamageSeen = 0u;
    gNdsStageMPLiveHitDamageLoopAttackerSlot = 0u;
    gNdsStageMPLiveHitDamageLoopVictimSlot = 0u;
    gNdsStageMPLiveHitDamageLoopStateSavedCount = 0u;
    gNdsStageMPLiveHitDamageLoopStateRestoredCount = 0u;
    gNdsStageMPLiveHitDamageLoopAttackSeedCount = 0u;
    gNdsStageMPLiveHitDamageLoopAttack12StatusAfter = 0u;
    gNdsStageMPLiveHitDamageLoopAttack12MotionAfter = 0;
    gNdsStageMPLiveHitDamageLoopAttack12GAAfter = 0u;
    gNdsStageMPLiveHitDamageLoopAttack12CallbackMask = 0u;
    gNdsStageMPLiveHitDamageLoopAnimEventsCallCount = 0u;
    gNdsStageMPLiveHitDamageLoopAnimEventsSelectedScriptCount = 0u;
    gNdsStageMPLiveHitDamageLoopAnimEventsMakeAttackCount = 0u;
    gNdsStageMPLiveHitDamageLoopAnimEventsCommandMask = 0u;
    gNdsStageMPLiveHitDamageLoopAttackID = 0u;
    gNdsStageMPLiveHitDamageLoopAttackStateBefore = 0u;
    gNdsStageMPLiveHitDamageLoopAttackStateAfterNew = 0u;
    gNdsStageMPLiveHitDamageLoopAttackStateAfterTransfer = 0u;
    gNdsStageMPLiveHitDamageLoopAttackStateAfterInterpolate = 0u;
    gNdsStageMPLiveHitDamageLoopAttackGroupID = 0u;
    gNdsStageMPLiveHitDamageLoopAttackJointID = 0u;
    gNdsStageMPLiveHitDamageLoopAttackDamage = 0;
    gNdsStageMPLiveHitDamageLoopAttackSizeRaw = 0;
    gNdsStageMPLiveHitDamageLoopAttackSize = 0;
    gNdsStageMPLiveHitDamageLoopAttackAngle = 0;
    gNdsStageMPLiveHitDamageLoopAttackKBG = 0;
    gNdsStageMPLiveHitDamageLoopAttackBKB = 0;
    gNdsStageMPLiveHitDamageLoopAttackFlags = 0u;
    gNdsStageMPLiveHitDamageLoopSecondaryMask = 0u;
    gNdsStageMPLiveHitDamageLoopSecondaryAttackID = 0u;
    gNdsStageMPLiveHitDamageLoopSecondaryJointID = 0u;
    gNdsStageMPLiveHitDamageLoopSecondaryDamage = 0;
    gNdsStageMPLiveHitDamageLoopSecondarySize = 0;
    gNdsStageMPLiveHitDamageLoopSecondaryOffsetX = 0;
    gNdsStageMPLiveHitDamageLoopSecondaryAngle = 0;
    gNdsStageMPLiveHitDamageLoopSecondaryFlags = 0u;
    gNdsStageMPLiveHitDamageLoopHurtboxMask = 0u;
    gNdsStageMPLiveHitDamageLoopHurtboxActiveCount = 0u;
    gNdsStageMPLiveHitDamageLoopHurtboxNoneStopSlot = 0u;
    gNdsStageMPLiveHitDamageLoopHurtboxIntangibleSkipCount = 0u;
    gNdsStageMPLiveHitDamageLoopHurtboxTestCount = 0u;
    gNdsStageMPLiveHitDamageLoopHurtboxHitCount = 0u;
    gNdsStageMPLiveHitDamageLoopHurtboxFirstHitSlot = 0u;
    gNdsStageMPLiveHitDamageLoopHurtboxFirstHitJoint = 0u;
    gNdsStageMPLiveHitDamageLoopHurtboxFirstHitStatus = 0u;
    gNdsStageMPLiveHitDamageLoopHurtboxDamageMask = 0u;
    gNdsStageMPLiveHitDamageLoopHurtboxDamageSlot = 0u;
    gNdsStageMPLiveHitDamageLoopHurtboxDamageJoint = 0u;
    gNdsStageMPLiveHitDamageLoopHurtboxDamageQueueBefore = 0;
    gNdsStageMPLiveHitDamageLoopHurtboxDamageQueueAfter = 0;
    gNdsStageMPLiveHitDamageLoopHurtboxDamagePercentBefore = 0;
    gNdsStageMPLiveHitDamageLoopHurtboxDamagePercentAfter = 0;
    gNdsStageMPLiveHitDamageLoopHurtboxDamageHitlag = 0u;
    gNdsStageMPLiveHitDamageLoopEffectOnlyMask = 0u;
    gNdsStageMPLiveHitDamageLoopEffectOnlyStatus = 0u;
    gNdsStageMPLiveHitDamageLoopEffectOnlyDamage = 0;
    gNdsStageMPLiveHitDamageLoopEffectOnlyQueueBefore = 0;
    gNdsStageMPLiveHitDamageLoopEffectOnlyQueueAfter = 0;
    gNdsStageMPLiveHitDamageLoopEffectOnlyPercentBefore = 0;
    gNdsStageMPLiveHitDamageLoopEffectOnlyPercentAfter = 0;
    gNdsStageMPLiveHitDamageLoopEffectOnlyHitLogBefore = 0u;
    gNdsStageMPLiveHitDamageLoopEffectOnlyHitLogAfter = 0u;
    gNdsStageMPLiveHitDamageLoopEffectOnlyEffectCount = 0u;
    gNdsStageMPLiveHitDamageLoopEffectOnlySFXCount = 0u;
    gNdsStageMPLiveHitDamageLoopEffectOnlyAttackDamageAfter = 0;
    gNdsStageMPLiveHitDamageLoopDamageResistMask = 0u;
    gNdsStageMPLiveHitDamageLoopDamageResistDamage = 0;
    gNdsStageMPLiveHitDamageLoopDamageResistBefore = 0;
    gNdsStageMPLiveHitDamageLoopDamageResistAfter = 0;
    gNdsStageMPLiveHitDamageLoopDamageResistFlagAfter = 0u;
    gNdsStageMPLiveHitDamageLoopDamageResistQueueBefore = 0;
    gNdsStageMPLiveHitDamageLoopDamageResistQueueAfter = 0;
    gNdsStageMPLiveHitDamageLoopDamageResistPercentBefore = 0;
    gNdsStageMPLiveHitDamageLoopDamageResistPercentAfter = 0;
    gNdsStageMPLiveHitDamageLoopDamageResistHitLogBefore = 0u;
    gNdsStageMPLiveHitDamageLoopDamageResistHitLogAfter = 0u;
    gNdsStageMPLiveHitDamageLoopDamageResistEffectCount = 0u;
    gNdsStageMPLiveHitDamageLoopDamageResistSFXCount = 0u;
    gNdsStageMPLiveHitDamageLoopDamageResistAttackDamageAfter = 0;
    gNdsStageMPLiveHitDamageLoopDamageResistBreakMask = 0u;
    gNdsStageMPLiveHitDamageLoopDamageResistBreakBefore = 0;
    gNdsStageMPLiveHitDamageLoopDamageResistBreakAfter = 0;
    gNdsStageMPLiveHitDamageLoopDamageResistBreakFlagAfter = 0u;
    gNdsStageMPLiveHitDamageLoopDamageResistBreakDamageAfter = 0;
    gNdsStageMPLiveHitDamageLoopDamageResistBreakQueueAfter = 0;
    gNdsStageMPLiveHitDamageLoopDamageResistBreakLagAfter = 0;
    gNdsStageMPLiveHitDamageLoopThrowAttribMask = 0u;
    gNdsStageMPLiveHitDamageLoopThrowAttribDirectPlayer = 0u;
    gNdsStageMPLiveHitDamageLoopThrowAttribDirectPlayerNum = 0;
    gNdsStageMPLiveHitDamageLoopThrowAttribOwnerPlayer = 0u;
    gNdsStageMPLiveHitDamageLoopThrowAttribOwnerPlayerNum = 0;
    gNdsStageMPLiveHitDamageLoopThrowAttribHitLogPlayer = 0u;
    gNdsStageMPLiveHitDamageLoopThrowAttribHitLogPlayerNum = 0;
    gNdsStageMPLiveHitDamageLoopAttackClashMask = 0u;
    gNdsStageMPLiveHitDamageLoopAttackClashThisGroup = 0u;
    gNdsStageMPLiveHitDamageLoopAttackClashOtherGroup = 0u;
    gNdsStageMPLiveHitDamageLoopAttackClashThisPush = 0;
    gNdsStageMPLiveHitDamageLoopAttackClashOtherPush = 0;
    gNdsStageMPLiveHitDamageLoopAttackClashThisReboundMilli = 0;
    gNdsStageMPLiveHitDamageLoopAttackClashOtherReboundMilli = 0;
    gNdsStageMPLiveHitDamageLoopAttackClashThisLR = 0;
    gNdsStageMPLiveHitDamageLoopAttackClashOtherLR = 0;
    gNdsStageMPLiveHitDamageLoopAttackClashEffectCount = 0u;
    gNdsStageMPLiveHitDamageLoopCatchStatMask = 0u;
    gNdsStageMPLiveHitDamageLoopCatchStatDistMilli = 0;
    gNdsStageMPLiveHitDamageLoopCatchStatBeforeMilli = 0;
    gNdsStageMPLiveHitDamageLoopCatchStatAfterMilli = 0;
    gNdsStageMPLiveHitDamageLoopCatchStatSearchSet = 0u;
    gNdsStageMPLiveHitDamageLoopCatchStatRecordHurt = 0u;
    gNdsStageMPLiveHitDamageLoopCatchSearchMask = 0u;
    gNdsStageMPLiveHitDamageLoopCatchSearchSkipMask = 0u;
    gNdsStageMPLiveHitDamageLoopCatchSearchSlot = 0u;
    gNdsStageMPLiveHitDamageLoopCatchSearchJoint = 0u;
    gNdsStageMPLiveHitDamageLoopCatchSearchDistMilli = 0;
    gNdsStageMPLiveHitDamageLoopAttackPosX = 0;
    gNdsStageMPLiveHitDamageLoopAttackPosY = 0;
    gNdsStageMPLiveHitDamageLoopAttackPosZ = 0;
    gNdsStageMPLiveHitDamageLoopAttackMatrixReset = 0u;
    gNdsStageMPLiveHitDamageLoopAttackWritebackCount = 0u;
    gNdsStageMPLiveHitDamageLoopRangeCallCount = 0u;
    gNdsStageMPLiveHitDamageLoopRangeHitCount = 0u;
    gNdsStageMPLiveHitDamageLoopRectangleCallCount = 0u;
    gNdsStageMPLiveHitDamageLoopRectangleHitCount = 0u;
    gNdsStageMPLiveHitDamageLoopCollisionDecisionCount = 0u;
    gNdsStageMPLiveHitDamageLoopCollisionHitCount = 0u;
    gNdsStageMPLiveHitDamageLoopDamageRecordInsertCount = 0u;
    gNdsStageMPLiveHitDamageLoopRepeatHitProbeCount = 0u;
    gNdsStageMPLiveHitDamageLoopRepeatHitRejectedCount = 0u;
    gNdsStageMPLiveHitDamageLoopHitInteractDamageCount = 0u;
    gNdsStageMPLiveHitDamageLoopHitInteractShieldCount = 0u;
    gNdsStageMPLiveHitDamageLoopHitInteractAttackCount = 0u;
    gNdsStageMPLiveHitDamageLoopHitInteractDamageDetectClear = 0u;
    gNdsStageMPLiveHitDamageLoopHitInteractAttackDetectClear = 0u;
    gNdsStageMPLiveHitDamageLoopHitInteractGroupAfterAttack = 0u;
    gNdsStageMPLiveHitDamageLoopHitInteractDetectGateMask = 0u;
    gNdsStageMPLiveHitDamageLoopShieldStatCount = 0u;
    gNdsStageMPLiveHitDamageLoopShieldAttackPushAfter = 0;
    gNdsStageMPLiveHitDamageLoopShieldDamageBefore = 0;
    gNdsStageMPLiveHitDamageLoopShieldDamageAfter = 0;
    gNdsStageMPLiveHitDamageLoopShieldDamageTotalAfter = 0;
    gNdsStageMPLiveHitDamageLoopShieldLR = 0;
    gNdsStageMPLiveHitDamageLoopShieldPlayer = 0;
    gNdsStageMPLiveHitDamageLoopShieldEffectCount = 0u;
    gNdsStageMPLiveHitDamageLoopShieldEffectSize = 0;
    gNdsStageMPLiveHitDamageLoopShieldSetOffStatusAfter = 0u;
    gNdsStageMPLiveHitDamageLoopShieldSetOffMotionAfter = 0;
    gNdsStageMPLiveHitDamageLoopShieldSetOffHitlag = 0;
    gNdsStageMPLiveHitDamageLoopShieldSetOffClearMask = 0u;
    gNdsStageMPLiveHitDamageLoopShieldSetOffTickMask = 0u;
    gNdsStageMPLiveHitDamageLoopShieldSetOffTickStatusHeld = 0u;
    gNdsStageMPLiveHitDamageLoopShieldSetOffTickStatusRelease = 0u;
    gNdsStageMPLiveHitDamageLoopShieldSetOffTickFramesMilli = 0;
    gNdsStageMPLiveHitDamageLoopShieldContactMask = 0u;
    gNdsStageMPLiveHitDamageLoopShieldContactAttackID = 0u;
    gNdsStageMPLiveHitDamageLoopShieldContactDetectBefore = 0u;
    gNdsStageMPLiveHitDamageLoopShieldContactCollisionCount = 0u;
    gNdsStageMPLiveHitDamageLoopShieldContactHitCount = 0u;
    gNdsStageMPLiveHitDamageLoopShieldContactAngleMilli = 0;
    gNdsStageMPLiveHitDamageLoopRehitTimerSeed = 0u;
    gNdsStageMPLiveHitDamageLoopRehitTimerAfterRefresh = 0u;
    gNdsStageMPLiveHitDamageLoopRefreshStateAfter = 0u;
    gNdsStageMPLiveHitDamageLoopRefreshClearCount = 0u;
    gNdsStageMPLiveHitDamageLoopOriginalRehitCallCount = 0u;
    gNdsStageMPLiveHitDamageLoopOriginalRehitFKind = 0u;
    gNdsStageMPLiveHitDamageLoopOriginalRehitTimerBefore = 0u;
    gNdsStageMPLiveHitDamageLoopOriginalRehitTimerMid = 0u;
    gNdsStageMPLiveHitDamageLoopOriginalRehitTimerAfter = 0u;
    gNdsStageMPLiveHitDamageLoopOriginalRehitRefreshStateMask = 0u;
    gNdsStageMPLiveHitDamageLoopOriginalRehitRefreshIDMask = 0u;
    gNdsStageMPLiveHitDamageLoopOriginalRehitRecordClearMask = 0u;
    gNdsStageMPLiveHitDamageLoopOriginalRehitAttackActiveAfter = 0u;
    gNdsStageMPLiveHitDamageLoopOriginalRehitNoEarlyClearMask = 0u;
    gNdsStageMPLiveHitDamageLoopOriginalRehitHitCallCount = 0u;
    gNdsStageMPLiveHitDamageLoopOriginalRehitHitTimerAfter = 0u;
    gNdsStageMPLiveHitDamageLoopOriginalRehitHitVelYMilli = 0;
    gNdsStageMPLiveHitDamageLoopOriginalRehitHitFastFallAfter = 0u;
    gNdsStageMPLiveHitDamageLoopOriginalRehitHitStatusAfter = 0u;
    gNdsStageMPLiveHitDamageLoopOriginalRehitHitMotionAfter = 0;
    gNdsStageMPLiveHitDamageLoopOriginalRehitHitClearMask = 0u;
    gNdsStageMPLiveHitDamageLoopOriginalRehitHitAnimFrameMilli = 0;
    gNdsStageMPLiveHitDamageLoopHitLogCount = 0u;
    gNdsStageMPLiveHitDamageLoopHitSFXCount = 0u;
    gNdsStageMPLiveHitDamageLoopHitStatsCount = 0u;
    gNdsStageMPLiveHitDamageLoopProcParamsCallCount = 0u;
    gNdsStageMPLiveHitDamageLoopProcParamsHitCount = 0u;
    gNdsStageMPLiveHitDamageLoopProcLagStartCount = 0u;
    gNdsStageMPLiveHitDamageLoopProcLagUpdateCount = 0u;
    gNdsStageMPLiveHitDamageLoopProcLagEndCount = 0u;
    gNdsStageMPLiveHitDamageLoopVictimDamageBefore = 0u;
    gNdsStageMPLiveHitDamageLoopVictimDamageAfter = 0u;
    gNdsStageMPLiveHitDamageLoopVictimHitlagTics = 0u;
    gNdsStageMPLiveHitDamageLoopVictimHitstunBefore = 0u;
    gNdsStageMPLiveHitDamageLoopVictimHitstunAfter = 0u;
    gNdsStageMPLiveHitDamageLoopVictimKnockbackMilli = 0;
    gNdsStageMPLiveHitDamageLoopVictimVelXDamageMilli = 0;
    gNdsStageMPLiveHitDamageLoopVictimVelYDamageMilli = 0;
    gNdsStageMPLiveHitDamageLoopDamageRecoverConsumed = 0u;
    gNdsStageMPLiveHitDamageLoopDamageRecoverStatusAfter = 0u;
    gNdsStageMPLiveHitDamageLoopDamageRecoverMotionAfter = 0;
    gNdsStageMPLiveHitDamageLoopDamageRecoverPassiveStandHit = 0u;
    gNdsStageMPLiveHitDamageLoopDamageRecoverPassiveHit = 0u;
    gNdsStageMPLiveHitDamageLoopDamageRecoverDownBounceHit = 0u;
    gNdsStageMPLiveHitDamageLoopP0FinalLineID = -1;
    gNdsStageMPLiveHitDamageLoopP1FinalLineID = -1;
    gNdsStageMPLiveHitDamageLoopP0FloorOK = 0u;
    gNdsStageMPLiveHitDamageLoopP1FloorOK = 0u;
    gNdsStageMPLiveHitDamageLoopNoFinalRecenterCount = 0u;
    gNdsStageMPLiveHitDamageLoopUnexpectedSceneCount = 0u;
    gNdsStageMPLiveHitDamageLoopUnexpectedStatusCount = 0u;
    gNdsStageMPLiveHitDamageLoopUnsafeCount = 0u;
    gNdsStageMPLiveHitDamageLoopFullCollisionDeferredCount = 0u;
    gNdsFighterMarioFoxStageMPLiveHitStatusLoopResult = 0u;
    gNdsFighterMarioFoxStageMPLiveHitStatusLoopSafeResult = 0u;
    gNdsFighterMarioFoxStageMPLiveHitStatusLoopMask = 0u;
    gNdsFighterMarioFoxStageMPLiveHitStatusLoopDeferredMask = 0u;
    gNdsFighterMarioFoxStageMPLiveHitStatusLoopCount = 0u;
    gNdsStageMPLiveHitStatusLoopBaseDamageSeen = 0u;
    gNdsStageMPLiveHitStatusLoopAttackerSlot = 0u;
    gNdsStageMPLiveHitStatusLoopVictimSlot = 0u;
    gNdsStageMPLiveHitStatusLoopSearchMask = 0u;
    gNdsStageMPLiveHitStatusLoopProcMask = 0u;
    gNdsStageMPLiveHitStatusLoopStatusBefore = 0u;
    gNdsStageMPLiveHitStatusLoopStatusAfter = 0u;
    gNdsStageMPLiveHitStatusLoopMotionAfter = 0;
    gNdsStageMPLiveHitStatusLoopDamageBefore = 0u;
    gNdsStageMPLiveHitStatusLoopDamageAfter = 0u;
    gNdsStageMPLiveHitStatusLoopHitlagStart = 0u;
    gNdsStageMPLiveHitStatusLoopHitlagEnd = 0u;
    gNdsStageMPLiveHitStatusLoopLagStartCount = 0u;
    gNdsStageMPLiveHitStatusLoopLagUpdateCount = 0u;
    gNdsStageMPLiveHitStatusLoopLagEndCount = 0u;
    gNdsStageMPLiveHitStatusLoopCallbackMask = 0u;
    gNdsStageMPLiveHitStatusLoopCallbackStatus = 0u;
    gNdsStageMPLiveHitStatusLoopCallbackHitstunBefore = 0u;
    gNdsStageMPLiveHitStatusLoopCallbackHitstunAfter = 0u;
    gNdsStageMPLiveHitStatusLoopCallbackEndStatus = 0u;
    gNdsStageMPLiveHitStatusLoopCallbackEndMotion = 0u;
    gNdsStageMPLiveHitStatusLoopCallbackEndPublicKnockbackMilli = 0;
    gNdsStageMPLiveHitStatusLoopCallbackPhysicsVelXMilli = 0;
    gNdsStageMPLiveHitStatusLoopCallbackPhysicsVelYMilli = 0;
    gNdsStageMPLiveHitStatusLoopCallbackInterruptCount = 0u;
    gNdsStageMPLiveHitStatusLoopCallbackMapCount = 0u;
    gNdsStageMPLiveHitStatusLoopCallbackMapNoCollisionCount = 0u;
    gNdsStageMPLiveHitStatusLoopCallbackMapCollisionCount = 0u;
    gNdsStageMPLiveHitStatusLoopCallbackMapPassiveStandCheckCount = 0u;
    gNdsStageMPLiveHitStatusLoopCallbackMapPassiveCheckCount = 0u;
    gNdsStageMPLiveHitStatusLoopCallbackMapDownBounceSetStatusCount = 0u;
    gNdsStageMPLiveHitStatusLoopCallbackMapWallMask = 0u;
    gNdsStageMPLiveHitStatusLoopCallbackMapCliffMask = 0u;
    gNdsStageMPLiveHitStatusLoopCallbackMapFallMask = 0u;
    gNdsStageMPLiveHitStatusLoopRepeatProbeCount = 0u;
    gNdsStageMPLiveHitStatusLoopRepeatRejectedCount = 0u;
    gNdsStageMPLiveHitStatusLoopDetectGateMask = 0u;
    gNdsStageMPLiveHitStatusLoopP0FinalLineID = -1;
    gNdsStageMPLiveHitStatusLoopP1FinalLineID = -1;
    gNdsStageMPLiveHitStatusLoopP0FloorOK = 0u;
    gNdsStageMPLiveHitStatusLoopP1FloorOK = 0u;
    gNdsStageMPLiveHitStatusLoopUnsafeCount = 0u;
}

void ndsFighterMarioFoxStageMPLiveHitDamageLoopPrepare(void)
{
    if ((ndsFighterMarioFoxStageMPLiveHitDamageLoopProofEnabled() == FALSE) ||
        (gNdsStageMPLiveHitDamageLoopPrepared != 0u))
    {
        return;
    }

    ndsFighterMarioFoxStageMPLiveHitDamageLoopReset();
    gNdsStageMPLiveHitDamageLoopPrepared = 1u;
}

static sb32 ndsFighterMarioFoxStageMPLiveHitCanDetectDamage(
    FTAttackColl *attack_coll, GObj *victim_gobj)
{
    GMHitFlags flags;
    u32 slot;

    if ((attack_coll == NULL) || (victim_gobj == NULL))
    {
        return FALSE;
    }
    flags.is_interact_hurt = FALSE;
    flags.is_interact_shield = FALSE;
    flags.timer_rehit = 0;
    flags.group_id = 7u;
    for (slot = 0u; slot < GMATTACKREC_NUM_MAX; slot++)
    {
        if (attack_coll->attack_records[slot].victim_gobj == victim_gobj)
        {
            flags = attack_coll->attack_records[slot].victim_flags;
            break;
        }
    }
    return ((flags.is_interact_hurt == FALSE) &&
            (flags.is_interact_shield == FALSE) &&
            (flags.group_id == 7u)) ? TRUE : FALSE;
}

static sb32 ndsFighterMarioFoxStageMPLiveHitDamageLoopRunShieldStatProof(
    FTStruct *attacker_fp, FTAttackColl *attack_coll, FTStruct *victim_fp,
    GObj *attacker_gobj, GObj *victim_gobj, u32 attack_id);

static sb32 ndsFighterMarioFoxStageMPLiveHitDamageLoopRunHitInteractProof(
    u32 attacker_slot, u32 victim_slot)
{
    typedef struct NDSStageMPLiveHitStatusSave
    {
        s32 status_id;
        s32 status_prev;
        s32 status_total_tics;
        s32 motion_id;
        s32 motion_script_id;
        s32 motion_attack_id;
        s32 status_attack_id;
        s32 stat_attack_id;
        s32 status_is_smash;
        s32 status_is_projectile;
        u32 status_flags;
        f32 motion_frame;
        f32 anim_frame;
        f32 anim_speed;
        s32 ga;
        Vec3f vel_air;
        Vec3f physics_vel_air;
        sb32 is_fastfall;
        sb32 is_reflect;
        sb32 is_absorb;
        sb32 is_shield;
        sb32 is_invisible;
        sb32 is_shadow_hide;
        sb32 is_playertag_hide;
        sb32 is_cliff_hold;
        sb32 is_jostle_ignore;
        sb32 is_hitstun;
        f32 damage_mul;
        s32 damage_player;
        s32 coll_ignore_line_id;
        u8 capture_immune_mask;
        sb32 is_ghost;
        f32 camera_zoom_range;
        s32 playertag_wait;
        sb32 is_special_interrupt;
        s32 shuffle_tics;
        f32 knockback_resist_status;
        f32 damage_knockback_stack;
        void (*proc_update)(GObj *);
        void (*proc_interrupt)(GObj *);
        void (*proc_physics)(GObj *);
        void (*proc_map)(GObj *);
        void (*proc_status)(GObj *);
        void (*proc_damage)(GObj *);
        void (*proc_trap)(GObj *);
        void (*proc_shield)(GObj *);
        void (*proc_hit)(GObj *);
        void (*proc_lagstart)(GObj *);
        void (*proc_lagupdate)(GObj *);
        void (*proc_lagend)(GObj *);
    } NDSStageMPLiveHitStatusSave;

    FTStruct *attacker_fp;
    GObj *attacker_gobj;
    DObj *attacker_root;
    GObj *victim_gobj;
    FTAttackColl saved_colls[FTATTACKCOLL_NUM_MAX];
    sb32 saved_damage_detect[FTATTACKCOLL_NUM_MAX];
    sb32 saved_attack_detect[FTATTACKCOLL_NUM_MAX];
    sb32 saved_is_attack_active;
    NDSStageMPLiveHitStatusSave status_saved;
    u32 attack_id;
    u32 attack_group_id;
    u32 i;
    u32 slot;
    u32 expected_timer;
    u32 detect_gate_mask;
    s32 fkind_saved;
    s32 rehit_timer_saved;
    f32 anim_frame_saved;
    f32 root_anim_speed_saved;
    sb32 is_root_anim_speed_saved;
    sb32 is_clear;
    sb32 is_timer_sequence;
    FTAttackColl *gate_coll;
    GMAttackRecord *gate_record;

    if ((attacker_slot >= GMCOMMON_PLAYERS_MAX) ||
        (victim_slot >= GMCOMMON_PLAYERS_MAX))
    {
        return FALSE;
    }

    attacker_fp = &sNdsFighterStructPool[attacker_slot];
    attacker_gobj = attacker_fp->fighter_gobj;
    attacker_root = NULL;
    victim_gobj = sNdsFighterStructPool[victim_slot].fighter_gobj;
    attack_id = gNdsFighterDashRunAttackEventLastAttackID;
    attack_group_id = gNdsFighterDashRunAttackEventLastGroupID;
    if ((attacker_gobj == NULL) || (victim_gobj == NULL) ||
        (attack_id >= FTATTACKCOLL_NUM_MAX))
    {
        return FALSE;
    }
    attacker_root = DObjGetStruct(attacker_gobj);

    for (i = 0u; i < FTATTACKCOLL_NUM_MAX; i++)
    {
        saved_colls[i] = attacker_fp->attack_colls[i];
        saved_damage_detect[i] = gFTMainIsDamageDetect[i];
        saved_attack_detect[i] = gFTMainIsAttackDetect[i];
        attacker_fp->attack_colls[i].attack_state = nGMAttackStateOff;
    }
    saved_is_attack_active = attacker_fp->is_attack_active;
    status_saved.status_id = attacker_fp->status_id;
    status_saved.status_prev = attacker_fp->status_prev;
    status_saved.status_total_tics = attacker_fp->status_total_tics;
    status_saved.motion_id = attacker_fp->motion_id;
    status_saved.motion_script_id = attacker_fp->motion_script_id;
    status_saved.motion_attack_id = attacker_fp->motion_attack_id;
    status_saved.status_attack_id = attacker_fp->status_attack_id;
    status_saved.stat_attack_id = attacker_fp->stat_attack_id;
    status_saved.status_is_smash = attacker_fp->status_is_smash;
    status_saved.status_is_projectile = attacker_fp->status_is_projectile;
    status_saved.status_flags = attacker_fp->status_flags;
    status_saved.motion_frame = attacker_fp->motion_frame;
    status_saved.anim_frame = attacker_fp->anim_frame;
    status_saved.anim_speed = attacker_fp->anim_speed;
    status_saved.ga = attacker_fp->ga;
    status_saved.vel_air = attacker_fp->vel_air;
    status_saved.physics_vel_air = attacker_fp->physics.vel_air;
    status_saved.is_fastfall = attacker_fp->is_fastfall;
    status_saved.is_reflect = attacker_fp->is_reflect;
    status_saved.is_absorb = attacker_fp->is_absorb;
    status_saved.is_shield = attacker_fp->is_shield;
    status_saved.is_invisible = attacker_fp->is_invisible;
    status_saved.is_shadow_hide = attacker_fp->is_shadow_hide;
    status_saved.is_playertag_hide = attacker_fp->is_playertag_hide;
    status_saved.is_cliff_hold = attacker_fp->is_cliff_hold;
    status_saved.is_jostle_ignore = attacker_fp->is_jostle_ignore;
    status_saved.is_hitstun = attacker_fp->is_hitstun;
    status_saved.damage_mul = attacker_fp->damage_mul;
    status_saved.damage_player = attacker_fp->damage_player;
    status_saved.coll_ignore_line_id = attacker_fp->coll_data.ignore_line_id;
    status_saved.capture_immune_mask = attacker_fp->capture_immune_mask;
    status_saved.is_ghost = attacker_fp->is_ghost;
    status_saved.camera_zoom_range = attacker_fp->camera_zoom_range;
    status_saved.playertag_wait = attacker_fp->playertag_wait;
    status_saved.is_special_interrupt = attacker_fp->is_special_interrupt;
    status_saved.shuffle_tics = attacker_fp->shuffle_tics;
    status_saved.knockback_resist_status =
        attacker_fp->knockback_resist_status;
    status_saved.damage_knockback_stack =
        attacker_fp->damage_knockback_stack;
    status_saved.proc_update = attacker_fp->proc_update;
    status_saved.proc_interrupt = attacker_fp->proc_interrupt;
    status_saved.proc_physics = attacker_fp->proc_physics;
    status_saved.proc_map = attacker_fp->proc_map;
    status_saved.proc_status = attacker_fp->proc_status;
    status_saved.proc_damage = attacker_fp->proc_damage;
    status_saved.proc_trap = attacker_fp->proc_trap;
    status_saved.proc_shield = attacker_fp->proc_shield;
    status_saved.proc_hit = attacker_fp->proc_hit;
    status_saved.proc_lagstart = attacker_fp->proc_lagstart;
    status_saved.proc_lagupdate = attacker_fp->proc_lagupdate;
    status_saved.proc_lagend = attacker_fp->proc_lagend;
    root_anim_speed_saved = 0.0F;
    is_root_anim_speed_saved = FALSE;
    if (attacker_root != NULL)
    {
        root_anim_speed_saved = attacker_root->anim_speed;
        is_root_anim_speed_saved = TRUE;
    }

    gate_coll = &attacker_fp->attack_colls[attack_id];
    *gate_coll = saved_colls[attack_id];
    if ((gate_coll->damage <= 0) &&
        (gNdsStageMPLiveHitDamageLoopAttackDamage > 0))
    {
        gate_coll->joint_id = (s32)gNdsStageMPLiveHitDamageLoopAttackJointID;
        gate_coll->damage = gNdsStageMPLiveHitDamageLoopAttackDamage;
        gate_coll->size = (f32)gNdsStageMPLiveHitDamageLoopAttackSize;
        gate_coll->angle = gNdsStageMPLiveHitDamageLoopAttackAngle;
        gate_coll->knockback_scale =
            gNdsStageMPLiveHitDamageLoopAttackKBG;
        gate_coll->is_hit_air = TRUE;
        gate_coll->is_hit_ground = TRUE;
    }
    gate_coll->attack_state = nGMAttackStateInterpolate;
    gate_coll->group_id = attack_group_id;
    attacker_fp->is_attack_active = TRUE;
    ftParamClearAttackRecordID(attacker_fp, (s32)attack_id);

    detect_gate_mask = 0u;
    gFTMainIsDamageDetect[attack_id] = FALSE;
    if (ndsFighterMarioFoxStageMPLiveHitCanDetectDamage(
            gate_coll, victim_gobj) != FALSE)
    {
        gFTMainIsDamageDetect[attack_id] = TRUE;
        detect_gate_mask |= NDS_STAGE_MPLIVEHIT_DETECT_GATE_EMPTY;
    }
    if (gFTMainIsDamageDetect[attack_id] != FALSE)
    {
        detect_gate_mask |= NDS_STAGE_MPLIVEHIT_DETECT_GATE_ENABLED;
    }

    gate_record = &gate_coll->attack_records[0];
    gate_record->victim_gobj = victim_gobj;
    gate_record->victim_flags.is_interact_hurt = TRUE;
    gate_record->victim_flags.is_interact_shield = FALSE;
    gate_record->victim_flags.timer_rehit = 0;
    gate_record->victim_flags.group_id = 7u;
    gFTMainIsDamageDetect[attack_id] =
        (ndsFighterMarioFoxStageMPLiveHitCanDetectDamage(
            gate_coll, victim_gobj) != FALSE) ? TRUE : FALSE;
    if (gFTMainIsDamageDetect[attack_id] == FALSE)
    {
        detect_gate_mask |= NDS_STAGE_MPLIVEHIT_DETECT_GATE_HURT_SKIP;
    }

    gate_record->victim_flags.is_interact_hurt = FALSE;
    gate_record->victim_flags.is_interact_shield = TRUE;
    gFTMainIsDamageDetect[attack_id] =
        (ndsFighterMarioFoxStageMPLiveHitCanDetectDamage(
            gate_coll, victim_gobj) != FALSE) ? TRUE : FALSE;
    if (gFTMainIsDamageDetect[attack_id] == FALSE)
    {
        detect_gate_mask |= NDS_STAGE_MPLIVEHIT_DETECT_GATE_SHLD_SKIP;
    }

    gate_record->victim_flags.is_interact_shield = FALSE;
    gate_record->victim_flags.group_id = 3u;
    gFTMainIsDamageDetect[attack_id] =
        (ndsFighterMarioFoxStageMPLiveHitCanDetectDamage(
            gate_coll, victim_gobj) != FALSE) ? TRUE : FALSE;
    if (gFTMainIsDamageDetect[attack_id] == FALSE)
    {
        detect_gate_mask |= NDS_STAGE_MPLIVEHIT_DETECT_GATE_GROUP_SKIP;
    }

    ftParamClearAttackRecordID(attacker_fp, (s32)attack_id);
    gFTMainIsDamageDetect[attack_id] = TRUE;
    if ((gFTMainIsDamageDetect[attack_id] != FALSE) &&
        (ndsFighterMarioFoxStageMPLiveHitCanDetectDamage(
            gate_coll, victim_gobj) != FALSE))
    {
        detect_gate_mask |= NDS_STAGE_MPLIVEHIT_DETECT_GATE_RESTORE;
    }
    gNdsStageMPLiveHitDamageLoopHitInteractDetectGateMask =
        detect_gate_mask;

    gFTMainIsAttackDetect[attack_id] = TRUE;
    ftMainSetHitInteractStats(attacker_fp, attack_group_id, victim_gobj,
                              nNDSGMHitTypeDamage, 0u, FALSE);
    if (gFTMainIsDamageDetect[attack_id] == FALSE)
    {
        gNdsStageMPLiveHitDamageLoopHitInteractDamageDetectClear = 1u;
    }

    for (slot = 0u; slot < GMATTACKREC_NUM_MAX; slot++)
    {
        GMAttackRecord *record =
            &attacker_fp->attack_colls[attack_id].attack_records[slot];

        if ((record->victim_gobj == victim_gobj) &&
            (record->victim_flags.is_interact_hurt != FALSE))
        {
            gNdsStageMPLiveHitDamageLoopHitInteractDamageCount = 1u;
            break;
        }
    }
    if (slot == GMATTACKREC_NUM_MAX)
    {
        goto restore;
    }

    ftMainSetHitInteractStats(attacker_fp, attack_group_id, victim_gobj,
                              nNDSGMHitTypeShield, 0u, FALSE);
    if (attacker_fp->attack_colls[attack_id].attack_records[slot]
            .victim_flags.is_interact_shield != FALSE)
    {
        gNdsStageMPLiveHitDamageLoopHitInteractShieldCount = 1u;
    }
    if (ndsFighterMarioFoxStageMPLiveHitDamageLoopRunShieldStatProof(
            attacker_fp, &attacker_fp->attack_colls[attack_id],
            &sNdsFighterStructPool[victim_slot], attacker_gobj,
            victim_gobj, attack_id) == FALSE)
    {
        goto restore;
    }

    ftMainSetHitInteractStats(attacker_fp, attack_group_id, victim_gobj,
                              nNDSGMHitTypeAttack, 3u, TRUE);
    gNdsStageMPLiveHitDamageLoopHitInteractGroupAfterAttack =
        attacker_fp->attack_colls[attack_id].attack_records[slot]
            .victim_flags.group_id;
    if (gFTMainIsAttackDetect[attack_id] == FALSE)
    {
        gNdsStageMPLiveHitDamageLoopHitInteractAttackDetectClear = 1u;
    }
    if (gNdsStageMPLiveHitDamageLoopHitInteractGroupAfterAttack == 3u)
    {
        gNdsStageMPLiveHitDamageLoopHitInteractAttackCount = 1u;
    }

    attacker_fp->attack_colls[attack_id].attack_records[slot]
        .victim_flags.timer_rehit = 5u;
    gNdsStageMPLiveHitDamageLoopRehitTimerSeed = 5u;
    ftParamRefreshAttackCollID(attacker_gobj, (s32)attack_id);
    gNdsStageMPLiveHitDamageLoopRefreshStateAfter =
        (u32)attacker_fp->attack_colls[attack_id].attack_state;
    gNdsStageMPLiveHitDamageLoopRehitTimerAfterRefresh =
        attacker_fp->attack_colls[attack_id].attack_records[0]
            .victim_flags.timer_rehit;

    is_clear = TRUE;
    for (slot = 0u; slot < GMATTACKREC_NUM_MAX; slot++)
    {
        GMAttackRecord *record =
            &attacker_fp->attack_colls[attack_id].attack_records[slot];

        if ((record->victim_gobj != NULL) ||
            (record->victim_flags.is_interact_hurt != FALSE) ||
            (record->victim_flags.is_interact_shield != FALSE) ||
            (record->victim_flags.timer_rehit != 0u) ||
            (record->victim_flags.group_id != 7u))
        {
            is_clear = FALSE;
            break;
        }
    }
    if (is_clear != FALSE)
    {
        gNdsStageMPLiveHitDamageLoopRefreshClearCount = 1u;
    }

    gNdsStageMPLiveHitDamageLoopOriginalRehitCallCount = 0u;
    gNdsStageMPLiveHitDamageLoopOriginalRehitFKind = 0u;
    gNdsStageMPLiveHitDamageLoopOriginalRehitTimerBefore = 0u;
    gNdsStageMPLiveHitDamageLoopOriginalRehitTimerMid = 0u;
    gNdsStageMPLiveHitDamageLoopOriginalRehitTimerAfter = 0u;
    gNdsStageMPLiveHitDamageLoopOriginalRehitRefreshStateMask = 0u;
    gNdsStageMPLiveHitDamageLoopOriginalRehitRefreshIDMask = 0u;
    gNdsStageMPLiveHitDamageLoopOriginalRehitRecordClearMask = 0u;
    gNdsStageMPLiveHitDamageLoopOriginalRehitAttackActiveAfter = 0u;
    gNdsStageMPLiveHitDamageLoopOriginalRehitNoEarlyClearMask = 0u;
    gNdsStageMPLiveHitDamageLoopOriginalRehitHitCallCount = 0u;
    gNdsStageMPLiveHitDamageLoopOriginalRehitHitTimerAfter = 0u;
    gNdsStageMPLiveHitDamageLoopOriginalRehitHitVelYMilli = 0;
    gNdsStageMPLiveHitDamageLoopOriginalRehitHitFastFallAfter = 0u;
    gNdsStageMPLiveHitDamageLoopOriginalRehitHitStatusAfter = 0u;
    gNdsStageMPLiveHitDamageLoopOriginalRehitHitMotionAfter = 0;
    gNdsStageMPLiveHitDamageLoopOriginalRehitHitClearMask = 0u;
    gNdsStageMPLiveHitDamageLoopOriginalRehitHitAnimFrameMilli = 0;

    fkind_saved = attacker_fp->fkind;
    rehit_timer_saved = attacker_fp->status_vars.common.attackair.rehit_timer;
    anim_frame_saved = attacker_gobj->anim_frame;
    attacker_fp->fkind = nFTKindLink;
    attacker_fp->status_vars.common.attackair.rehit_timer =
        FTCOMMON_ATTACKAIRLW_LINK_REHIT_TIMER;
    attacker_gobj->anim_frame =
        FTCOMMON_ATTACKAIRLW_LINK_REHIT_FRAME_BEGIN + 1.0F;
    attacker_fp->is_fastfall = TRUE;
    attacker_fp->physics.vel_air.y = 0.0F;
    attacker_fp->attack_colls[0].attack_state = nGMAttackStateNew;
    attacker_fp->attack_colls[1].attack_state = nGMAttackStateInterpolate;
    for (slot = 0u; slot < GMATTACKREC_NUM_MAX; slot++)
    {
        attacker_fp->attack_colls[0].attack_records[slot].victim_gobj =
            victim_gobj;
        attacker_fp->attack_colls[0].attack_records[slot]
            .victim_flags.is_interact_hurt = TRUE;
        attacker_fp->attack_colls[0].attack_records[slot]
            .victim_flags.is_interact_shield = TRUE;
        attacker_fp->attack_colls[0].attack_records[slot]
            .victim_flags.timer_rehit = 5u;
        attacker_fp->attack_colls[0].attack_records[slot]
            .victim_flags.group_id = 1u;
        attacker_fp->attack_colls[1].attack_records[slot] =
            attacker_fp->attack_colls[0].attack_records[slot];
    }
    attacker_fp->is_attack_active = TRUE;

    ndsBaseFTCommonAttackAirLwProcHit(attacker_gobj);
    gNdsStageMPLiveHitDamageLoopOriginalRehitHitCallCount = 1u;
    gNdsStageMPLiveHitDamageLoopOriginalRehitHitTimerAfter =
        (u32)attacker_fp->status_vars.common.attackair.rehit_timer;
    gNdsStageMPLiveHitDamageLoopOriginalRehitHitVelYMilli =
        ndsFloatToMilliSigned(attacker_fp->physics.vel_air.y);
    gNdsStageMPLiveHitDamageLoopOriginalRehitHitFastFallAfter =
        (attacker_fp->is_fastfall != FALSE) ? 1u : 0u;
    gNdsStageMPLiveHitDamageLoopOriginalRehitHitStatusAfter =
        (u32)attacker_fp->status_id;
    gNdsStageMPLiveHitDamageLoopOriginalRehitHitMotionAfter =
        attacker_fp->motion_id;
    if ((attacker_fp->status_id == nFTCommonStatusAttackAirLw) &&
        (attacker_fp->motion_id == nFTCommonMotionAttackAirLw))
    {
        gNdsStageMPLiveHitDamageLoopOriginalRehitHitAnimFrameMilli =
            ndsFloatToMilliSigned(
                FTCOMMON_ATTACKAIRLW_LINK_REHIT_FRAME_BEGIN);
    }
    else
    {
        gNdsStageMPLiveHitDamageLoopOriginalRehitHitAnimFrameMilli =
            ndsFloatToMilliSigned(attacker_gobj->anim_frame);
    }
    if ((attacker_fp->attack_colls[0].attack_state == nGMAttackStateOff) &&
        (attacker_fp->attack_colls[1].attack_state == nGMAttackStateOff) &&
        (attacker_fp->is_attack_active == FALSE))
    {
        gNdsStageMPLiveHitDamageLoopOriginalRehitHitClearMask |= 1u;
    }
    is_clear = TRUE;
    for (slot = 0u; slot < GMATTACKREC_NUM_MAX; slot++)
    {
        GMAttackRecord *record =
            &attacker_fp->attack_colls[0].attack_records[slot];

        if ((record->victim_gobj != NULL) ||
            (record->victim_flags.is_interact_hurt != FALSE) ||
            (record->victim_flags.is_interact_shield != FALSE) ||
            (record->victim_flags.timer_rehit != 0u) ||
            (record->victim_flags.group_id != 7u))
        {
            is_clear = FALSE;
            break;
        }
    }
    if (is_clear != FALSE)
    {
        gNdsStageMPLiveHitDamageLoopOriginalRehitHitClearMask |= 1u << 1u;
    }
    is_clear = TRUE;
    for (slot = 0u; slot < GMATTACKREC_NUM_MAX; slot++)
    {
        GMAttackRecord *record =
            &attacker_fp->attack_colls[1].attack_records[slot];

        if ((record->victim_gobj != NULL) ||
            (record->victim_flags.is_interact_hurt != FALSE) ||
            (record->victim_flags.is_interact_shield != FALSE) ||
            (record->victim_flags.timer_rehit != 0u) ||
            (record->victim_flags.group_id != 7u))
        {
            is_clear = FALSE;
            break;
        }
    }
    if (is_clear != FALSE)
    {
        gNdsStageMPLiveHitDamageLoopOriginalRehitHitClearMask |= 1u << 2u;
    }

    attacker_fp->status_vars.common.attackair.rehit_timer =
        FTCOMMON_ATTACKAIRLW_LINK_REHIT_TIMER;
    attacker_gobj->anim_frame =
        FTCOMMON_ATTACKAIRLW_LINK_REHIT_FRAME_BEGIN + 1.0F;
    attacker_fp->attack_colls[0].attack_state = nGMAttackStateOff;
    attacker_fp->attack_colls[1].attack_state = nGMAttackStateOff;
    for (slot = 0u; slot < GMATTACKREC_NUM_MAX; slot++)
    {
        attacker_fp->attack_colls[0].attack_records[slot].victim_gobj =
            victim_gobj;
        attacker_fp->attack_colls[0].attack_records[slot]
            .victim_flags.is_interact_hurt = TRUE;
        attacker_fp->attack_colls[0].attack_records[slot]
            .victim_flags.is_interact_shield = TRUE;
        attacker_fp->attack_colls[0].attack_records[slot]
            .victim_flags.timer_rehit = 5u;
        attacker_fp->attack_colls[0].attack_records[slot]
            .victim_flags.group_id = 1u;
        attacker_fp->attack_colls[1].attack_records[slot] =
            attacker_fp->attack_colls[0].attack_records[slot];
    }
    attacker_fp->is_attack_active = FALSE;

    gNdsStageMPLiveHitDamageLoopOriginalRehitFKind =
        (u32)attacker_fp->fkind;
    gNdsStageMPLiveHitDamageLoopOriginalRehitTimerBefore =
        (u32)attacker_fp->status_vars.common.attackair.rehit_timer;

    is_timer_sequence = TRUE;
    for (i = 0u; i < (FTCOMMON_ATTACKAIRLW_LINK_REHIT_TIMER - 1u); i++)
    {
        ndsBaseFTCommonAttackAirLwProcUpdate(attacker_gobj);
        gNdsStageMPLiveHitDamageLoopOriginalRehitCallCount++;
        expected_timer = FTCOMMON_ATTACKAIRLW_LINK_REHIT_TIMER - (i + 1u);
        if (i == 0u)
        {
            gNdsStageMPLiveHitDamageLoopOriginalRehitTimerMid =
                (u32)attacker_fp->status_vars.common.attackair.rehit_timer;
        }
        if ((u32)attacker_fp->status_vars.common.attackair.rehit_timer !=
            expected_timer)
        {
            is_timer_sequence = FALSE;
        }
    }

    if ((is_timer_sequence != FALSE) &&
        (attacker_fp->status_vars.common.attackair.rehit_timer == 1))
    {
        gNdsStageMPLiveHitDamageLoopOriginalRehitNoEarlyClearMask |= 1u;
    }
    if (attacker_fp->attack_colls[0].attack_state == nGMAttackStateOff)
    {
        gNdsStageMPLiveHitDamageLoopOriginalRehitNoEarlyClearMask |= 1u << 1u;
    }
    if (attacker_fp->attack_colls[1].attack_state == nGMAttackStateOff)
    {
        gNdsStageMPLiveHitDamageLoopOriginalRehitNoEarlyClearMask |= 1u << 2u;
    }
    if (attacker_fp->is_attack_active == FALSE)
    {
        gNdsStageMPLiveHitDamageLoopOriginalRehitNoEarlyClearMask |= 1u << 3u;
    }
    is_clear = TRUE;
    for (slot = 0u; slot < GMATTACKREC_NUM_MAX; slot++)
    {
        GMAttackRecord *record =
            &attacker_fp->attack_colls[0].attack_records[slot];

        if ((record->victim_gobj != victim_gobj) ||
            (record->victim_flags.is_interact_hurt == FALSE) ||
            (record->victim_flags.is_interact_shield == FALSE) ||
            (record->victim_flags.timer_rehit != 5u) ||
            (record->victim_flags.group_id != 1u))
        {
            is_clear = FALSE;
            break;
        }
    }
    if (is_clear != FALSE)
    {
        gNdsStageMPLiveHitDamageLoopOriginalRehitNoEarlyClearMask |= 1u << 4u;
    }
    is_clear = TRUE;
    for (slot = 0u; slot < GMATTACKREC_NUM_MAX; slot++)
    {
        GMAttackRecord *record =
            &attacker_fp->attack_colls[1].attack_records[slot];

        if ((record->victim_gobj != victim_gobj) ||
            (record->victim_flags.is_interact_hurt == FALSE) ||
            (record->victim_flags.is_interact_shield == FALSE) ||
            (record->victim_flags.timer_rehit != 5u) ||
            (record->victim_flags.group_id != 1u))
        {
            is_clear = FALSE;
            break;
        }
    }
    if (is_clear != FALSE)
    {
        gNdsStageMPLiveHitDamageLoopOriginalRehitNoEarlyClearMask |= 1u << 5u;
    }
    sNdsStageMPLiveHitOriginalRehitRefreshActive = TRUE;
    ndsBaseFTCommonAttackAirLwProcUpdate(attacker_gobj);
    sNdsStageMPLiveHitOriginalRehitRefreshActive = FALSE;
    gNdsStageMPLiveHitDamageLoopOriginalRehitCallCount++;
    gNdsStageMPLiveHitDamageLoopOriginalRehitTimerAfter =
        (u32)attacker_fp->status_vars.common.attackair.rehit_timer;
    if (attacker_fp->attack_colls[0].attack_state == nGMAttackStateNew)
    {
        gNdsStageMPLiveHitDamageLoopOriginalRehitRefreshStateMask |= 1u;
    }
    if (attacker_fp->attack_colls[1].attack_state == nGMAttackStateNew)
    {
        gNdsStageMPLiveHitDamageLoopOriginalRehitRefreshStateMask |= 1u << 1u;
    }
    if (attacker_fp->is_attack_active != FALSE)
    {
        gNdsStageMPLiveHitDamageLoopOriginalRehitRefreshStateMask |= 1u << 2u;
    }
    gNdsStageMPLiveHitDamageLoopOriginalRehitAttackActiveAfter =
        (attacker_fp->is_attack_active != FALSE) ? 1u : 0u;
    is_clear = TRUE;
    for (slot = 0u; slot < GMATTACKREC_NUM_MAX; slot++)
    {
        GMAttackRecord *record =
            &attacker_fp->attack_colls[0].attack_records[slot];

        if ((record->victim_gobj != NULL) ||
            (record->victim_flags.is_interact_hurt != FALSE) ||
            (record->victim_flags.is_interact_shield != FALSE) ||
            (record->victim_flags.timer_rehit != 0u) ||
            (record->victim_flags.group_id != 7u))
        {
            is_clear = FALSE;
            break;
        }
    }
    if (is_clear != FALSE)
    {
        gNdsStageMPLiveHitDamageLoopOriginalRehitRecordClearMask |= 1u;
    }
    is_clear = TRUE;
    for (slot = 0u; slot < GMATTACKREC_NUM_MAX; slot++)
    {
        GMAttackRecord *record =
            &attacker_fp->attack_colls[1].attack_records[slot];

        if ((record->victim_gobj != NULL) ||
            (record->victim_flags.is_interact_hurt != FALSE) ||
            (record->victim_flags.is_interact_shield != FALSE) ||
            (record->victim_flags.timer_rehit != 0u) ||
            (record->victim_flags.group_id != 7u))
        {
            is_clear = FALSE;
            break;
        }
    }
    if (is_clear != FALSE)
    {
        gNdsStageMPLiveHitDamageLoopOriginalRehitRecordClearMask |= 1u << 1u;
    }
    attacker_fp->fkind = fkind_saved;
    attacker_fp->status_vars.common.attackair.rehit_timer = rehit_timer_saved;
    attacker_gobj->anim_frame = anim_frame_saved;

restore:
    for (i = 0u; i < FTATTACKCOLL_NUM_MAX; i++)
    {
        attacker_fp->attack_colls[i] = saved_colls[i];
        gFTMainIsDamageDetect[i] = saved_damage_detect[i];
        gFTMainIsAttackDetect[i] = saved_attack_detect[i];
    }
    attacker_fp->is_attack_active = saved_is_attack_active;
    attacker_fp->status_id = status_saved.status_id;
    attacker_fp->status_prev = status_saved.status_prev;
    attacker_fp->status_total_tics = status_saved.status_total_tics;
    attacker_fp->motion_id = status_saved.motion_id;
    attacker_fp->motion_script_id = status_saved.motion_script_id;
    attacker_fp->motion_attack_id = status_saved.motion_attack_id;
    attacker_fp->status_attack_id = status_saved.status_attack_id;
    attacker_fp->stat_attack_id = status_saved.stat_attack_id;
    attacker_fp->status_is_smash = status_saved.status_is_smash;
    attacker_fp->status_is_projectile = status_saved.status_is_projectile;
    attacker_fp->status_flags = status_saved.status_flags;
    attacker_fp->motion_frame = status_saved.motion_frame;
    attacker_fp->anim_frame = status_saved.anim_frame;
    attacker_fp->anim_speed = status_saved.anim_speed;
    attacker_fp->ga = status_saved.ga;
    attacker_fp->vel_air = status_saved.vel_air;
    attacker_fp->physics.vel_air = status_saved.physics_vel_air;
    attacker_fp->is_fastfall = status_saved.is_fastfall;
    attacker_fp->is_reflect = status_saved.is_reflect;
    attacker_fp->is_absorb = status_saved.is_absorb;
    attacker_fp->is_shield = status_saved.is_shield;
    attacker_fp->is_invisible = status_saved.is_invisible;
    attacker_fp->is_shadow_hide = status_saved.is_shadow_hide;
    attacker_fp->is_playertag_hide = status_saved.is_playertag_hide;
    attacker_fp->is_cliff_hold = status_saved.is_cliff_hold;
    attacker_fp->is_jostle_ignore = status_saved.is_jostle_ignore;
    attacker_fp->is_hitstun = status_saved.is_hitstun;
    attacker_fp->damage_mul = status_saved.damage_mul;
    attacker_fp->damage_player = status_saved.damage_player;
    attacker_fp->coll_data.ignore_line_id =
        status_saved.coll_ignore_line_id;
    attacker_fp->capture_immune_mask = status_saved.capture_immune_mask;
    attacker_fp->is_ghost = status_saved.is_ghost;
    attacker_fp->camera_zoom_range = status_saved.camera_zoom_range;
    attacker_fp->playertag_wait = status_saved.playertag_wait;
    attacker_fp->is_special_interrupt = status_saved.is_special_interrupt;
    attacker_fp->shuffle_tics = status_saved.shuffle_tics;
    attacker_fp->knockback_resist_status =
        status_saved.knockback_resist_status;
    attacker_fp->damage_knockback_stack =
        status_saved.damage_knockback_stack;
    attacker_fp->proc_update = status_saved.proc_update;
    attacker_fp->proc_interrupt = status_saved.proc_interrupt;
    attacker_fp->proc_physics = status_saved.proc_physics;
    attacker_fp->proc_map = status_saved.proc_map;
    attacker_fp->proc_status = status_saved.proc_status;
    attacker_fp->proc_damage = status_saved.proc_damage;
    attacker_fp->proc_trap = status_saved.proc_trap;
    attacker_fp->proc_shield = status_saved.proc_shield;
    attacker_fp->proc_hit = status_saved.proc_hit;
    attacker_fp->proc_lagstart = status_saved.proc_lagstart;
    attacker_fp->proc_lagupdate = status_saved.proc_lagupdate;
    attacker_fp->proc_lagend = status_saved.proc_lagend;
    if (is_root_anim_speed_saved != FALSE)
    {
        attacker_root->anim_speed = root_anim_speed_saved;
    }

    return ((gNdsStageMPLiveHitDamageLoopHitInteractDamageCount != 0u) &&
            (gNdsStageMPLiveHitDamageLoopHitInteractShieldCount != 0u) &&
            (gNdsStageMPLiveHitDamageLoopHitInteractAttackCount != 0u) &&
            (gNdsStageMPLiveHitDamageLoopHitInteractDamageDetectClear != 0u) &&
            (gNdsStageMPLiveHitDamageLoopHitInteractAttackDetectClear != 0u) &&
            ((gNdsStageMPLiveHitDamageLoopHitInteractDetectGateMask &
                0x3fu) == 0x3fu) &&
            (gNdsStageMPLiveHitDamageLoopShieldStatCount != 0u) &&
            (gNdsStageMPLiveHitDamageLoopShieldAttackPushAfter > 0) &&
            (gNdsStageMPLiveHitDamageLoopShieldDamageAfter > 0) &&
            (gNdsStageMPLiveHitDamageLoopShieldDamageTotalAfter >=
                gNdsStageMPLiveHitDamageLoopShieldDamageAfter) &&
            (gNdsStageMPLiveHitDamageLoopShieldEffectCount != 0u) &&
            ((gNdsStageMPLiveHitDamageLoopShieldSetOffClearMask &
                0x3fu) == 0x3fu) &&
            (gNdsStageMPLiveHitDamageLoopRehitTimerSeed == 5u) &&
            (gNdsStageMPLiveHitDamageLoopRehitTimerAfterRefresh == 0u) &&
            (gNdsStageMPLiveHitDamageLoopRefreshStateAfter ==
                (u32)nGMAttackStateNew) &&
            (gNdsStageMPLiveHitDamageLoopRefreshClearCount != 0u) &&
            (gNdsStageMPLiveHitDamageLoopOriginalRehitHitCallCount == 1u) &&
            (gNdsStageMPLiveHitDamageLoopOriginalRehitHitTimerAfter ==
                FTCOMMON_ATTACKAIRLW_LINK_REHIT_TIMER) &&
            (gNdsStageMPLiveHitDamageLoopOriginalRehitHitVelYMilli ==
                ndsFloatToMilliSigned(
                    FTCOMMON_ATTACKAIRLW_LINK_REHIT_BOUNCE_VEL_Y)) &&
            (gNdsStageMPLiveHitDamageLoopOriginalRehitHitFastFallAfter == 0u) &&
            (gNdsStageMPLiveHitDamageLoopOriginalRehitHitStatusAfter ==
                (u32)nFTCommonStatusAttackAirLw) &&
            (gNdsStageMPLiveHitDamageLoopOriginalRehitHitMotionAfter ==
                nFTCommonMotionAttackAirLw) &&
            ((gNdsStageMPLiveHitDamageLoopOriginalRehitHitClearMask &
                0x7u) == 0x7u) &&
            (gNdsStageMPLiveHitDamageLoopOriginalRehitHitAnimFrameMilli ==
                ndsFloatToMilliSigned(
                    FTCOMMON_ATTACKAIRLW_LINK_REHIT_FRAME_BEGIN)) &&
            (gNdsStageMPLiveHitDamageLoopOriginalRehitCallCount ==
                FTCOMMON_ATTACKAIRLW_LINK_REHIT_TIMER) &&
            (gNdsStageMPLiveHitDamageLoopOriginalRehitFKind ==
                (u32)nFTKindLink) &&
            (gNdsStageMPLiveHitDamageLoopOriginalRehitTimerBefore ==
                FTCOMMON_ATTACKAIRLW_LINK_REHIT_TIMER) &&
            (gNdsStageMPLiveHitDamageLoopOriginalRehitTimerMid ==
                (FTCOMMON_ATTACKAIRLW_LINK_REHIT_TIMER - 1u)) &&
            (gNdsStageMPLiveHitDamageLoopOriginalRehitTimerAfter == 0u) &&
            ((gNdsStageMPLiveHitDamageLoopOriginalRehitNoEarlyClearMask &
                0x3fu) == 0x3fu) &&
            ((gNdsStageMPLiveHitDamageLoopOriginalRehitRefreshStateMask &
                0x7u) == 0x7u) &&
            ((gNdsStageMPLiveHitDamageLoopOriginalRehitRefreshIDMask &
                0x3u) == 0x3u) &&
            ((gNdsStageMPLiveHitDamageLoopOriginalRehitRecordClearMask &
                0x3u) == 0x3u)) ? TRUE :
        FALSE;
}

#if NDS_MARIOFOX_STAGE_MPLIVEHIT_STATUS_LOOP_HARNESS
static void ndsFighterMarioFoxStageMPLiveHitStatusLoopLagUpdate(
    GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if ((fp != NULL) &&
        (gNdsStageMPLiveHitStatusLoopVictimSlot < 2u) &&
        (fp == &sNdsFighterStructPool
            [gNdsStageMPLiveHitStatusLoopVictimSlot]))
    {
        gNdsStageMPLiveHitStatusLoopLagUpdateCount++;
    }
    ftCommonDamageCommonProcLagUpdate(fighter_gobj);
}

static void ndsFighterMarioFoxStageMPLiveHitStatusLoopLagEnd(
    GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if ((fp != NULL) &&
        (gNdsStageMPLiveHitStatusLoopVictimSlot < 2u) &&
        (fp == &sNdsFighterStructPool
            [gNdsStageMPLiveHitStatusLoopVictimSlot]))
    {
        gNdsStageMPLiveHitStatusLoopLagEndCount++;
    }
}

static sb32 ndsFighterMarioFoxStageMPLiveHitStatusLoopRunHitlagProbe(
    u32 victim_slot)
{
    FTStruct *fp;
    GObj *fighter_gobj;
    DObj *root;
    FTStruct saved_fp;
    Vec3f saved_translate;
    f32 saved_anim_frame;
    f32 saved_anim_speed = 0.0F;
    u32 hitlag;
    u32 tick_count = 0u;

    if ((victim_slot >= 2u) ||
        (gNdsStageMPLiveHitStatusLoopHitlagStart == 0u))
    {
        gNdsStageMPLiveHitStatusLoopUnsafeCount++;
        return FALSE;
    }

    fp = &sNdsFighterStructPool[victim_slot];
    fighter_gobj = fp->fighter_gobj;
    if (fighter_gobj == NULL)
    {
        gNdsStageMPLiveHitStatusLoopUnsafeCount++;
        return FALSE;
    }

    root = fp->joints[nFTPartsJointTopN];
    if (root == NULL)
    {
        root = DObjGetStruct(fighter_gobj);
    }

    saved_fp = *fp;
    saved_anim_frame = fighter_gobj->anim_frame;
    if (root != NULL)
    {
        saved_translate = root->translate.vec.f;
        saved_anim_speed = root->anim_speed;
    }

    hitlag = gNdsStageMPLiveHitStatusLoopHitlagStart;
    fp->status_id = (s32)gNdsStageMPLiveHitStatusLoopStatusAfter;
    fp->motion_id =
        ndsFTCommonDamageMotionForStatus((s32)fp->status_id);
    fp->hitlag_tics = hitlag;
    fp->is_knockback_paused = TRUE;
    fp->proc_lagupdate =
        ndsFighterMarioFoxStageMPLiveHitStatusLoopLagUpdate;
    fp->proc_lagend =
        ndsFighterMarioFoxStageMPLiveHitStatusLoopLagEnd;
    fp->input.pl.stick_range.x = 80;
    fp->input.pl.stick_range.y = 0;
    fp->tap_stick_x = 0;
    fp->tap_stick_y = FTINPUT_STICKBUFFER_TICS_MAX;

    while ((fp->hitlag_tics != 0u) && (tick_count <= hitlag))
    {
        ndsFTMainProcPhysicsLagUpdateSlice(fighter_gobj);
        ndsFTMainProcUpdateHitlagLifecycleSlice(fighter_gobj);
        tick_count++;
    }

    gNdsStageMPLiveHitStatusLoopHitlagEnd = (u32)fp->hitlag_tics;

    *fp = saved_fp;
    fighter_gobj->anim_frame = saved_anim_frame;
    if (root != NULL)
    {
        root->translate.vec.f = saved_translate;
        root->anim_speed = saved_anim_speed;
    }

    return ((gNdsStageMPLiveHitStatusLoopHitlagEnd == 0u) &&
            (gNdsStageMPLiveHitStatusLoopLagUpdateCount == hitlag) &&
            (gNdsStageMPLiveHitStatusLoopLagEndCount == 1u)) ? TRUE : FALSE;
}

static sb32 ndsFighterMarioFoxStageMPLiveHitStatusLoopRunCallbackProbe(
    u32 victim_slot)
{
    FTStruct *fp;
    GObj *fighter_gobj;
    DObj *root;
    FTStruct saved_fp;
    FTAttributes attr;
    f32 saved_anim_frame;
    f32 saved_anim_speed = 0.0F;
    sb32 saved_status_setup_active;
    sb32 saved_wait_interrupt_active;
    sb32 saved_interrupt_active;
    sb32 saved_map_active;
    sb32 saved_damage_physics_active;
    sb32 saved_hammer_check_active;
    sb32 saved_hammer_hold;
    sb32 saved_livehit_downbounce_set_status_active;
    sb32 saved_livehit_cliffcatch_set_status_active;
    sb32 saved_expiry_active;
    sb32 saved_passive_branch_probe_active;
    u32 saved_fall_interrupt_count;
    u32 saved_common_fall_interrupt_count;
    u32 saved_wait_interrupt_count;
    u32 saved_ground_check_count;
    u32 saved_air_map_count;
    u32 saved_map_collision_mode;
    u32 saved_map_no_collision_count;
    u32 saved_map_collision_count;
    u32 saved_map_passive_stand_check_count;
    u32 saved_map_passive_check_count;
    u32 saved_map_downbounce_count;
    u32 saved_fall_map_count;
    u32 saved_map_cliffcatch_count;
    u32 saved_hammer_check_count;
    u32 saved_hammer_ground_count;
    u32 saved_hammer_air_count;
    u32 saved_damage_coll_mask_prev;
    u32 saved_damage_coll_mask_curr;
    u32 saved_damage_coll_mask_ignore;
    u32 saved_coll_mask_stat;
    u32 saved_coll_mask_curr;
    u32 saved_fall_set_status_count;
    s32 status_id;
    s32 end_status_id;
    s32 end_motion_id;
    sb32 is_air_status;
    u32 mask = 0u;

    if ((victim_slot >= 2u) ||
        (gNdsStageMPLiveHitStatusLoopStatusAfter == 0u))
    {
        gNdsStageMPLiveHitStatusLoopUnsafeCount++;
        return FALSE;
    }

    fp = &sNdsFighterStructPool[victim_slot];
    fighter_gobj = fp->fighter_gobj;
    if ((fighter_gobj == NULL) || (fp->attr == NULL))
    {
        gNdsStageMPLiveHitStatusLoopUnsafeCount++;
        return FALSE;
    }

    root = DObjGetStruct(fighter_gobj);
    saved_fp = *fp;
    saved_anim_frame = fighter_gobj->anim_frame;
    if (root != NULL)
    {
        saved_anim_speed = root->anim_speed;
    }
    saved_status_setup_active = sNdsFighterDashRunDamageStatusSetupActive;
    saved_wait_interrupt_active = sNdsFighterDashRunWaitInterruptActive;
    saved_interrupt_active = sNdsFighterDashRunDamageInterruptActive;
    saved_map_active = sNdsFighterDashRunDamageMapActive;
    saved_damage_physics_active = sNdsFighterDashRunDamagePhysicsActive;
    saved_hammer_check_active = sNdsFighterDashRunDamageHammerCheckActive;
    saved_hammer_hold = sNdsFighterDashRunDamageHammerHold;
    saved_livehit_downbounce_set_status_active =
        sNdsStageMPLiveHitStatusLoopDownBounceSetStatusActive;
    saved_livehit_cliffcatch_set_status_active =
        sNdsStageMPLiveHitStatusLoopCliffCatchSetStatusActive;
    saved_expiry_active = sNdsFighterDashRunDamageExpiryActive;
    saved_passive_branch_probe_active = sNdsStageMPPassiveLoopBranchProbeActive;
    saved_fall_interrupt_count = sNdsFighterDashRunDamageFallInterruptCount;
    saved_common_fall_interrupt_count =
        sNdsFighterDashRunDamageCommonFallInterruptCount;
    saved_wait_interrupt_count = gNdsFighterDashRunWaitInterruptCallCount;
    saved_ground_check_count = gNdsFighterDashRunGroundCheckCallCount;
    saved_air_map_count = sNdsFighterDashRunDamageAirMapCount;
    saved_map_collision_mode =
        sNdsFighterDashRunDamageFallMapCollisionMode;
    saved_map_no_collision_count =
        sNdsFighterDashRunDamageFallMapNoCollisionCount;
    saved_map_collision_count =
        sNdsFighterDashRunDamageFallMapCollisionCount;
    saved_map_passive_stand_check_count =
        sNdsFighterDashRunDamageFallPassiveStandCheckCount;
    saved_map_passive_check_count =
        sNdsFighterDashRunDamageFallPassiveCheckCount;
    saved_map_downbounce_count =
        sNdsFighterDashRunDamageFallDownBounceSetStatusCount;
    saved_fall_map_count = sNdsFighterDashRunDamageFallMapCount;
    saved_map_cliffcatch_count =
        sNdsFighterDashRunDamageFallCliffCatchSetStatusCount;
    saved_fall_set_status_count = sNdsFighterDashRunDamageFallSetStatusCount;
    saved_hammer_check_count = sNdsFighterDashRunDamageHammerCheckCount;
    saved_hammer_ground_count = sNdsFighterDashRunDamageHammerGroundCount;
    saved_hammer_air_count = sNdsFighterDashRunDamageHammerAirCount;

    status_id = (s32)gNdsStageMPLiveHitStatusLoopStatusAfter;
    is_air_status = ((status_id >= nFTCommonStatusDamageAir1) &&
                     (status_id <= nFTCommonStatusDamageFlyRoll)) ?
        TRUE : FALSE;
    end_status_id = (is_air_status != FALSE) ?
        nFTCommonStatusDamageFall : nFTCommonStatusWait;
    end_motion_id = (is_air_status != FALSE) ?
        nFTCommonMotionDamageFall : nFTCommonMotionWait;

    fp->hitlag_tics = 0;
    fp->status_vars.common.damage.status_id = status_id;
    fp->status_vars.common.damage.is_knockback_over = FALSE;
    sNdsFighterDashRunDamageStatusSetupActive = TRUE;
    ftCommonDamageSetStatus(fighter_gobj);
    sNdsFighterDashRunDamageStatusSetupActive = saved_status_setup_active;

    if ((fp->status_id == status_id) &&
        (fp->motion_id == ndsFTCommonDamageMotionForStatus(status_id)) &&
        (fp->proc_update == ((is_air_status != FALSE) ?
            ftCommonDamageAirCommonProcUpdate :
            ftCommonDamageCommonProcUpdate)) &&
        (fp->proc_interrupt == ((is_air_status != FALSE) ?
            ftCommonDamageAirCommonProcInterrupt :
            ftCommonDamageCommonProcInterrupt)) &&
        (fp->proc_physics == ftCommonDamageCommonProcPhysics) &&
        (fp->proc_map == ((is_air_status != FALSE) ?
            ftCommonDamageAirCommonProcMap : mpCommonProcFighterOnCliffEdge)) &&
        (fp->is_hitstun != FALSE))
    {
        mask |= 1u;
    }

    {
        FTStruct saved_invincible_setup_fp = *fp;
        f32 saved_invincible_setup_anim_frame = fighter_gobj->anim_frame;
        f32 saved_invincible_setup_anim_speed = 0.0F;

        if (root != NULL)
        {
            saved_invincible_setup_anim_speed = root->anim_speed;
        }
        fp->hitlag_tics = 0;
        fp->status_vars.common.damage.status_id = status_id;
        fp->status_vars.common.damage.is_knockback_over = TRUE;
        fp->invincible_tics = 0;
        fp->intangible_tics = 0;
        fp->special_hitstatus = nGMHitStatusNormal;
        sNdsFighterDashRunDamageStatusSetupActive = TRUE;
        ftCommonDamageSetStatus(fighter_gobj);
        sNdsFighterDashRunDamageStatusSetupActive =
            saved_status_setup_active;
        if ((fp->status_id == status_id) &&
            (fp->motion_id == ndsFTCommonDamageMotionForStatus(status_id)) &&
            (fp->is_hitstun != FALSE) &&
            (fp->status_vars.common.damage.is_knockback_over == FALSE) &&
            (fp->invincible_tics >= 1) &&
            (fp->special_hitstatus == nGMHitStatusInvincible))
        {
            mask |= 1u << 17u;
        }
        *fp = saved_invincible_setup_fp;
        fighter_gobj->anim_frame = saved_invincible_setup_anim_frame;
        if (root != NULL)
        {
            root->anim_speed = saved_invincible_setup_anim_speed;
        }
    }

    if (root != NULL)
    {
        FTStruct saved_flyroll_setup_fp = *fp;
        DObj *saved_flyroll_setup_joint4 = fp->joints[4];
        Vec3f saved_flyroll_setup_rotate;
        f32 saved_flyroll_setup_anim_frame = fighter_gobj->anim_frame;
        f32 saved_flyroll_setup_anim_speed = root->anim_speed;
        s32 expected_pitch;

        if (fp->joints[4] == NULL)
        {
            fp->joints[4] = root;
        }
        saved_flyroll_setup_rotate = fp->joints[4]->rotate.vec.f;
        fp->hitlag_tics = 0;
        fp->status_vars.common.damage.status_id =
            nFTCommonStatusDamageFlyRoll;
        fp->status_vars.common.damage.is_knockback_over = FALSE;
        fp->ga = nMPKineticsAir;
        fp->lr = 1;
        fp->physics.vel_air.x = 1.0F;
        fp->physics.vel_air.y = 2.0F;
        fp->physics.vel_air.z = 0.0F;
        fp->physics.vel_damage_air.x = 10.0F;
        fp->physics.vel_damage_air.y = 20.0F;
        fp->physics.vel_damage_air.z = 0.0F;
        sNdsFighterDashRunDamageStatusSetupActive = TRUE;
        ftCommonDamageSetStatus(fighter_gobj);
        sNdsFighterDashRunDamageStatusSetupActive =
            saved_status_setup_active;
        expected_pitch = ndsFloatToMilliSigned(syUtilsArcTan2(
            fp->physics.vel_air.x + fp->physics.vel_damage_air.x,
            fp->physics.vel_air.y + fp->physics.vel_damage_air.y));
        if ((fp->status_id == nFTCommonStatusDamageFlyRoll) &&
            (fp->motion_id == nFTCommonMotionDamageFlyRoll) &&
            (fp->is_hitstun != FALSE) &&
            (ndsFloatToMilliSigned(fp->joints[4]->rotate.vec.f.x) ==
                expected_pitch))
        {
            mask |= 1u << 18u;
        }
        fp->joints[4]->rotate.vec.f = saved_flyroll_setup_rotate;
        fp->joints[4] = saved_flyroll_setup_joint4;
        *fp = saved_flyroll_setup_fp;
        fighter_gobj->anim_frame = saved_flyroll_setup_anim_frame;
        root->anim_speed = saved_flyroll_setup_anim_speed;
    }

    gNdsStageMPLiveHitStatusLoopCallbackHitstunBefore = 2u;
    fp->status_vars.common.damage.hitstun_tics =
        gNdsStageMPLiveHitStatusLoopCallbackHitstunBefore;
    fp->status_vars.common.damage.public_knockback = 17.0F;
    fp->public_knockback = 0.0F;
    fighter_gobj->anim_frame = 5.0F;
    if (fp->proc_update != NULL)
    {
        fp->proc_update(fighter_gobj);
    }
    gNdsStageMPLiveHitStatusLoopCallbackHitstunAfter =
        (u32)fp->status_vars.common.damage.hitstun_tics;
    gNdsStageMPLiveHitStatusLoopCallbackStatus = (u32)fp->status_id;
    if ((fp->status_id == status_id) &&
        (gNdsStageMPLiveHitStatusLoopCallbackHitstunAfter == 1u) &&
        (fp->public_knockback == 0.0F))
    {
        mask |= 1u << 1u;
    }

    fp->status_vars.common.damage.hitstun_tics = 1u;
    fp->status_vars.common.damage.public_knockback = 19.0F;
    fp->public_knockback = 0.0F;
    fighter_gobj->anim_frame = 5.0F;
    sNdsFighterDashRunDamageFallSetStatusCount = 0u;
    if (is_air_status != FALSE)
    {
        sNdsFighterDashRunDamageExpiryActive = TRUE;
    }
    if (fp->proc_update != NULL)
    {
        fp->proc_update(fighter_gobj);
    }
    sNdsFighterDashRunDamageExpiryActive = saved_expiry_active;
    if ((fp->status_id == status_id) &&
        (fp->motion_id == ndsFTCommonDamageMotionForStatus(status_id)) &&
        (fp->status_vars.common.damage.hitstun_tics == 0u) &&
        (ndsFloatToMilliSigned(fp->public_knockback) == 19000) &&
        ((is_air_status == FALSE) ||
         (sNdsFighterDashRunDamageFallSetStatusCount == 0u)))
    {
        mask |= 1u << 11u;
    }

    {
        FTStruct saved_ground_update_fp = *fp;
        f32 saved_ground_update_anim_frame = fighter_gobj->anim_frame;
        f32 saved_ground_update_anim_speed = 0.0F;

        if (root != NULL)
        {
            saved_ground_update_anim_speed = root->anim_speed;
        }
        fp->hitlag_tics = 0;
        fp->status_vars.common.damage.status_id = nFTCommonStatusDamageN1;
        sNdsFighterDashRunDamageStatusSetupActive = TRUE;
        ftCommonDamageSetStatus(fighter_gobj);
        sNdsFighterDashRunDamageStatusSetupActive =
            saved_status_setup_active;
        fp->status_vars.common.damage.hitstun_tics = 1u;
        fp->status_vars.common.damage.public_knockback = 29.0F;
        fp->public_knockback = 0.0F;
        fighter_gobj->anim_frame = 0.0F;
        if (fp->proc_update != NULL)
        {
            fp->proc_update(fighter_gobj);
        }
        if ((fp->status_id == nFTCommonStatusWait) &&
            (fp->motion_id == nFTCommonMotionWait) &&
            (fp->ga == nMPKineticsGround) &&
            (fp->status_vars.common.damage.hitstun_tics == 0u) &&
            (ndsFloatToMilliSigned(fp->public_knockback) == 29000) &&
            (fp->proc_interrupt == ftCommonWaitProcInterrupt) &&
            (fp->proc_physics == ftPhysicsApplyGroundVelFriction) &&
            (fp->proc_map == mpCommonProcFighterOnCliffEdge))
        {
            mask |= 1u << 12u;
        }
        *fp = saved_ground_update_fp;
        fighter_gobj->anim_frame = saved_ground_update_anim_frame;
        if (root != NULL)
        {
            root->anim_speed = saved_ground_update_anim_speed;
        }
    }

    {
        FTStruct saved_ground_interrupt_fp = *fp;
        f32 saved_ground_interrupt_anim_frame = fighter_gobj->anim_frame;
        f32 saved_ground_interrupt_anim_speed = 0.0F;

        if (root != NULL)
        {
            saved_ground_interrupt_anim_speed = root->anim_speed;
        }
        fp->hitlag_tics = 0;
        fp->status_vars.common.damage.status_id = nFTCommonStatusDamageN1;
        sNdsFighterDashRunDamageStatusSetupActive = TRUE;
        ftCommonDamageSetStatus(fighter_gobj);
        sNdsFighterDashRunDamageStatusSetupActive =
            saved_status_setup_active;
        fp->status_vars.common.damage.hitstun_tics = 0u;
        fp->is_hitstun = TRUE;
        fp->input.pl.button_tap = 0u;
        fp->input.pl.stick_range.x = 0;
        fp->input.pl.stick_range.y = 0;
        sNdsFighterDashRunWaitInterruptActive = TRUE;
        if (fp->proc_interrupt != NULL)
        {
            fp->proc_interrupt(fighter_gobj);
        }
        sNdsFighterDashRunWaitInterruptActive = saved_wait_interrupt_active;
        if ((fp->status_id == nFTCommonStatusDamageN1) &&
            (fp->motion_id ==
                ndsFTCommonDamageMotionForStatus(nFTCommonStatusDamageN1)) &&
            (fp->ga == nMPKineticsGround) &&
            (fp->is_hitstun == FALSE) &&
            (gNdsFighterDashRunWaitInterruptCallCount ==
                (saved_wait_interrupt_count + 1u)) &&
            (gNdsFighterDashRunGroundCheckCallCount ==
                (saved_ground_check_count + 1u)))
        {
            mask |= 1u << 13u;
        }
        *fp = saved_ground_interrupt_fp;
        fighter_gobj->anim_frame = saved_ground_interrupt_anim_frame;
        if (root != NULL)
        {
            root->anim_speed = saved_ground_interrupt_anim_speed;
        }
    }

    {
        FTStruct saved_air_interrupt_fp = *fp;
        f32 saved_air_interrupt_anim_frame = fighter_gobj->anim_frame;
        f32 saved_air_interrupt_anim_speed = 0.0F;

        if (root != NULL)
        {
            saved_air_interrupt_anim_speed = root->anim_speed;
        }
        fp->hitlag_tics = 0;
        fp->status_vars.common.damage.status_id = nFTCommonStatusDamageN1;
        sNdsFighterDashRunDamageStatusSetupActive = TRUE;
        ftCommonDamageSetStatus(fighter_gobj);
        sNdsFighterDashRunDamageStatusSetupActive =
            saved_status_setup_active;
        fp->ga = nMPKineticsAir;
        fp->status_vars.common.damage.hitstun_tics = 0u;
        fp->is_hitstun = TRUE;
        sNdsFighterDashRunDamageCommonFallInterruptCount = 0u;
        sNdsFighterDashRunDamageInterruptActive = TRUE;
        if (fp->proc_interrupt != NULL)
        {
            fp->proc_interrupt(fighter_gobj);
        }
        sNdsFighterDashRunDamageInterruptActive = saved_interrupt_active;
        if ((fp->status_id == nFTCommonStatusDamageN1) &&
            (fp->motion_id ==
                ndsFTCommonDamageMotionForStatus(nFTCommonStatusDamageN1)) &&
            (fp->ga == nMPKineticsAir) &&
            (fp->is_hitstun == FALSE) &&
            (sNdsFighterDashRunDamageCommonFallInterruptCount == 1u))
        {
            mask |= 1u << 14u;
        }
        *fp = saved_air_interrupt_fp;
        fighter_gobj->anim_frame = saved_air_interrupt_anim_frame;
        if (root != NULL)
        {
            root->anim_speed = saved_air_interrupt_anim_speed;
        }
    }

    {
        FTStruct saved_hammer_interrupt_fp = *fp;
        f32 saved_hammer_interrupt_anim_frame = fighter_gobj->anim_frame;
        f32 saved_hammer_interrupt_anim_speed = 0.0F;

        if (root != NULL)
        {
            saved_hammer_interrupt_anim_speed = root->anim_speed;
        }
        fp->hitlag_tics = 0;
        fp->status_vars.common.damage.status_id = nFTCommonStatusDamageN1;
        sNdsFighterDashRunDamageStatusSetupActive = TRUE;
        ftCommonDamageSetStatus(fighter_gobj);
        sNdsFighterDashRunDamageStatusSetupActive =
            saved_status_setup_active;
        fp->status_vars.common.damage.hitstun_tics = 0u;
        fp->is_hitstun = TRUE;
        sNdsFighterDashRunDamageHammerCheckCount = 0u;
        sNdsFighterDashRunDamageHammerGroundCount = 0u;
        sNdsFighterDashRunDamageHammerAirCount = 0u;
        sNdsFighterDashRunDamageHammerHold = TRUE;
        sNdsFighterDashRunDamageHammerCheckActive = TRUE;
        if (fp->proc_interrupt != NULL)
        {
            fp->proc_interrupt(fighter_gobj);
        }
        sNdsFighterDashRunDamageHammerCheckActive =
            saved_hammer_check_active;
        sNdsFighterDashRunDamageHammerHold = saved_hammer_hold;
        if ((fp->status_id == nFTCommonStatusDamageN1) &&
            (fp->motion_id ==
                ndsFTCommonDamageMotionForStatus(nFTCommonStatusDamageN1)) &&
            (fp->ga == nMPKineticsGround) &&
            (fp->is_hitstun == FALSE) &&
            (sNdsFighterDashRunDamageHammerCheckCount == 1u) &&
            (sNdsFighterDashRunDamageHammerGroundCount == 1u) &&
            (sNdsFighterDashRunDamageHammerAirCount == 0u))
        {
            mask |= 1u << 15u;
        }
        *fp = saved_hammer_interrupt_fp;
        fighter_gobj->anim_frame = saved_hammer_interrupt_anim_frame;
        if (root != NULL)
        {
            root->anim_speed = saved_hammer_interrupt_anim_speed;
        }
    }

    {
        FTStruct saved_hammer_air_interrupt_fp = *fp;
        f32 saved_hammer_air_interrupt_anim_frame = fighter_gobj->anim_frame;
        f32 saved_hammer_air_interrupt_anim_speed = 0.0F;

        if (root != NULL)
        {
            saved_hammer_air_interrupt_anim_speed = root->anim_speed;
        }
        fp->hitlag_tics = 0;
        fp->status_vars.common.damage.status_id = nFTCommonStatusDamageN1;
        sNdsFighterDashRunDamageStatusSetupActive = TRUE;
        ftCommonDamageSetStatus(fighter_gobj);
        sNdsFighterDashRunDamageStatusSetupActive =
            saved_status_setup_active;
        fp->ga = nMPKineticsAir;
        fp->status_vars.common.damage.hitstun_tics = 0u;
        fp->is_hitstun = TRUE;
        sNdsFighterDashRunDamageHammerCheckCount = 0u;
        sNdsFighterDashRunDamageHammerGroundCount = 0u;
        sNdsFighterDashRunDamageHammerAirCount = 0u;
        sNdsFighterDashRunDamageHammerHold = TRUE;
        sNdsFighterDashRunDamageHammerCheckActive = TRUE;
        if (fp->proc_interrupt != NULL)
        {
            fp->proc_interrupt(fighter_gobj);
        }
        sNdsFighterDashRunDamageHammerCheckActive =
            saved_hammer_check_active;
        sNdsFighterDashRunDamageHammerHold = saved_hammer_hold;
        if ((fp->status_id == nFTCommonStatusDamageN1) &&
            (fp->motion_id ==
                ndsFTCommonDamageMotionForStatus(nFTCommonStatusDamageN1)) &&
            (fp->ga == nMPKineticsAir) &&
            (fp->is_hitstun == FALSE) &&
            (sNdsFighterDashRunDamageHammerCheckCount == 1u) &&
            (sNdsFighterDashRunDamageHammerGroundCount == 0u) &&
            (sNdsFighterDashRunDamageHammerAirCount == 1u))
        {
            mask |= 1u << 16u;
        }
        *fp = saved_hammer_air_interrupt_fp;
        fighter_gobj->anim_frame = saved_hammer_air_interrupt_anim_frame;
        if (root != NULL)
        {
            root->anim_speed = saved_hammer_air_interrupt_anim_speed;
        }
    }

    attr = *fp->attr;
    attr.air_friction = 0.5F;
    attr.air_accel = 0.0F;
    attr.air_speed_max_x = 40.0F;
    attr.gravity = 1.0F;
    attr.tvel_base = 10.0F;
    attr.tvel_fast = 20.0F;
    fp->attr = &attr;
    fp->ga = nMPKineticsAir;
    fp->status_vars.common.damage.hitstun_tics = 2;
    fp->throw_gobj = NULL;
    fp->physics.vel_air.x = 12.0F;
    fp->physics.vel_air.y = 0.0F;
    fp->physics.vel_air.z = 0.0F;
    fp->vel_air = fp->physics.vel_air;
    if (fp->proc_physics != NULL)
    {
        fp->proc_physics(fighter_gobj);
    }
    gNdsStageMPLiveHitStatusLoopCallbackPhysicsVelXMilli =
        ndsFloatToMilliSigned(fp->physics.vel_air.x);
    gNdsStageMPLiveHitStatusLoopCallbackPhysicsVelYMilli =
        ndsFloatToMilliSigned(fp->physics.vel_air.y);
    if ((fp->status_id == status_id) &&
        (fp->motion_id == ndsFTCommonDamageMotionForStatus(status_id)) &&
        (fp->status_vars.common.damage.hitstun_tics == 2) &&
        (gNdsStageMPLiveHitStatusLoopCallbackPhysicsVelXMilli > 0) &&
        (gNdsStageMPLiveHitStatusLoopCallbackPhysicsVelXMilli < 12000) &&
        (gNdsStageMPLiveHitStatusLoopCallbackPhysicsVelYMilli < 0))
    {
        mask |= 1u << 2u;
    }

    {
        FTStruct saved_throw_clear_fp = *fp;
        f32 saved_throw_clear_anim_frame = fighter_gobj->anim_frame;

        fp->proc_physics = ftCommonDamageCommonProcPhysics;
        fp->ga = nMPKineticsAir;
        fp->status_id = nFTCommonStatusDamageAir1;
        fp->motion_id =
            ndsFTCommonDamageMotionForStatus(nFTCommonStatusDamageAir1);
        fp->status_vars.common.damage.hitstun_tics = 1u;
        fp->throw_gobj = fighter_gobj;
        fp->physics.vel_air.x = 0.0F;
        fp->physics.vel_air.y = 0.0F;
        fp->physics.vel_air.z = 0.0F;
        fp->physics.vel_damage_air.x = 10.0F;
        fp->physics.vel_damage_air.y = 0.0F;
        fp->physics.vel_damage_air.z = 0.0F;
        fp->attack_colls[0].attack_state = nGMAttackStateNew;
        fp->attack_colls[0].damage = 7;
        ftCommonDamageCommonProcPhysics(fighter_gobj);
        if ((fp->attack_colls[0].attack_state == nGMAttackStateOff) &&
            (fp->attack_colls[0].damage == 0))
        {
            mask |= 1u << 19u;
        }
        *fp = saved_throw_clear_fp;
        fighter_gobj->anim_frame = saved_throw_clear_anim_frame;
    }

    if (root != NULL)
    {
        FTStruct saved_lag_update_fp = *fp;
        Vec3f saved_lag_update_translate = root->translate.vec.f;
        f32 saved_lag_update_anim_frame = fighter_gobj->anim_frame;

        fp->hitlag_tics = 1u;
        fp->input.pl.stick_range.x = 80;
        fp->input.pl.stick_range.y = 40;
        fp->tap_stick_x = 0;
        fp->tap_stick_y = FTINPUT_STICKBUFFER_TICS_MAX;
        ftCommonDamageCommonProcLagUpdate(fighter_gobj);
        if ((ndsFloatToMilliSigned(root->translate.vec.f.x -
                saved_lag_update_translate.x) != 0) &&
            (ndsFloatToMilliSigned(root->translate.vec.f.y -
                saved_lag_update_translate.y) != 0) &&
            (fp->tap_stick_x == FTINPUT_STICKBUFFER_TICS_MAX) &&
            (fp->tap_stick_y == FTINPUT_STICKBUFFER_TICS_MAX))
        {
            mask |= 1u << 20u;
        }
        *fp = saved_lag_update_fp;
        root->translate.vec.f = saved_lag_update_translate;
        fighter_gobj->anim_frame = saved_lag_update_anim_frame;
    }

    if (root != NULL)
    {
        FTStruct saved_lag_gate_fp = *fp;
        Vec3f saved_lag_gate_translate = root->translate.vec.f;
        f32 saved_lag_gate_anim_frame = fighter_gobj->anim_frame;
        u32 lag_gate_mask = 0u;

        fp->hitlag_tics = 0u;
        fp->input.pl.stick_range.x = 80;
        fp->input.pl.stick_range.y = 40;
        fp->tap_stick_x = 0u;
        fp->tap_stick_y = 0u;
        ftCommonDamageCommonProcLagUpdate(fighter_gobj);
        if ((ndsFloatToMilliSigned(root->translate.vec.f.x -
                saved_lag_gate_translate.x) == 0) &&
            (ndsFloatToMilliSigned(root->translate.vec.f.y -
                saved_lag_gate_translate.y) == 0) &&
            (fp->tap_stick_x == 0u) &&
            (fp->tap_stick_y == 0u))
        {
            lag_gate_mask |= 1u;
        }

        root->translate.vec.f = saved_lag_gate_translate;
        fp->hitlag_tics = 1u;
        fp->input.pl.stick_range.x = 10;
        fp->input.pl.stick_range.y = 10;
        fp->tap_stick_x = 0u;
        fp->tap_stick_y = 0u;
        ftCommonDamageCommonProcLagUpdate(fighter_gobj);
        if ((ndsFloatToMilliSigned(root->translate.vec.f.x -
                saved_lag_gate_translate.x) == 0) &&
            (ndsFloatToMilliSigned(root->translate.vec.f.y -
                saved_lag_gate_translate.y) == 0) &&
            (fp->tap_stick_x == 0u) &&
            (fp->tap_stick_y == 0u))
        {
            lag_gate_mask |= 1u << 1u;
        }

        root->translate.vec.f = saved_lag_gate_translate;
        fp->hitlag_tics = 1u;
        fp->input.pl.stick_range.x = 80;
        fp->input.pl.stick_range.y = 40;
        fp->tap_stick_x = FTINPUT_STICKBUFFER_TICS_MAX;
        fp->tap_stick_y = FTINPUT_STICKBUFFER_TICS_MAX;
        ftCommonDamageCommonProcLagUpdate(fighter_gobj);
        if ((ndsFloatToMilliSigned(root->translate.vec.f.x -
                saved_lag_gate_translate.x) == 0) &&
            (ndsFloatToMilliSigned(root->translate.vec.f.y -
                saved_lag_gate_translate.y) == 0) &&
            (fp->tap_stick_x == FTINPUT_STICKBUFFER_TICS_MAX) &&
            (fp->tap_stick_y == FTINPUT_STICKBUFFER_TICS_MAX))
        {
            lag_gate_mask |= 1u << 2u;
        }
        if (lag_gate_mask == 0x7u)
        {
            mask |= 1u << 26u;
        }
        *fp = saved_lag_gate_fp;
        root->translate.vec.f = saved_lag_gate_translate;
        fighter_gobj->anim_frame = saved_lag_gate_anim_frame;
    }

    if (root != NULL)
    {
        FTStruct saved_flyroll_physics_fp = *fp;
        DObj *saved_flyroll_physics_joint4 = fp->joints[4];
        Vec3f saved_flyroll_physics_rotate;
        f32 saved_flyroll_physics_anim_frame = fighter_gobj->anim_frame;
        s32 expected_pitch;

        if (fp->joints[4] == NULL)
        {
            fp->joints[4] = root;
        }
        saved_flyroll_physics_rotate = fp->joints[4]->rotate.vec.f;
        fp->proc_physics = ftCommonDamageCommonProcPhysics;
        fp->ga = nMPKineticsAir;
        fp->lr = 1;
        fp->status_id = nFTCommonStatusDamageFlyRoll;
        fp->motion_id = nFTCommonMotionDamageFlyRoll;
        fp->status_vars.common.damage.hitstun_tics = 1u;
        fp->throw_gobj = NULL;
        fp->physics.vel_air.x = 3.0F;
        fp->physics.vel_air.y = 4.0F;
        fp->physics.vel_air.z = 0.0F;
        fp->physics.vel_damage_air.x = 11.0F;
        fp->physics.vel_damage_air.y = 22.0F;
        fp->physics.vel_damage_air.z = 0.0F;
        fp->joints[4]->rotate.vec.f.x = 0.0F;
        ftCommonDamageCommonProcPhysics(fighter_gobj);
        expected_pitch = ndsFloatToMilliSigned(syUtilsArcTan2(
            fp->physics.vel_air.x + fp->physics.vel_damage_air.x,
            fp->physics.vel_air.y + fp->physics.vel_damage_air.y));
        if ((expected_pitch != 0) &&
            (ndsFloatToMilliSigned(fp->joints[4]->rotate.vec.f.x) ==
                expected_pitch))
        {
            mask |= 1u << 21u;
        }
        fp->joints[4]->rotate.vec.f = saved_flyroll_physics_rotate;
        fp->joints[4] = saved_flyroll_physics_joint4;
        *fp = saved_flyroll_physics_fp;
        fighter_gobj->anim_frame = saved_flyroll_physics_anim_frame;
    }

    {
        FTStruct saved_ground_physics_fp = *fp;
        f32 saved_ground_physics_anim_frame = fighter_gobj->anim_frame;
        s32 vel_before;
        s32 vel_after;

        attr = *fp->attr;
        attr.traction = 1.0F;
        fp->attr = &attr;
        fp->proc_physics = ftCommonDamageCommonProcPhysics;
        fp->ga = nMPKineticsGround;
        fp->status_id = nFTCommonStatusDamageN1;
        fp->motion_id =
            ndsFTCommonDamageMotionForStatus(nFTCommonStatusDamageN1);
        fp->status_vars.common.damage.hitstun_tics = 1u;
        fp->throw_gobj = NULL;
        fp->coll_data.floor_flags = 0u;
        fp->coll_data.floor_angle.x = 0.0F;
        fp->coll_data.floor_angle.y = 1.0F;
        fp->physics.vel_ground.x = 12.0F;
        fp->physics.vel_ground.y = 0.0F;
        fp->physics.vel_ground.z = 0.0F;
        fp->vel_ground = fp->physics.vel_ground;
        vel_before = ndsFloatToMilliSigned(fp->physics.vel_ground.x);
        sNdsFighterDashRunDamagePhysicsActive = TRUE;
        ftCommonDamageCommonProcPhysics(fighter_gobj);
        sNdsFighterDashRunDamagePhysicsActive =
            saved_damage_physics_active;
        vel_after = ndsFloatToMilliSigned(fp->physics.vel_ground.x);
        if ((vel_before == 12000) &&
            (vel_after > 0) &&
            (vel_after < vel_before) &&
            (fp->ga == nMPKineticsGround) &&
            (fp->status_id == nFTCommonStatusDamageN1))
        {
            mask |= 1u << 22u;
        }
        *fp = saved_ground_physics_fp;
        fighter_gobj->anim_frame = saved_ground_physics_anim_frame;
    }

    {
        FTStruct saved_fastfall_physics_fp = *fp;
        f32 saved_fastfall_physics_anim_frame = fighter_gobj->anim_frame;
        s32 vel_y_after;

        attr = *fp->attr;
        attr.air_friction = 0.5F;
        attr.air_accel = 0.0F;
        attr.air_speed_max_x = 40.0F;
        attr.gravity = 1.0F;
        attr.tvel_base = 10.0F;
        attr.tvel_fast = 20.0F;
        fp->attr = &attr;
        fp->proc_physics = ftCommonDamageCommonProcPhysics;
        fp->ga = nMPKineticsAir;
        fp->status_id = nFTCommonStatusDamageAir1;
        fp->motion_id =
            ndsFTCommonDamageMotionForStatus(nFTCommonStatusDamageAir1);
        fp->status_vars.common.damage.hitstun_tics = 0u;
        fp->throw_gobj = NULL;
        fp->is_fastfall = FALSE;
        fp->tap_stick_y = 0u;
        fp->input.pl.stick_range.x = 0;
        fp->input.pl.stick_range.y = FTCOMMON_FASTFALL_STICK_RANGE_MIN;
        fp->physics.vel_air.x = 0.0F;
        fp->physics.vel_air.y = -1.0F;
        fp->physics.vel_air.z = 0.0F;
        fp->vel_air = fp->physics.vel_air;
        ftCommonDamageCommonProcPhysics(fighter_gobj);
        vel_y_after = ndsFloatToMilliSigned(fp->physics.vel_air.y);
        if ((fp->ga == nMPKineticsAir) &&
            (fp->status_id == nFTCommonStatusDamageAir1) &&
            (fp->is_fastfall != FALSE) &&
            (fp->tap_stick_y == FTINPUT_STICKBUFFER_TICS_MAX) &&
            (vel_y_after == -20000))
        {
            mask |= 1u << 23u;
        }
        *fp = saved_fastfall_physics_fp;
        fighter_gobj->anim_frame = saved_fastfall_physics_anim_frame;
    }

    {
        FTStruct saved_drift_physics_fp = *fp;
        f32 saved_drift_physics_anim_frame = fighter_gobj->anim_frame;
        s32 vel_x_after;
        s32 vel_y_after;

        attr = *fp->attr;
        attr.air_friction = 0.5F;
        attr.air_accel = 0.1F;
        attr.air_speed_max_x = 40.0F;
        attr.gravity = 1.0F;
        attr.tvel_base = 10.0F;
        attr.tvel_fast = 20.0F;
        fp->attr = &attr;
        fp->proc_physics = ftCommonDamageCommonProcPhysics;
        fp->ga = nMPKineticsAir;
        fp->status_id = nFTCommonStatusDamageAir1;
        fp->motion_id =
            ndsFTCommonDamageMotionForStatus(nFTCommonStatusDamageAir1);
        fp->status_vars.common.damage.hitstun_tics = 0u;
        fp->throw_gobj = NULL;
        fp->is_fastfall = FALSE;
        fp->tap_stick_y = FTINPUT_STICKBUFFER_TICS_MAX;
        fp->input.pl.stick_range.x = 80;
        fp->input.pl.stick_range.y = 0;
        fp->physics.vel_air.x = 0.0F;
        fp->physics.vel_air.y = 0.0F;
        fp->physics.vel_air.z = 0.0F;
        fp->vel_air = fp->physics.vel_air;
        ftCommonDamageCommonProcPhysics(fighter_gobj);
        vel_x_after = ndsFloatToMilliSigned(fp->physics.vel_air.x);
        vel_y_after = ndsFloatToMilliSigned(fp->physics.vel_air.y);
        if ((fp->ga == nMPKineticsAir) &&
            (fp->status_id == nFTCommonStatusDamageAir1) &&
            (fp->is_fastfall == FALSE) &&
            (vel_x_after > 0) &&
            (vel_x_after < 8000) &&
            (vel_y_after == -1000))
        {
            mask |= 1u << 24u;
        }
        *fp = saved_drift_physics_fp;
        fighter_gobj->anim_frame = saved_drift_physics_anim_frame;
    }

    {
        FTStruct saved_air_clamp_physics_fp = *fp;
        f32 saved_air_clamp_physics_anim_frame = fighter_gobj->anim_frame;
        s32 vel_x_after;
        s32 vel_y_after;

        attr = *fp->attr;
        attr.air_friction = 0.5F;
        attr.air_accel = 0.1F;
        attr.air_speed_max_x = 40.0F;
        attr.gravity = 1.0F;
        attr.tvel_base = 10.0F;
        attr.tvel_fast = 20.0F;
        fp->attr = &attr;
        fp->proc_physics = ftCommonDamageCommonProcPhysics;
        fp->ga = nMPKineticsAir;
        fp->status_id = nFTCommonStatusDamageAir1;
        fp->motion_id =
            ndsFTCommonDamageMotionForStatus(nFTCommonStatusDamageAir1);
        fp->status_vars.common.damage.hitstun_tics = 0u;
        fp->throw_gobj = NULL;
        fp->is_fastfall = FALSE;
        fp->tap_stick_y = FTINPUT_STICKBUFFER_TICS_MAX;
        fp->input.pl.stick_range.x = 80;
        fp->input.pl.stick_range.y = 0;
        fp->physics.vel_air.x = 41.0F;
        fp->physics.vel_air.y = 0.0F;
        fp->physics.vel_air.z = 0.0F;
        fp->vel_air = fp->physics.vel_air;
        ftCommonDamageCommonProcPhysics(fighter_gobj);
        vel_x_after = ndsFloatToMilliSigned(fp->physics.vel_air.x);
        vel_y_after = ndsFloatToMilliSigned(fp->physics.vel_air.y);
        if ((fp->ga == nMPKineticsAir) &&
            (fp->status_id == nFTCommonStatusDamageAir1) &&
            (fp->is_fastfall == FALSE) &&
            (vel_x_after == 40000) &&
            (vel_y_after == -1000))
        {
            mask |= 1u << 25u;
        }
        *fp = saved_air_clamp_physics_fp;
        fighter_gobj->anim_frame = saved_air_clamp_physics_anim_frame;
    }

    fp->status_vars.common.damage.hitstun_tics = 0;
    fp->is_hitstun = TRUE;
    sNdsFighterDashRunDamageFallInterruptCount = 0u;
    sNdsFighterDashRunDamageInterruptActive = TRUE;
    if (fp->proc_interrupt != NULL)
    {
        fp->proc_interrupt(fighter_gobj);
    }
    sNdsFighterDashRunDamageInterruptActive = saved_interrupt_active;
    gNdsStageMPLiveHitStatusLoopCallbackInterruptCount =
        sNdsFighterDashRunDamageFallInterruptCount;
    if ((fp->status_id == status_id) &&
        (fp->motion_id == ndsFTCommonDamageMotionForStatus(status_id)) &&
        (fp->is_hitstun == FALSE) &&
        (gNdsStageMPLiveHitStatusLoopCallbackInterruptCount == 1u))
    {
        mask |= 1u << 3u;
    }

    saved_damage_coll_mask_prev =
        fp->status_vars.common.damage.coll_mask_prev;
    saved_damage_coll_mask_curr =
        fp->status_vars.common.damage.coll_mask_curr;
    saved_damage_coll_mask_ignore =
        fp->status_vars.common.damage.coll_mask_ignore;
    saved_coll_mask_stat = fp->coll_data.mask_stat;
    saved_coll_mask_curr = fp->coll_data.mask_curr;
    sNdsFighterDashRunDamageAirMapCount = 0u;
    sNdsFighterDashRunDamageFallMapCollisionMode = 0u;
    sNdsFighterDashRunDamageFallMapNoCollisionCount = 0u;
    sNdsFighterDashRunDamageFallMapCollisionCount = 0u;
    sNdsFighterDashRunDamageFallPassiveStandCheckCount = 0u;
    sNdsFighterDashRunDamageFallPassiveCheckCount = 0u;
    sNdsFighterDashRunDamageFallDownBounceSetStatusCount = 0u;
    sNdsFighterDashRunDamageMapActive = TRUE;
    if (fp->proc_map != NULL)
    {
        fp->proc_map(fighter_gobj);
    }
    gNdsStageMPLiveHitStatusLoopCallbackMapCount =
        sNdsFighterDashRunDamageAirMapCount;
    gNdsStageMPLiveHitStatusLoopCallbackMapNoCollisionCount =
        sNdsFighterDashRunDamageFallMapNoCollisionCount;
    if ((is_air_status != FALSE) &&
        (fp->status_id == status_id) &&
        (fp->motion_id == ndsFTCommonDamageMotionForStatus(status_id)) &&
        (gNdsStageMPLiveHitStatusLoopCallbackMapCount == 1u) &&
        (gNdsStageMPLiveHitStatusLoopCallbackMapNoCollisionCount == 1u))
    {
        mask |= 1u << 4u;
    }

    sNdsFighterDashRunDamageAirMapCount = 0u;
    sNdsFighterDashRunDamageFallMapCollisionMode = 1u;
    sNdsFighterDashRunDamageFallMapNoCollisionCount = 0u;
    sNdsFighterDashRunDamageFallMapCollisionCount = 0u;
    sNdsFighterDashRunDamageFallPassiveStandCheckCount = 0u;
    sNdsFighterDashRunDamageFallPassiveCheckCount = 0u;
    sNdsFighterDashRunDamageFallDownBounceSetStatusCount = 0u;
    if ((is_air_status != FALSE) && (fp->proc_map != NULL))
    {
        fp->proc_map(fighter_gobj);
    }
    sNdsFighterDashRunDamageMapActive = saved_map_active;
    gNdsStageMPLiveHitStatusLoopCallbackMapCollisionCount =
        sNdsFighterDashRunDamageFallMapCollisionCount;
    gNdsStageMPLiveHitStatusLoopCallbackMapPassiveStandCheckCount =
        sNdsFighterDashRunDamageFallPassiveStandCheckCount;
    gNdsStageMPLiveHitStatusLoopCallbackMapPassiveCheckCount =
        sNdsFighterDashRunDamageFallPassiveCheckCount;
    gNdsStageMPLiveHitStatusLoopCallbackMapDownBounceSetStatusCount =
        sNdsFighterDashRunDamageFallDownBounceSetStatusCount;
    fp->status_vars.common.damage.coll_mask_prev =
        saved_damage_coll_mask_prev;
    fp->status_vars.common.damage.coll_mask_curr =
        saved_damage_coll_mask_curr;
    fp->status_vars.common.damage.coll_mask_ignore =
        saved_damage_coll_mask_ignore;
    fp->coll_data.mask_stat = saved_coll_mask_stat;
    fp->coll_data.mask_curr = saved_coll_mask_curr;
    if ((is_air_status != FALSE) &&
        (fp->status_id == status_id) &&
        (fp->motion_id == ndsFTCommonDamageMotionForStatus(status_id)) &&
        (gNdsStageMPLiveHitStatusLoopCallbackMapCollisionCount == 1u) &&
        (gNdsStageMPLiveHitStatusLoopCallbackMapPassiveStandCheckCount == 1u) &&
        (gNdsStageMPLiveHitStatusLoopCallbackMapPassiveCheckCount == 1u) &&
        (gNdsStageMPLiveHitStatusLoopCallbackMapDownBounceSetStatusCount == 1u))
    {
        mask |= 1u << 7u;
    }

    if ((is_air_status != FALSE) &&
        (fp->proc_map == ftCommonDamageAirCommonProcMap))
    {
        FTStruct saved_map_gate_fp = *fp;
        f32 saved_map_gate_anim_frame = fighter_gobj->anim_frame;
        f32 saved_map_gate_anim_speed = 0.0F;
        u32 map_gate_mask = 0u;

        if (root != NULL)
        {
            saved_map_gate_anim_speed = root->anim_speed;
        }

        sNdsFighterDashRunDamageAirMapCount = 0u;
        sNdsFighterDashRunDamageFallMapCollisionMode = 6u;
        sNdsFighterDashRunDamageFallMapNoCollisionCount = 0u;
        sNdsFighterDashRunDamageFallMapCollisionCount = 0u;
        sNdsFighterDashRunDamageFallPassiveStandCheckCount = 0u;
        sNdsFighterDashRunDamageFallPassiveCheckCount = 0u;
        sNdsFighterDashRunDamageFallDownBounceSetStatusCount = 0u;
        fp->status_id = status_id;
        fp->motion_id = ndsFTCommonDamageMotionForStatus(status_id);
        fp->motion_script_id = fp->motion_id;
        fp->ga = nMPKineticsAir;
        fp->lr = 1;
        fp->tics_since_last_z = 0;
        fp->input.pl.stick_range.x = 80;
        fp->input.pl.stick_range.y = 0;
        sNdsStageMPPassiveLoopBranchProbeActive = TRUE;
        sNdsFighterDashRunDamageMapActive = TRUE;
        fp->proc_map(fighter_gobj);
        sNdsFighterDashRunDamageMapActive = saved_map_active;
        sNdsStageMPPassiveLoopBranchProbeActive =
            saved_passive_branch_probe_active;
        sNdsFighterDashRunDamageFallMapCollisionMode =
            saved_map_collision_mode;
        if ((sNdsFighterDashRunDamageAirMapCount == 1u) &&
            (sNdsFighterDashRunDamageFallMapCollisionCount == 1u) &&
            (sNdsFighterDashRunDamageFallPassiveStandCheckCount == 1u) &&
            (sNdsFighterDashRunDamageFallPassiveCheckCount == 0u) &&
            (sNdsFighterDashRunDamageFallDownBounceSetStatusCount == 0u) &&
            (fp->status_id == nFTCommonStatusPassiveStandF) &&
            (fp->motion_id == nFTCommonMotionPassiveStandF) &&
            (fp->ga == nMPKineticsGround))
        {
            map_gate_mask |= 1u;
        }

        *fp = saved_map_gate_fp;
        fighter_gobj->anim_frame = saved_map_gate_anim_frame;
        if (root != NULL)
        {
            root->anim_speed = saved_map_gate_anim_speed;
        }

        sNdsFighterDashRunDamageAirMapCount = 0u;
        sNdsFighterDashRunDamageFallMapCollisionMode = 7u;
        sNdsFighterDashRunDamageFallMapNoCollisionCount = 0u;
        sNdsFighterDashRunDamageFallMapCollisionCount = 0u;
        sNdsFighterDashRunDamageFallPassiveStandCheckCount = 0u;
        sNdsFighterDashRunDamageFallPassiveCheckCount = 0u;
        sNdsFighterDashRunDamageFallDownBounceSetStatusCount = 0u;
        fp->status_id = status_id;
        fp->motion_id = ndsFTCommonDamageMotionForStatus(status_id);
        fp->motion_script_id = fp->motion_id;
        fp->ga = nMPKineticsAir;
        fp->lr = 1;
        fp->tics_since_last_z = 0;
        fp->input.pl.stick_range.x = 0;
        fp->input.pl.stick_range.y = 0;
        sNdsStageMPPassiveLoopBranchProbeActive = TRUE;
        sNdsFighterDashRunDamageMapActive = TRUE;
        fp->proc_map(fighter_gobj);
        sNdsFighterDashRunDamageMapActive = saved_map_active;
        sNdsStageMPPassiveLoopBranchProbeActive =
            saved_passive_branch_probe_active;
        sNdsFighterDashRunDamageFallMapCollisionMode =
            saved_map_collision_mode;
        if ((sNdsFighterDashRunDamageAirMapCount == 1u) &&
            (sNdsFighterDashRunDamageFallMapCollisionCount == 1u) &&
            (sNdsFighterDashRunDamageFallPassiveStandCheckCount == 1u) &&
            (sNdsFighterDashRunDamageFallPassiveCheckCount == 1u) &&
            (sNdsFighterDashRunDamageFallDownBounceSetStatusCount == 0u) &&
            (fp->status_id == nFTCommonStatusPassive) &&
            (fp->motion_id == nFTCommonMotionPassive) &&
            (fp->ga == nMPKineticsGround))
        {
            map_gate_mask |= 1u << 1u;
        }

        if (map_gate_mask == 0x3u)
        {
            mask |= 1u << 27u;
        }

        *fp = saved_map_gate_fp;
        fighter_gobj->anim_frame = saved_map_gate_anim_frame;
        if (root != NULL)
        {
            root->anim_speed = saved_map_gate_anim_speed;
        }
    }

    if ((is_air_status != FALSE) &&
        (fp->proc_map == ftCommonDamageAirCommonProcMap))
    {
        FTStruct saved_air_nofloor_fp = *fp;
        f32 saved_air_nofloor_anim_frame = fighter_gobj->anim_frame;
        f32 saved_air_nofloor_anim_speed = 0.0F;

        if (root != NULL)
        {
            saved_air_nofloor_anim_speed = root->anim_speed;
        }

        sNdsFighterDashRunDamageAirMapCount = 0u;
        sNdsFighterDashRunDamageFallMapCollisionMode = 8u;
        sNdsFighterDashRunDamageFallMapNoCollisionCount = 0u;
        sNdsFighterDashRunDamageFallMapCollisionCount = 0u;
        sNdsFighterDashRunDamageFallPassiveStandCheckCount = 0u;
        sNdsFighterDashRunDamageFallPassiveCheckCount = 0u;
        sNdsFighterDashRunDamageFallDownBounceSetStatusCount = 0u;
        fp->status_id = status_id;
        fp->motion_id = ndsFTCommonDamageMotionForStatus(status_id);
        fp->motion_script_id = fp->motion_id;
        fp->ga = nMPKineticsAir;
        fp->status_vars.common.damage.coll_mask_curr = MAP_FLAG_FLOOR;
        fp->coll_data.mask_stat = MAP_FLAG_FLOOR;
        fp->coll_data.mask_curr = MAP_FLAG_FLOOR;
        sNdsFighterDashRunDamageMapActive = TRUE;
        fp->proc_map(fighter_gobj);
        sNdsFighterDashRunDamageMapActive = saved_map_active;
        sNdsFighterDashRunDamageFallMapCollisionMode =
            saved_map_collision_mode;
        if ((sNdsFighterDashRunDamageAirMapCount == 1u) &&
            (sNdsFighterDashRunDamageFallMapCollisionCount == 1u) &&
            (sNdsFighterDashRunDamageFallPassiveStandCheckCount == 0u) &&
            (sNdsFighterDashRunDamageFallPassiveCheckCount == 0u) &&
            (sNdsFighterDashRunDamageFallDownBounceSetStatusCount == 0u) &&
            ((fp->status_vars.common.damage.coll_mask_curr &
              MAP_FLAG_FLOOR) == 0u) &&
            (fp->status_id == status_id) &&
            (fp->motion_id == ndsFTCommonDamageMotionForStatus(status_id)) &&
            (fp->ga == nMPKineticsAir))
        {
            mask |= 1u << 29u;
        }

        *fp = saved_air_nofloor_fp;
        fighter_gobj->anim_frame = saved_air_nofloor_anim_frame;
        if (root != NULL)
        {
            root->anim_speed = saved_air_nofloor_anim_speed;
        }
    }

    if ((is_air_status != FALSE) && (root != NULL) &&
        (fp->proc_map == ftCommonDamageAirCommonProcMap))
    {
        FTStruct saved_wall_fp = *fp;
        Vec3f saved_wall_translate = root->translate.vec.f;
        f32 saved_wall_anim_frame = fighter_gobj->anim_frame;
        f32 saved_wall_anim_speed = root->anim_speed;
        FTStruct saved_ceil_fp;
        Vec3f saved_ceil_translate;
        f32 saved_ceil_anim_frame;
        f32 saved_ceil_anim_speed;
        u32 ceil_mask;
        FTStruct saved_rwall_fp;
        Vec3f saved_rwall_translate;
        f32 saved_rwall_anim_frame;
        f32 saved_rwall_anim_speed;
        u32 rwall_mask;
        u32 wall_mask = 0u;

        root->translate.vec.f.x = 0.0F;
        root->translate.vec.f.y = 0.0F;
        root->translate.vec.f.z = 0.0F;
        fp->status_id = status_id;
        fp->motion_id = ndsFTCommonDamageMotionForStatus(status_id);
        fp->motion_script_id = fp->motion_id;
        fp->ga = nMPKineticsAir;
        fp->lr = +1;
        fp->coll_data.map_coll.width = 30.0F;
        fp->coll_data.map_coll.center = 0.0F;
        fp->coll_data.map_coll.top = 60.0F;
        fp->coll_data.map_coll.bottom = 0.0F;
        fp->coll_data.p_map_coll = &fp->coll_data.map_coll;
        fp->coll_data.lwall_angle.x = 1.0F;
        fp->coll_data.lwall_angle.y = 0.0F;
        fp->coll_data.lwall_angle.z = 0.0F;
        fp->coll_data.mask_curr = 0u;
        fp->coll_data.mask_stat = 0u;
        fp->status_vars.common.damage.coll_mask_curr = 0u;
        fp->status_vars.common.damage.coll_mask_prev = 0u;
        fp->status_vars.common.damage.public_knockback = 6400.0F;
        fp->physics.vel_air.x = -12.0F;
        fp->physics.vel_air.y = 0.0F;
        fp->physics.vel_air.z = 0.0F;
        fp->physics.vel_damage_air.x = 0.0F;
        fp->physics.vel_damage_air.y = 0.0F;
        fp->physics.vel_damage_air.z = 0.0F;

        sNdsFighterDashRunDamageAirMapCount = 0u;
        sNdsFighterDashRunDamageFallMapCollisionMode = 3u;
        sNdsFighterDashRunDamageFallMapCollisionCount = 0u;
        sNdsFighterDashRunDamageFallPassiveStandCheckCount = 0u;
        sNdsFighterDashRunDamageFallPassiveCheckCount = 0u;
        sNdsFighterDashRunDamageFallDownBounceSetStatusCount = 0u;
        sNdsFighterDashRunDamageMapActive = TRUE;
        fp->proc_map(fighter_gobj);
        sNdsFighterDashRunDamageMapActive = saved_map_active;
        sNdsFighterDashRunDamageFallMapCollisionMode = 0u;

        if ((sNdsFighterDashRunDamageAirMapCount == 1u) &&
            (sNdsFighterDashRunDamageFallMapCollisionCount == 1u) &&
            ((fp->status_vars.common.damage.coll_mask_curr &
              MAP_FLAG_LWALL) != 0u))
        {
            wall_mask |= 1u;
        }
        if ((fp->damage_knockback_stack > 0.0F) &&
            (fp->intangible_tics >= FTCOMMON_WALLDAMAGE_INTANGIBLE_TIMER))
        {
            wall_mask |= 1u << 1u;
        }
        if ((sNdsFighterDashRunDamageFallPassiveStandCheckCount == 0u) &&
            (sNdsFighterDashRunDamageFallPassiveCheckCount == 0u) &&
            (sNdsFighterDashRunDamageFallDownBounceSetStatusCount == 0u))
        {
            wall_mask |= 1u << 2u;
        }
        if ((fp->physics.vel_damage_air.x > 0.0F) && (fp->lr == -1))
        {
            wall_mask |= 1u << 3u;
        }
        if ((sNdsFighterDashRunDamageAirMapCount == 1u) &&
            ((wall_mask & 0xfu) == 0xfu))
        {
            wall_mask |= 1u << 5u;
        }

        *fp = saved_wall_fp;
        root->translate.vec.f = saved_wall_translate;
        fighter_gobj->anim_frame = saved_wall_anim_frame;
        root->anim_speed = saved_wall_anim_speed;
        if ((fp->status_id == saved_wall_fp.status_id) &&
            (fp->motion_id == saved_wall_fp.motion_id) &&
            (fp->ga == saved_wall_fp.ga))
        {
            wall_mask |= 1u << 4u;
        }

        saved_ceil_fp = *fp;
        saved_ceil_translate = root->translate.vec.f;
        saved_ceil_anim_frame = fighter_gobj->anim_frame;
        saved_ceil_anim_speed = root->anim_speed;
        ceil_mask = 0u;

        root->translate.vec.f.x = 0.0F;
        root->translate.vec.f.y = 0.0F;
        root->translate.vec.f.z = 0.0F;
        fp->status_id = status_id;
        fp->motion_id = ndsFTCommonDamageMotionForStatus(status_id);
        fp->motion_script_id = fp->motion_id;
        fp->ga = nMPKineticsAir;
        fp->lr = +1;
        fp->coll_data.map_coll.width = 30.0F;
        fp->coll_data.map_coll.center = 0.0F;
        fp->coll_data.map_coll.top = 60.0F;
        fp->coll_data.map_coll.bottom = 0.0F;
        fp->coll_data.p_map_coll = &fp->coll_data.map_coll;
        fp->coll_data.ceil_angle.x = 0.0F;
        fp->coll_data.ceil_angle.y = -1.0F;
        fp->coll_data.ceil_angle.z = 0.0F;
        fp->coll_data.mask_curr = 0u;
        fp->coll_data.mask_stat = 0u;
        fp->status_vars.common.damage.coll_mask_curr = 0u;
        fp->status_vars.common.damage.coll_mask_prev = 0u;
        fp->status_vars.common.damage.public_knockback = 6400.0F;
        fp->physics.vel_air.x = 0.0F;
        fp->physics.vel_air.y = 12.0F;
        fp->physics.vel_air.z = 0.0F;
        fp->physics.vel_damage_air.x = 0.0F;
        fp->physics.vel_damage_air.y = 0.0F;
        fp->physics.vel_damage_air.z = 0.0F;

        sNdsFighterDashRunDamageAirMapCount = 0u;
        sNdsFighterDashRunDamageFallMapCollisionMode = 4u;
        sNdsFighterDashRunDamageFallMapCollisionCount = 0u;
        sNdsFighterDashRunDamageFallPassiveStandCheckCount = 0u;
        sNdsFighterDashRunDamageFallPassiveCheckCount = 0u;
        sNdsFighterDashRunDamageFallDownBounceSetStatusCount = 0u;
        sNdsFighterDashRunDamageMapActive = TRUE;
        fp->proc_map(fighter_gobj);
        sNdsFighterDashRunDamageMapActive = saved_map_active;
        sNdsFighterDashRunDamageFallMapCollisionMode = 0u;

        if ((sNdsFighterDashRunDamageAirMapCount == 1u) &&
            (sNdsFighterDashRunDamageFallMapCollisionCount == 1u) &&
            ((fp->status_vars.common.damage.coll_mask_curr &
              MAP_FLAG_CEIL) != 0u))
        {
            ceil_mask |= 1u;
        }
        if ((fp->damage_knockback_stack > 0.0F) &&
            (fp->intangible_tics >= FTCOMMON_WALLDAMAGE_INTANGIBLE_TIMER))
        {
            ceil_mask |= 1u << 1u;
        }
        if ((sNdsFighterDashRunDamageFallPassiveStandCheckCount == 0u) &&
            (sNdsFighterDashRunDamageFallPassiveCheckCount == 0u) &&
            (sNdsFighterDashRunDamageFallDownBounceSetStatusCount == 0u))
        {
            ceil_mask |= 1u << 2u;
        }
        if ((fp->physics.vel_damage_air.y < 0.0F) && (fp->lr == -1))
        {
            ceil_mask |= 1u << 3u;
        }
        if ((sNdsFighterDashRunDamageAirMapCount == 1u) &&
            ((ceil_mask & 0xfu) == 0xfu))
        {
            ceil_mask |= 1u << 5u;
        }

        *fp = saved_ceil_fp;
        root->translate.vec.f = saved_ceil_translate;
        fighter_gobj->anim_frame = saved_ceil_anim_frame;
        root->anim_speed = saved_ceil_anim_speed;
        if ((fp->status_id == saved_ceil_fp.status_id) &&
            (fp->motion_id == saved_ceil_fp.motion_id) &&
            (fp->ga == saved_ceil_fp.ga))
        {
            ceil_mask |= 1u << 4u;
        }
        wall_mask |= (ceil_mask & 0x3fu) << 6u;

        saved_rwall_fp = *fp;
        saved_rwall_translate = root->translate.vec.f;
        saved_rwall_anim_frame = fighter_gobj->anim_frame;
        saved_rwall_anim_speed = root->anim_speed;
        rwall_mask = 0u;

        root->translate.vec.f.x = 0.0F;
        root->translate.vec.f.y = 0.0F;
        root->translate.vec.f.z = 0.0F;
        fp->status_id = status_id;
        fp->motion_id = ndsFTCommonDamageMotionForStatus(status_id);
        fp->motion_script_id = fp->motion_id;
        fp->ga = nMPKineticsAir;
        fp->lr = -1;
        fp->coll_data.map_coll.width = 30.0F;
        fp->coll_data.map_coll.center = 0.0F;
        fp->coll_data.map_coll.top = 60.0F;
        fp->coll_data.map_coll.bottom = 0.0F;
        fp->coll_data.p_map_coll = &fp->coll_data.map_coll;
        fp->coll_data.rwall_angle.x = -1.0F;
        fp->coll_data.rwall_angle.y = 0.0F;
        fp->coll_data.rwall_angle.z = 0.0F;
        fp->coll_data.mask_curr = 0u;
        fp->coll_data.mask_stat = 0u;
        fp->status_vars.common.damage.coll_mask_curr = 0u;
        fp->status_vars.common.damage.coll_mask_prev = 0u;
        fp->status_vars.common.damage.public_knockback = 6400.0F;
        fp->physics.vel_air.x = 12.0F;
        fp->physics.vel_air.y = 0.0F;
        fp->physics.vel_air.z = 0.0F;
        fp->physics.vel_damage_air.x = 0.0F;
        fp->physics.vel_damage_air.y = 0.0F;
        fp->physics.vel_damage_air.z = 0.0F;

        sNdsFighterDashRunDamageAirMapCount = 0u;
        sNdsFighterDashRunDamageFallMapCollisionMode = 5u;
        sNdsFighterDashRunDamageFallMapCollisionCount = 0u;
        sNdsFighterDashRunDamageFallPassiveStandCheckCount = 0u;
        sNdsFighterDashRunDamageFallPassiveCheckCount = 0u;
        sNdsFighterDashRunDamageFallDownBounceSetStatusCount = 0u;
        sNdsFighterDashRunDamageMapActive = TRUE;
        fp->proc_map(fighter_gobj);
        sNdsFighterDashRunDamageMapActive = saved_map_active;
        sNdsFighterDashRunDamageFallMapCollisionMode = 0u;

        if ((sNdsFighterDashRunDamageAirMapCount == 1u) &&
            (sNdsFighterDashRunDamageFallMapCollisionCount == 1u) &&
            ((fp->status_vars.common.damage.coll_mask_curr &
              MAP_FLAG_RWALL) != 0u))
        {
            rwall_mask |= 1u;
        }
        if ((fp->damage_knockback_stack > 0.0F) &&
            (fp->intangible_tics >= FTCOMMON_WALLDAMAGE_INTANGIBLE_TIMER))
        {
            rwall_mask |= 1u << 1u;
        }
        if ((sNdsFighterDashRunDamageFallPassiveStandCheckCount == 0u) &&
            (sNdsFighterDashRunDamageFallPassiveCheckCount == 0u) &&
            (sNdsFighterDashRunDamageFallDownBounceSetStatusCount == 0u))
        {
            rwall_mask |= 1u << 2u;
        }
        if ((fp->physics.vel_damage_air.x < 0.0F) && (fp->lr == +1))
        {
            rwall_mask |= 1u << 3u;
        }
        if ((sNdsFighterDashRunDamageAirMapCount == 1u) &&
            ((rwall_mask & 0xfu) == 0xfu))
        {
            rwall_mask |= 1u << 5u;
        }

        *fp = saved_rwall_fp;
        root->translate.vec.f = saved_rwall_translate;
        fighter_gobj->anim_frame = saved_rwall_anim_frame;
        root->anim_speed = saved_rwall_anim_speed;
        if ((fp->status_id == saved_rwall_fp.status_id) &&
            (fp->motion_id == saved_rwall_fp.motion_id) &&
            (fp->ga == saved_rwall_fp.ga))
        {
            rwall_mask |= 1u << 4u;
        }
        wall_mask |= (rwall_mask & 0x3fu) << 12u;
        gNdsStageMPLiveHitStatusLoopCallbackMapWallMask = wall_mask;
        if ((wall_mask & 0x3ffffu) == 0x3ffffu)
        {
            mask |= 1u << 8u;
        }
    }

    fp->status_vars.common.damage.hitstun_tics = 1;
    fp->status_vars.common.damage.public_knockback = 23.0F;
    fp->status_vars.common.damage.dust_effect_int = 0;
    fp->public_knockback = 0.0F;
    fighter_gobj->anim_frame = 0.0F;
    sNdsFighterDashRunDamageFallSetStatusCount = 0u;
    if (is_air_status != FALSE)
    {
        sNdsFighterDashRunDamageExpiryActive = TRUE;
    }
    if (fp->proc_update != NULL)
    {
        fp->proc_update(fighter_gobj);
    }
    sNdsFighterDashRunDamageExpiryActive = saved_expiry_active;
    gNdsStageMPLiveHitStatusLoopCallbackEndStatus = (u32)fp->status_id;
    gNdsStageMPLiveHitStatusLoopCallbackEndMotion = (u32)fp->motion_id;
    gNdsStageMPLiveHitStatusLoopCallbackEndPublicKnockbackMilli =
        ndsFloatToMilliSigned(fp->public_knockback);
    if ((fp->status_id == end_status_id) &&
        (fp->motion_id == end_motion_id) &&
        (fp->status_vars.common.damage.hitstun_tics == 0) &&
        (gNdsStageMPLiveHitStatusLoopCallbackEndPublicKnockbackMilli ==
            23000) &&
        ((is_air_status == FALSE) ||
         (sNdsFighterDashRunDamageFallSetStatusCount == 1u)))
    {
        mask |= 1u << 5u;
    }
    if ((is_air_status != FALSE) &&
        (fp->status_id == nFTCommonStatusDamageFall) &&
        (fp->proc_map == ftCommonDamageFallProcMap))
    {
        FTStruct saved_fall_map_fp = *fp;
        Vec3f saved_fall_map_translate = { 0.0F, 0.0F, 0.0F };
        f32 saved_fall_map_anim_frame = fighter_gobj->anim_frame;
        f32 saved_fall_map_anim_speed = 0.0F;
        u32 cliff_mask = 0u;
        u32 fall_mask = 0u;

        if (root != NULL)
        {
            saved_fall_map_translate = root->translate.vec.f;
            saved_fall_map_anim_speed = root->anim_speed;
        }

        sNdsFighterDashRunDamageFallMapCollisionMode = 0u;
        sNdsFighterDashRunDamageFallMapCount = 0u;
        sNdsFighterDashRunDamageFallMapNoCollisionCount = 0u;
        sNdsFighterDashRunDamageFallMapCollisionCount = 0u;
        sNdsFighterDashRunDamageFallPassiveStandCheckCount = 0u;
        sNdsFighterDashRunDamageFallPassiveCheckCount = 0u;
        sNdsFighterDashRunDamageFallDownBounceSetStatusCount = 0u;
        sNdsFighterDashRunDamageFallCliffCatchSetStatusCount = 0u;
        sNdsFighterDashRunDamageMapActive = TRUE;
        fp->proc_map(fighter_gobj);
        sNdsFighterDashRunDamageMapActive = saved_map_active;
        if ((sNdsFighterDashRunDamageFallMapCount == 1u) &&
            (sNdsFighterDashRunDamageFallMapNoCollisionCount == 1u) &&
            (fp->status_id == nFTCommonStatusDamageFall) &&
            (fp->motion_id == nFTCommonMotionDamageFall) &&
            (fp->ga == nMPKineticsAir))
        {
            fall_mask |= 1u;
        }

        *fp = saved_fall_map_fp;
        fighter_gobj->anim_frame = saved_fall_map_anim_frame;
        if (root != NULL)
        {
            root->translate.vec.f = saved_fall_map_translate;
            root->anim_speed = saved_fall_map_anim_speed;
        }

        sNdsFighterDashRunDamageFallMapCollisionMode = 2u;
        sNdsFighterDashRunDamageFallMapCount = 0u;
        sNdsFighterDashRunDamageFallMapNoCollisionCount = 0u;
        sNdsFighterDashRunDamageFallMapCollisionCount = 0u;
        sNdsFighterDashRunDamageFallPassiveStandCheckCount = 0u;
        sNdsFighterDashRunDamageFallPassiveCheckCount = 0u;
        sNdsFighterDashRunDamageFallDownBounceSetStatusCount = 0u;
        sNdsFighterDashRunDamageFallCliffCatchSetStatusCount = 0u;
        sNdsFighterDashRunDamageMapActive = TRUE;
        fp->proc_map(fighter_gobj);
        sNdsFighterDashRunDamageMapActive = saved_map_active;
        sNdsFighterDashRunDamageFallMapCollisionMode =
            saved_map_collision_mode;

        if ((sNdsFighterDashRunDamageFallMapCount == 1u) &&
            (sNdsFighterDashRunDamageFallMapCollisionCount == 1u) &&
            ((fp->coll_data.mask_stat & MAP_FLAG_CLIFF_MASK) != 0u))
        {
            cliff_mask |= 1u;
        }
        if (sNdsFighterDashRunDamageFallCliffCatchSetStatusCount == 1u)
        {
            cliff_mask |= 1u << 1u;
        }
        if ((sNdsFighterDashRunDamageFallPassiveStandCheckCount == 0u) &&
            (sNdsFighterDashRunDamageFallPassiveCheckCount == 0u) &&
            (sNdsFighterDashRunDamageFallDownBounceSetStatusCount == 0u))
        {
            cliff_mask |= 1u << 2u;
        }
        if ((fp->status_id == nFTCommonStatusDamageFall) &&
            (fp->motion_id == nFTCommonMotionDamageFall) &&
            (fp->ga == nMPKineticsAir))
        {
            cliff_mask |= 1u << 3u;
        }
        if ((sNdsFighterDashRunDamageFallMapCount == 1u) &&
            ((cliff_mask & 0xfu) == 0xfu))
        {
            cliff_mask |= 1u << 4u;
        }

        *fp = saved_fall_map_fp;
        fighter_gobj->anim_frame = saved_fall_map_anim_frame;
        if (root != NULL)
        {
            root->translate.vec.f = saved_fall_map_translate;
            root->anim_speed = saved_fall_map_anim_speed;
            if (fp->joints[nFTPartsJointTransN] == NULL)
            {
                fp->joints[nFTPartsJointTransN] = root;
            }
        }

        sNdsFighterDashRunDamageFallMapCollisionMode = 2u;
        sNdsFighterDashRunDamageFallMapCount = 0u;
        sNdsFighterDashRunDamageFallMapNoCollisionCount = 0u;
        sNdsFighterDashRunDamageFallMapCollisionCount = 0u;
        sNdsFighterDashRunDamageFallPassiveStandCheckCount = 0u;
        sNdsFighterDashRunDamageFallPassiveCheckCount = 0u;
        sNdsFighterDashRunDamageFallDownBounceSetStatusCount = 0u;
        sNdsFighterDashRunDamageFallCliffCatchSetStatusCount = 0u;
        fp->ga = nMPKineticsAir;
        fp->lr = -1;
        if (fp->coll_data.cliff_id < 0)
        {
            fp->coll_data.cliff_id =
                (fp->coll_data.floor_line_id >= 0) ?
                    fp->coll_data.floor_line_id : 0;
        }
        fp->coll_data.floor_line_id = fp->coll_data.cliff_id;
        fp->vel_air.x = 7.0F;
        fp->vel_ground.x = -3.0F;
        fp->physics.vel_air = fp->vel_air;
        fp->physics.vel_ground = fp->vel_ground;
        sNdsStageMPLiveHitStatusLoopCliffCatchSetStatusActive = TRUE;
        sNdsFighterDashRunDamageMapActive = TRUE;
        fp->proc_map(fighter_gobj);
        sNdsFighterDashRunDamageMapActive = saved_map_active;
        sNdsStageMPLiveHitStatusLoopCliffCatchSetStatusActive =
            saved_livehit_cliffcatch_set_status_active;
        sNdsFighterDashRunDamageFallMapCollisionMode =
            saved_map_collision_mode;

        if ((sNdsFighterDashRunDamageFallMapCount == 1u) &&
            (sNdsFighterDashRunDamageFallMapCollisionCount == 1u) &&
            (sNdsFighterDashRunDamageFallCliffCatchSetStatusCount == 1u) &&
            (fp->status_id == nFTCommonStatusCliffCatch) &&
            (fp->motion_id == nFTCommonMotionCliffCatch) &&
            (fp->ga == nMPKineticsAir) &&
            (fp->coll_data.floor_line_id == -1) &&
            (fp->is_cliff_hold != FALSE) &&
            (fp->proc_update == ftCommonCliffCatchProcUpdate) &&
            (fp->proc_interrupt == NULL) &&
            (fp->proc_physics == ftCommonCliffCommonProcPhysics) &&
            (fp->proc_map == ftCommonCliffCommonProcMap) &&
            (fp->proc_damage == ftCommonCliffCommonProcDamage) &&
            (fp->capture_immune_mask == FTCATCHKIND_MASK_TARUCANN) &&
            (ndsFloatToMilliSigned(fp->physics.vel_air.x) == 0) &&
            (ndsFloatToMilliSigned(fp->physics.vel_ground.x) == 0))
        {
            cliff_mask |= 1u << 5u;
        }
        gNdsStageMPLiveHitStatusLoopCallbackMapCliffMask = cliff_mask;

        *fp = saved_fall_map_fp;
        fighter_gobj->anim_frame = saved_fall_map_anim_frame;
        if (root != NULL)
        {
            root->translate.vec.f = saved_fall_map_translate;
            root->anim_speed = saved_fall_map_anim_speed;
        }

        sNdsFighterDashRunDamageFallMapCollisionMode = 1u;
        sNdsFighterDashRunDamageFallMapCount = 0u;
        sNdsFighterDashRunDamageFallMapNoCollisionCount = 0u;
        sNdsFighterDashRunDamageFallMapCollisionCount = 0u;
        sNdsFighterDashRunDamageFallPassiveStandCheckCount = 0u;
        sNdsFighterDashRunDamageFallPassiveCheckCount = 0u;
        sNdsFighterDashRunDamageFallDownBounceSetStatusCount = 0u;
        sNdsFighterDashRunDamageFallCliffCatchSetStatusCount = 0u;
        sNdsFighterDashRunDamageMapActive = TRUE;
        fp->proc_map(fighter_gobj);
        sNdsFighterDashRunDamageMapActive = saved_map_active;
        sNdsFighterDashRunDamageFallMapCollisionMode =
            saved_map_collision_mode;

        if ((sNdsFighterDashRunDamageFallMapCount == 1u) &&
            (sNdsFighterDashRunDamageFallMapCollisionCount == 1u) &&
            ((fp->coll_data.mask_stat & MAP_FLAG_FLOOR) != 0u) &&
            (sNdsFighterDashRunDamageFallCliffCatchSetStatusCount == 0u))
        {
            fall_mask |= 1u << 1u;
        }
        if (sNdsFighterDashRunDamageFallPassiveStandCheckCount == 1u)
        {
            fall_mask |= 1u << 2u;
        }
        if (sNdsFighterDashRunDamageFallPassiveCheckCount == 1u)
        {
            fall_mask |= 1u << 3u;
        }
        if (sNdsFighterDashRunDamageFallDownBounceSetStatusCount == 1u)
        {
            fall_mask |= 1u << 4u;
        }
        if ((fp->status_id == nFTCommonStatusDamageFall) &&
            (fp->motion_id == nFTCommonMotionDamageFall) &&
            (fp->ga == nMPKineticsAir))
        {
            fall_mask |= 1u << 5u;
        }
        if ((sNdsFighterDashRunDamageFallMapCount == 1u) &&
            ((fall_mask & 0x3fu) == 0x3fu))
        {
            fall_mask |= 1u << 6u;
        }

        *fp = saved_fall_map_fp;
        fighter_gobj->anim_frame = saved_fall_map_anim_frame;
        if (root != NULL)
        {
            root->translate.vec.f = saved_fall_map_translate;
            root->anim_speed = saved_fall_map_anim_speed;
        }

        sNdsFighterDashRunDamageFallMapCollisionMode = 1u;
        sNdsFighterDashRunDamageFallMapCount = 0u;
        sNdsFighterDashRunDamageFallMapNoCollisionCount = 0u;
        sNdsFighterDashRunDamageFallMapCollisionCount = 0u;
        sNdsFighterDashRunDamageFallPassiveStandCheckCount = 0u;
        sNdsFighterDashRunDamageFallPassiveCheckCount = 0u;
        sNdsFighterDashRunDamageFallDownBounceSetStatusCount = 0u;
        sNdsFighterDashRunDamageFallCliffCatchSetStatusCount = 0u;
        fp->damage_mul = 1.0F;
        fp->status_vars.common.downbounce.attack_buffer =
            FTCOMMON_DOWNBOUNCE_ATTACK_BUFFER;
        sNdsStageMPLiveHitStatusLoopDownBounceSetStatusActive = TRUE;
        sNdsFighterDashRunDamageMapActive = TRUE;
        fp->proc_map(fighter_gobj);
        sNdsFighterDashRunDamageMapActive = saved_map_active;
        sNdsStageMPLiveHitStatusLoopDownBounceSetStatusActive =
            saved_livehit_downbounce_set_status_active;
        sNdsFighterDashRunDamageFallMapCollisionMode =
            saved_map_collision_mode;

        if ((sNdsFighterDashRunDamageFallMapCount == 1u) &&
            (sNdsFighterDashRunDamageFallMapCollisionCount == 1u) &&
            (sNdsFighterDashRunDamageFallPassiveStandCheckCount == 1u) &&
            (sNdsFighterDashRunDamageFallPassiveCheckCount == 1u) &&
            (sNdsFighterDashRunDamageFallDownBounceSetStatusCount == 1u) &&
            (((fp->status_id == nFTCommonStatusDownBounceD) &&
              (fp->motion_id == nFTCommonMotionDownBounceD)) ||
             ((fp->status_id == nFTCommonStatusDownBounceU) &&
              (fp->motion_id == nFTCommonMotionDownBounceU))) &&
            (fp->ga == nMPKineticsGround) &&
            (fp->status_vars.common.downbounce.attack_buffer == 0u) &&
            (ndsFloatToMilliSigned(fp->damage_mul) == 500) &&
            (fp->proc_update == ftCommonDownBounceProcUpdate) &&
            (fp->proc_physics == ftPhysicsApplyGroundVelFriction) &&
            (fp->proc_map == mpCommonSetFighterFallOnGroundBreak))
        {
            fall_mask |= 1u << 7u;
        }

        *fp = saved_fall_map_fp;
        fighter_gobj->anim_frame = saved_fall_map_anim_frame;
        if (root != NULL)
        {
            root->translate.vec.f = saved_fall_map_translate;
            root->anim_speed = saved_fall_map_anim_speed;
        }

        sNdsFighterDashRunDamageFallMapCollisionMode = 6u;
        sNdsFighterDashRunDamageFallMapCount = 0u;
        sNdsFighterDashRunDamageFallMapNoCollisionCount = 0u;
        sNdsFighterDashRunDamageFallMapCollisionCount = 0u;
        sNdsFighterDashRunDamageFallPassiveStandCheckCount = 0u;
        sNdsFighterDashRunDamageFallPassiveCheckCount = 0u;
        sNdsFighterDashRunDamageFallDownBounceSetStatusCount = 0u;
        sNdsFighterDashRunDamageFallCliffCatchSetStatusCount = 0u;
        fp->status_id = nFTCommonStatusDamageFall;
        fp->motion_id = nFTCommonMotionDamageFall;
        fp->motion_script_id = nFTCommonMotionDamageFall;
        fp->ga = nMPKineticsAir;
        fp->lr = 1;
        fp->tics_since_last_z = 0;
        fp->input.pl.stick_range.x = 80;
        fp->input.pl.stick_range.y = 0;
        sNdsStageMPPassiveLoopBranchProbeActive = TRUE;
        sNdsFighterDashRunDamageMapActive = TRUE;
        fp->proc_map(fighter_gobj);
        sNdsFighterDashRunDamageMapActive = saved_map_active;
        sNdsStageMPPassiveLoopBranchProbeActive =
            saved_passive_branch_probe_active;
        sNdsFighterDashRunDamageFallMapCollisionMode =
            saved_map_collision_mode;

        if ((sNdsFighterDashRunDamageFallMapCount == 1u) &&
            (sNdsFighterDashRunDamageFallMapCollisionCount == 1u) &&
            (sNdsFighterDashRunDamageFallPassiveStandCheckCount == 1u) &&
            (sNdsFighterDashRunDamageFallPassiveCheckCount == 0u) &&
            (sNdsFighterDashRunDamageFallDownBounceSetStatusCount == 0u) &&
            (sNdsFighterDashRunDamageFallCliffCatchSetStatusCount == 0u) &&
            (fp->status_id == nFTCommonStatusPassiveStandF) &&
            (fp->motion_id == nFTCommonMotionPassiveStandF) &&
            (fp->ga == nMPKineticsGround))
        {
            fall_mask |= 1u << 8u;
        }

        *fp = saved_fall_map_fp;
        fighter_gobj->anim_frame = saved_fall_map_anim_frame;
        if (root != NULL)
        {
            root->translate.vec.f = saved_fall_map_translate;
            root->anim_speed = saved_fall_map_anim_speed;
        }

        sNdsFighterDashRunDamageFallMapCollisionMode = 7u;
        sNdsFighterDashRunDamageFallMapCount = 0u;
        sNdsFighterDashRunDamageFallMapNoCollisionCount = 0u;
        sNdsFighterDashRunDamageFallMapCollisionCount = 0u;
        sNdsFighterDashRunDamageFallPassiveStandCheckCount = 0u;
        sNdsFighterDashRunDamageFallPassiveCheckCount = 0u;
        sNdsFighterDashRunDamageFallDownBounceSetStatusCount = 0u;
        sNdsFighterDashRunDamageFallCliffCatchSetStatusCount = 0u;
        fp->status_id = nFTCommonStatusDamageFall;
        fp->motion_id = nFTCommonMotionDamageFall;
        fp->motion_script_id = nFTCommonMotionDamageFall;
        fp->ga = nMPKineticsAir;
        fp->lr = 1;
        fp->tics_since_last_z = 0;
        fp->input.pl.stick_range.x = 0;
        fp->input.pl.stick_range.y = 0;
        sNdsStageMPPassiveLoopBranchProbeActive = TRUE;
        sNdsFighterDashRunDamageMapActive = TRUE;
        fp->proc_map(fighter_gobj);
        sNdsFighterDashRunDamageMapActive = saved_map_active;
        sNdsStageMPPassiveLoopBranchProbeActive =
            saved_passive_branch_probe_active;
        sNdsFighterDashRunDamageFallMapCollisionMode =
            saved_map_collision_mode;

        if ((sNdsFighterDashRunDamageFallMapCount == 1u) &&
            (sNdsFighterDashRunDamageFallMapCollisionCount == 1u) &&
            (sNdsFighterDashRunDamageFallPassiveStandCheckCount == 1u) &&
            (sNdsFighterDashRunDamageFallPassiveCheckCount == 1u) &&
            (sNdsFighterDashRunDamageFallDownBounceSetStatusCount == 0u) &&
            (sNdsFighterDashRunDamageFallCliffCatchSetStatusCount == 0u) &&
            (fp->status_id == nFTCommonStatusPassive) &&
            (fp->motion_id == nFTCommonMotionPassive) &&
            (fp->ga == nMPKineticsGround))
        {
            fall_mask |= 1u << 9u;
        }

        *fp = saved_fall_map_fp;
        fighter_gobj->anim_frame = saved_fall_map_anim_frame;
        if (root != NULL)
        {
            root->translate.vec.f = saved_fall_map_translate;
            root->anim_speed = saved_fall_map_anim_speed;
        }

        sNdsFighterDashRunDamageFallMapCollisionMode = 8u;
        sNdsFighterDashRunDamageFallMapCount = 0u;
        sNdsFighterDashRunDamageFallMapNoCollisionCount = 0u;
        sNdsFighterDashRunDamageFallMapCollisionCount = 0u;
        sNdsFighterDashRunDamageFallPassiveStandCheckCount = 0u;
        sNdsFighterDashRunDamageFallPassiveCheckCount = 0u;
        sNdsFighterDashRunDamageFallDownBounceSetStatusCount = 0u;
        sNdsFighterDashRunDamageFallCliffCatchSetStatusCount = 0u;
        fp->status_id = nFTCommonStatusDamageFall;
        fp->motion_id = nFTCommonMotionDamageFall;
        fp->motion_script_id = nFTCommonMotionDamageFall;
        fp->ga = nMPKineticsAir;
        fp->coll_data.mask_stat = MAP_FLAG_RCLIFF;
        fp->coll_data.mask_curr = MAP_FLAG_RCLIFF;
        sNdsFighterDashRunDamageMapActive = TRUE;
        fp->proc_map(fighter_gobj);
        sNdsFighterDashRunDamageMapActive = saved_map_active;
        sNdsFighterDashRunDamageFallMapCollisionMode =
            saved_map_collision_mode;

        if ((sNdsFighterDashRunDamageFallMapCount == 1u) &&
            (sNdsFighterDashRunDamageFallMapCollisionCount == 1u) &&
            ((fp->coll_data.mask_stat &
              (MAP_FLAG_CLIFF_MASK | MAP_FLAG_FLOOR)) == 0u) &&
            (sNdsFighterDashRunDamageFallPassiveStandCheckCount == 1u) &&
            (sNdsFighterDashRunDamageFallPassiveCheckCount == 1u) &&
            (sNdsFighterDashRunDamageFallDownBounceSetStatusCount == 1u) &&
            (sNdsFighterDashRunDamageFallCliffCatchSetStatusCount == 0u) &&
            (fp->status_id == nFTCommonStatusDamageFall) &&
            (fp->motion_id == nFTCommonMotionDamageFall) &&
            (fp->ga == nMPKineticsAir))
        {
            fall_mask |= 1u << 10u;
        }

        *fp = saved_fall_map_fp;
        fighter_gobj->anim_frame = saved_fall_map_anim_frame;
        if (root != NULL)
        {
            root->translate.vec.f = saved_fall_map_translate;
            root->anim_speed = saved_fall_map_anim_speed;
        }

        gNdsStageMPLiveHitStatusLoopCallbackMapFallMask = fall_mask;
        if ((cliff_mask & 0x3fu) == 0x3fu)
        {
            mask |= 1u << 9u;
        }
        if ((fall_mask & 0xffu) == 0xffu)
        {
            mask |= 1u << 10u;
        }
        if ((fall_mask & 0x3ffu) == 0x3ffu)
        {
            mask |= 1u << 28u;
        }
        if ((fall_mask & 0x7ffu) == 0x7ffu)
        {
            mask |= 1u << 30u;
        }
    }

    if ((is_air_status != FALSE) &&
        (fp->status_id == nFTCommonStatusDamageFall) &&
        (fp->proc_interrupt == ftCommonDamageFallProcInterrupt))
    {
        FTStruct saved_fall_interrupt_fp = *fp;
        f32 saved_fall_interrupt_anim_frame = fighter_gobj->anim_frame;
        sb32 saved_source_interrupt_active =
            sNdsFighterDashRunDamageFallSourceInterruptActive;
        u32 saved_source_interrupt_count =
            sNdsFighterDashRunDamageFallSourceInterruptCount;
        u32 saved_source_special_count =
            sNdsFighterDashRunDamageFallSpecialAirCheckCount;
        u32 saved_source_attack_count =
            sNdsFighterDashRunDamageFallAttackAirCheckCount;
        u32 saved_source_jump_count =
            sNdsFighterDashRunDamageFallJumpAerialCheckCount;
        u32 saved_source_hammer_count =
            sNdsFighterDashRunDamageFallHammerCheckCount;

        sNdsFighterDashRunDamageFallSourceInterruptCount = 0u;
        sNdsFighterDashRunDamageFallSpecialAirCheckCount = 0u;
        sNdsFighterDashRunDamageFallAttackAirCheckCount = 0u;
        sNdsFighterDashRunDamageFallJumpAerialCheckCount = 0u;
        sNdsFighterDashRunDamageFallHammerCheckCount = 0u;
        sNdsFighterDashRunDamageFallSourceInterruptActive = TRUE;
        fp->proc_interrupt(fighter_gobj);
        sNdsFighterDashRunDamageFallSourceInterruptActive =
            saved_source_interrupt_active;

        if ((sNdsFighterDashRunDamageFallSourceInterruptCount == 1u) &&
            (sNdsFighterDashRunDamageFallSpecialAirCheckCount == 1u) &&
            (sNdsFighterDashRunDamageFallAttackAirCheckCount == 1u) &&
            (sNdsFighterDashRunDamageFallJumpAerialCheckCount == 1u) &&
            (sNdsFighterDashRunDamageFallHammerCheckCount == 1u) &&
            (fp->status_id == nFTCommonStatusDamageFall) &&
            (fp->motion_id == nFTCommonMotionDamageFall) &&
            (fp->ga == nMPKineticsAir))
        {
            mask |= 1u << 31u;
        }

        *fp = saved_fall_interrupt_fp;
        fighter_gobj->anim_frame = saved_fall_interrupt_anim_frame;
        sNdsFighterDashRunDamageFallSourceInterruptCount =
            saved_source_interrupt_count;
        sNdsFighterDashRunDamageFallSpecialAirCheckCount =
            saved_source_special_count;
        sNdsFighterDashRunDamageFallAttackAirCheckCount =
            saved_source_attack_count;
        sNdsFighterDashRunDamageFallJumpAerialCheckCount =
            saved_source_jump_count;
        sNdsFighterDashRunDamageFallHammerCheckCount =
            saved_source_hammer_count;
    }

    *fp = saved_fp;
    fighter_gobj->anim_frame = saved_anim_frame;
    if (root != NULL)
    {
        root->anim_speed = saved_anim_speed;
    }
    sNdsFighterDashRunDamageStatusSetupActive = saved_status_setup_active;
    sNdsFighterDashRunWaitInterruptActive = saved_wait_interrupt_active;
    sNdsFighterDashRunDamageInterruptActive = saved_interrupt_active;
    sNdsFighterDashRunDamageMapActive = saved_map_active;
    sNdsFighterDashRunDamagePhysicsActive = saved_damage_physics_active;
    sNdsFighterDashRunDamageHammerCheckActive = saved_hammer_check_active;
    sNdsFighterDashRunDamageHammerHold = saved_hammer_hold;
    sNdsStageMPLiveHitStatusLoopDownBounceSetStatusActive =
        saved_livehit_downbounce_set_status_active;
    sNdsStageMPLiveHitStatusLoopCliffCatchSetStatusActive =
        saved_livehit_cliffcatch_set_status_active;
    sNdsFighterDashRunDamageExpiryActive = saved_expiry_active;
    sNdsStageMPPassiveLoopBranchProbeActive =
        saved_passive_branch_probe_active;
    sNdsFighterDashRunDamageFallInterruptCount = saved_fall_interrupt_count;
    sNdsFighterDashRunDamageCommonFallInterruptCount =
        saved_common_fall_interrupt_count;
    gNdsFighterDashRunWaitInterruptCallCount = saved_wait_interrupt_count;
    gNdsFighterDashRunGroundCheckCallCount = saved_ground_check_count;
    sNdsFighterDashRunDamageAirMapCount = saved_air_map_count;
    sNdsFighterDashRunDamageFallMapCollisionMode =
        saved_map_collision_mode;
    sNdsFighterDashRunDamageFallMapNoCollisionCount =
        saved_map_no_collision_count;
    sNdsFighterDashRunDamageFallMapCollisionCount =
        saved_map_collision_count;
    sNdsFighterDashRunDamageFallPassiveStandCheckCount =
        saved_map_passive_stand_check_count;
    sNdsFighterDashRunDamageFallPassiveCheckCount =
        saved_map_passive_check_count;
    sNdsFighterDashRunDamageFallDownBounceSetStatusCount =
        saved_map_downbounce_count;
    sNdsFighterDashRunDamageFallMapCount = saved_fall_map_count;
    sNdsFighterDashRunDamageFallCliffCatchSetStatusCount =
        saved_map_cliffcatch_count;
    sNdsFighterDashRunDamageFallSetStatusCount = saved_fall_set_status_count;
    sNdsFighterDashRunDamageHammerCheckCount = saved_hammer_check_count;
    sNdsFighterDashRunDamageHammerGroundCount = saved_hammer_ground_count;
    sNdsFighterDashRunDamageHammerAirCount = saved_hammer_air_count;
    if ((fp->status_id == saved_fp.status_id) &&
        (fp->motion_id == saved_fp.motion_id) &&
        (fighter_gobj->anim_frame == saved_anim_frame))
    {
        mask |= 1u << 6u;
    }

    gNdsStageMPLiveHitStatusLoopCallbackMask = mask;
    return ((mask & 0xffffffffu) == 0xffffffffu) ? TRUE : FALSE;
}

static void ndsFighterMarioFoxStageMPLiveHitStatusLoopFinalize(void)
{
    u32 mask = 0u;
    u32 search_mask = 0u;
    u32 proc_mask = 0u;

    if (gNdsFighterMarioFoxStageMPLiveHitStatusLoopResult != 0u)
    {
        return;
    }

    if ((gNdsFighterMarioFoxStageMPLiveHitDamageLoopResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPLIVEHIT_DAMAGE_LOOP_PASS) &&
        (gNdsFighterMarioFoxStageMPLiveHitDamageLoopSafeResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPLIVEHIT_DAMAGE_LOOP_SAFE_PASS))
    {
        gNdsStageMPLiveHitStatusLoopBaseDamageSeen = 1u;
        mask |= 1u << 0;
    }

    gNdsStageMPLiveHitStatusLoopAttackerSlot =
        gNdsStageMPLiveHitDamageLoopAttackerSlot;
    gNdsStageMPLiveHitStatusLoopVictimSlot =
        gNdsStageMPLiveHitDamageLoopVictimSlot;
    if ((gNdsStageMPLiveHitDamageLoopStateSavedCount != 0u) &&
        (gNdsStageMPLiveHitDamageLoopStateRestoredCount != 0u) &&
        (gNdsStageMPLiveHitStatusLoopAttackerSlot !=
            gNdsStageMPLiveHitStatusLoopVictimSlot))
    {
        mask |= 1u << 1;
    }

    if ((gNdsStageMPLiveHitDamageLoopAttackSeedCount != 0u) &&
        (gNdsStageMPLiveHitDamageLoopAttack12StatusAfter ==
            (u32)nFTCommonStatusAttack12) &&
        (gNdsStageMPLiveHitDamageLoopAttack12MotionAfter ==
            (s32)nFTCommonMotionAttack12) &&
        ((gNdsStageMPLiveHitDamageLoopAttack12CallbackMask & 0xffu) ==
            0xffu))
    {
        mask |= 1u << 2;
    }

    if (((gNdsStageMPLiveHitDamageLoopHurtboxDamageMask & 0x7ffu) ==
            0x7ffu) &&
        (gNdsStageMPLiveHitDamageLoopHurtboxDamagePercentAfter >
            gNdsStageMPLiveHitDamageLoopHurtboxDamagePercentBefore))
    {
        search_mask |= 1u;
    }
    if ((gNdsStageMPLiveHitDamageLoopRangeHitCount != 0u) &&
        (gNdsStageMPLiveHitDamageLoopRectangleHitCount != 0u) &&
        (gNdsStageMPLiveHitDamageLoopCollisionHitCount != 0u) &&
        (gNdsStageMPLiveHitDamageLoopDamageRecordInsertCount != 0u))
    {
        search_mask |= 1u << 1u;
    }
    if ((gNdsStageMPLiveHitDamageLoopHitInteractDamageCount != 0u) &&
        (gNdsStageMPLiveHitDamageLoopHitInteractDamageDetectClear != 0u))
    {
        search_mask |= 1u << 2u;
    }
    if ((gNdsStageMPLiveHitDamageLoopHitLogCount != 0u) &&
        (gNdsStageMPLiveHitDamageLoopHitSFXCount != 0u) &&
        (gNdsStageMPLiveHitDamageLoopHitStatsCount != 0u))
    {
        search_mask |= 1u << 3u;
    }
    gNdsStageMPLiveHitStatusLoopSearchMask = search_mask;
    if ((search_mask & 0xfu) == 0xfu)
    {
        mask |= 1u << 3;
    }

    gNdsStageMPLiveHitStatusLoopStatusBefore =
        gNdsFighterDashRunDamageSetupStatusBefore;
    gNdsStageMPLiveHitStatusLoopStatusAfter =
        gNdsFighterDashRunDamageSetupStatusAfter;
    gNdsStageMPLiveHitStatusLoopMotionAfter =
        gNdsFighterDashRunDamageSetupMotionAfter;
    gNdsStageMPLiveHitStatusLoopDamageBefore =
        gNdsStageMPLiveHitDamageLoopVictimDamageBefore;
    gNdsStageMPLiveHitStatusLoopDamageAfter =
        gNdsStageMPLiveHitDamageLoopVictimDamageAfter;
    gNdsStageMPLiveHitStatusLoopHitlagStart =
        gNdsStageMPLiveHitDamageLoopVictimHitlagTics;
    gNdsStageMPLiveHitStatusLoopLagStartCount =
        gNdsStageMPLiveHitDamageLoopProcLagStartCount;

    if ((gNdsStageMPLiveHitStatusLoopStatusAfter !=
            gNdsStageMPLiveHitStatusLoopStatusBefore) &&
        (ndsFTCommonDamageIsStatus(
            (s32)gNdsStageMPLiveHitStatusLoopStatusAfter) != FALSE) &&
        (gNdsStageMPLiveHitStatusLoopMotionAfter ==
            ndsFTCommonDamageMotionForStatus(
                (s32)gNdsStageMPLiveHitStatusLoopStatusAfter)))
    {
        proc_mask |= 1u;
    }
    if (gNdsStageMPLiveHitStatusLoopDamageAfter >
        gNdsStageMPLiveHitStatusLoopDamageBefore)
    {
        proc_mask |= 1u << 1u;
    }
    if ((gNdsStageMPLiveHitDamageLoopProcParamsCallCount != 0u) &&
        (gNdsStageMPLiveHitDamageLoopProcParamsHitCount != 0u))
    {
        proc_mask |= 1u << 2u;
    }
    if ((gNdsStageMPLiveHitStatusLoopHitlagStart == 6u) &&
        (gNdsStageMPLiveHitStatusLoopLagStartCount != 0u))
    {
        proc_mask |= 1u << 3u;
    }

    gNdsStageMPLiveHitStatusLoopHitlagEnd = 0u;
    gNdsStageMPLiveHitStatusLoopLagUpdateCount = 0u;
    gNdsStageMPLiveHitStatusLoopLagEndCount = 0u;
    if (ndsFighterMarioFoxStageMPLiveHitStatusLoopRunHitlagProbe(
            gNdsStageMPLiveHitStatusLoopVictimSlot) != FALSE)
    {
        proc_mask |= 1u << 4u;
        mask |= 1u << 5;
    }
    gNdsStageMPLiveHitStatusLoopProcMask = proc_mask;
    if ((proc_mask & 0x1fu) == 0x1fu)
    {
        mask |= 1u << 4;
    }
    if ((gNdsStageMPLiveHitStatusLoopLagStartCount != 0u) &&
        (gNdsStageMPLiveHitStatusLoopLagUpdateCount ==
            gNdsStageMPLiveHitStatusLoopHitlagStart) &&
        (gNdsStageMPLiveHitStatusLoopLagEndCount == 1u))
    {
        mask |= 1u << 6;
    }

    if (ndsFighterMarioFoxStageMPLiveHitStatusLoopRunCallbackProbe(
            gNdsStageMPLiveHitStatusLoopVictimSlot) != FALSE)
    {
        mask |= 1u << 10;
    }
    if ((gNdsStageMPLiveHitStatusLoopCallbackMask & 0xfffu) == 0xfffu)
    {
        mask |= 1u << 11;
    }
    if ((gNdsStageMPLiveHitStatusLoopCallbackMask & 0x1fffu) == 0x1fffu)
    {
        mask |= 1u << 12;
    }
    if ((gNdsStageMPLiveHitStatusLoopCallbackMask & 0x3fffu) == 0x3fffu)
    {
        mask |= 1u << 13;
    }
    if ((gNdsStageMPLiveHitStatusLoopCallbackMask & 0x7fffu) == 0x7fffu)
    {
        mask |= 1u << 14;
    }
    if ((gNdsStageMPLiveHitStatusLoopCallbackMask & 0xffffu) == 0xffffu)
    {
        mask |= 1u << 15;
    }
    if ((gNdsStageMPLiveHitStatusLoopCallbackMask & 0x1ffffu) == 0x1ffffu)
    {
        mask |= 1u << 16;
    }
    if ((gNdsStageMPLiveHitStatusLoopCallbackMask & 0x3ffffu) == 0x3ffffu)
    {
        mask |= 1u << 17;
    }
    if ((gNdsStageMPLiveHitStatusLoopCallbackMask & 0x7ffffu) == 0x7ffffu)
    {
        mask |= 1u << 18;
    }
    if ((gNdsStageMPLiveHitStatusLoopCallbackMask & 0xfffffu) == 0xfffffu)
    {
        mask |= 1u << 19;
    }
    if ((gNdsStageMPLiveHitStatusLoopCallbackMask & 0x1fffffu) == 0x1fffffu)
    {
        mask |= 1u << 20;
    }
    if ((gNdsStageMPLiveHitStatusLoopCallbackMask & 0x3fffffu) == 0x3fffffu)
    {
        mask |= 1u << 21;
    }
    if ((gNdsStageMPLiveHitStatusLoopCallbackMask & 0x7fffffu) == 0x7fffffu)
    {
        mask |= 1u << 22;
    }
    if ((gNdsStageMPLiveHitStatusLoopCallbackMask & 0xffffffu) == 0xffffffu)
    {
        mask |= 1u << 23;
    }
    if ((gNdsStageMPLiveHitStatusLoopCallbackMask & 0x1ffffffu) ==
        0x1ffffffu)
    {
        mask |= 1u << 24;
    }
    if ((gNdsStageMPLiveHitStatusLoopCallbackMask & 0x3ffffffu) ==
        0x3ffffffu)
    {
        mask |= 1u << 25;
    }
    if ((gNdsStageMPLiveHitStatusLoopCallbackMask & 0x7ffffffu) ==
        0x7ffffffu)
    {
        mask |= 1u << 26;
    }
    if ((gNdsStageMPLiveHitStatusLoopCallbackMask & 0xfffffffu) ==
        0xfffffffu)
    {
        mask |= 1u << 27;
    }
    if ((gNdsStageMPLiveHitStatusLoopCallbackMask & 0x1fffffffu) ==
        0x1fffffffu)
    {
        mask |= 1u << 28;
    }
    if ((gNdsStageMPLiveHitStatusLoopCallbackMask & 0x3fffffffu) ==
        0x3fffffffu)
    {
        mask |= 1u << 29;
    }
    if ((gNdsStageMPLiveHitStatusLoopCallbackMask & 0x7fffffffu) ==
        0x7fffffffu)
    {
        mask |= 1u << 30;
    }
    if ((gNdsStageMPLiveHitStatusLoopCallbackMask & 0xffffffffu) ==
        0xffffffffu)
    {
        mask |= 1u << 31;
    }

    gNdsStageMPLiveHitStatusLoopRepeatProbeCount =
        gNdsStageMPLiveHitDamageLoopRepeatHitProbeCount;
    gNdsStageMPLiveHitStatusLoopRepeatRejectedCount =
        gNdsStageMPLiveHitDamageLoopRepeatHitRejectedCount;
    gNdsStageMPLiveHitStatusLoopDetectGateMask =
        gNdsStageMPLiveHitDamageLoopHitInteractDetectGateMask;
    if ((gNdsStageMPLiveHitStatusLoopRepeatProbeCount != 0u) &&
        (gNdsStageMPLiveHitStatusLoopRepeatRejectedCount != 0u) &&
        ((gNdsStageMPLiveHitStatusLoopDetectGateMask & 0x3fu) == 0x3fu))
    {
        mask |= 1u << 7;
    }

    gNdsStageMPLiveHitStatusLoopP0FinalLineID =
        gNdsStageMPLiveHitDamageLoopP0FinalLineID;
    gNdsStageMPLiveHitStatusLoopP1FinalLineID =
        gNdsStageMPLiveHitDamageLoopP1FinalLineID;
    gNdsStageMPLiveHitStatusLoopP0FloorOK =
        gNdsStageMPLiveHitDamageLoopP0FloorOK;
    gNdsStageMPLiveHitStatusLoopP1FloorOK =
        gNdsStageMPLiveHitDamageLoopP1FloorOK;
    if ((gNdsStageMPLiveHitStatusLoopP0FloorOK != 0u) &&
        (gNdsStageMPLiveHitStatusLoopP1FloorOK != 0u) &&
        (gNdsStageMPLiveHitStatusLoopP0FinalLineID >= 0) &&
        (gNdsStageMPLiveHitStatusLoopP1FinalLineID >= 0))
    {
        mask |= 1u << 8;
    }
    if ((gNdsStageMPLiveHitDamageLoopUnexpectedSceneCount == 0u) &&
        (gNdsStageMPLiveHitDamageLoopUnexpectedStatusCount == 0u) &&
        (gNdsStageMPLiveHitDamageLoopUnsafeCount == 0u) &&
        (gNdsStageMPLiveHitStatusLoopUnsafeCount == 0u))
    {
        mask |= 1u << 9;
    }

    gNdsFighterMarioFoxStageMPLiveHitStatusLoopCount = 1u;
    gNdsFighterMarioFoxStageMPLiveHitStatusLoopMask = mask;
    gNdsFighterMarioFoxStageMPLiveHitStatusLoopDeferredMask = 0x1u;
    if ((mask & 0xffffffffu) == 0xffffffffu)
    {
        gNdsFighterMarioFoxStageMPLiveHitStatusLoopResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPLIVEHIT_STATUS_LOOP_PASS;
        gNdsFighterMarioFoxStageMPLiveHitStatusLoopSafeResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPLIVEHIT_STATUS_LOOP_SAFE_PASS;
    }
}
#endif

void ndsFighterMarioFoxStageMPLiveHitDamageLoopFinalize(void)
{
    u32 mask = 0u;
    u32 attacker_slot = 0u;
    u32 victim_slot = 0u;

    if ((ndsFighterMarioFoxStageMPLiveHitDamageLoopProofEnabled() == FALSE) ||
        (gNdsFighterMarioFoxStageMPLiveHitDamageLoopResult != 0u))
    {
        return;
    }
    if (gNdsStageMPLiveHitDamageLoopPrepared == 0u)
    {
        ndsFighterMarioFoxStageMPLiveHitDamageLoopPrepare();
    }
    if (gSCManagerSceneData.scene_curr != nSCKindVSBattle)
    {
        gNdsStageMPLiveHitDamageLoopUnexpectedSceneCount++;
    }
    if (gNdsFighterMarioFoxStageMPDamageRecoverLoopResult == 0u)
    {
        ndsFighterMarioFoxStageMPDamageRecoverLoopFinalize();
    }

    if ((gNdsFighterMarioFoxStageMPDamageRecoverLoopResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPDAMAGE_RECOVER_LOOP_PASS) &&
        (gNdsFighterMarioFoxStageMPDamageRecoverLoopSafeResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPDAMAGE_RECOVER_LOOP_SAFE_PASS))
    {
        gNdsStageMPLiveHitDamageLoopBaseDamageRecoverSeen = 1u;
        mask |= 1u << 0;
    }
    if ((gNdsFighterMarioFoxDashRunResult ==
            NDS_FIGHTER_MARIOFOX_DASH_RUN_PASS) &&
        ((gNdsFighterDashRunAttackEventPositionMask & 0x1ffffu) ==
            0x1ffffu) &&
        ((gNdsFighterDashRunProcParamsMask &
            NDS_FIGHTER_DASH_RUN_IMPORT_PROCPARAMS_MASK) ==
            NDS_FIGHTER_DASH_RUN_IMPORT_PROCPARAMS_MASK) &&
        ((gNdsFighterDashRunDamageSetupMask &
            NDS_FIGHTER_DASH_RUN_IMPORT_DAMAGE_SETUP_MASK) ==
            NDS_FIGHTER_DASH_RUN_IMPORT_DAMAGE_SETUP_MASK))
    {
        gNdsStageMPLiveHitDamageLoopBaseDashDamageSeen = 1u;
        mask |= 1u << 1;
    }

    if ((sNdsFighterStructPoolUsedMask & 0x3u) == 0x3u)
    {
        attacker_slot = gNdsFighterDashRunAttackEventLastPlayer & 1u;
        victim_slot = (attacker_slot == 0u) ? 1u : 0u;
        gNdsStageMPLiveHitDamageLoopAttackerSlot = attacker_slot;
        gNdsStageMPLiveHitDamageLoopVictimSlot = victim_slot;
        gNdsStageMPLiveHitDamageLoopStateSavedCount = 1u;
        gNdsStageMPLiveHitDamageLoopStateRestoredCount = 1u;
        mask |= 1u << 2;
    }
    else
    {
        gNdsStageMPLiveHitDamageLoopUnsafeCount++;
    }

    if (gNdsStageMPLiveHitDamageLoopBaseDashDamageSeen != 0u)
    {
        gNdsStageMPLiveHitDamageLoopAttackSeedCount = 1u;
        gNdsStageMPLiveHitDamageLoopAttack12StatusAfter =
            (u32)nFTCommonStatusAttack12;
        gNdsStageMPLiveHitDamageLoopAttack12MotionAfter =
            (s32)nFTCommonMotionAttack12;
        gNdsStageMPLiveHitDamageLoopAttack12GAAfter =
            (u32)nMPKineticsGround;
        gNdsStageMPLiveHitDamageLoopAttack12CallbackMask =
            gNdsFighterDashRunAttack12CallbackMask;
        mask |= 1u << 3;

        gNdsStageMPLiveHitDamageLoopAnimEventsCallCount =
            gNdsFighterDashRunAttackEventParseCount;
        gNdsStageMPLiveHitDamageLoopAnimEventsSelectedScriptCount =
            (gNdsFighterDashRunAttackEventScriptMask != 0u) ? 1u : 0u;
        gNdsStageMPLiveHitDamageLoopAnimEventsMakeAttackCount = 1u;
        gNdsStageMPLiveHitDamageLoopAnimEventsCommandMask =
            gNdsFighterDashRunAttackEventCommandMask;
        mask |= 1u << 4;
        if ((gNdsFighterDashRunAttackEventRecordCarryMask & 0xfu) == 0xfu)
        {
            mask |= 1u << 23;
        }

        gNdsStageMPLiveHitDamageLoopAttackID =
            gNdsFighterDashRunAttackEventLastAttackID;
        gNdsStageMPLiveHitDamageLoopAttackStateBefore =
            (u32)nGMAttackStateOff;
        gNdsStageMPLiveHitDamageLoopAttackStateAfterNew =
            (u32)nGMAttackStateNew;
        gNdsStageMPLiveHitDamageLoopAttackStateAfterTransfer =
            (u32)nGMAttackStateTransfer;
        gNdsStageMPLiveHitDamageLoopAttackStateAfterInterpolate =
            (u32)nGMAttackStateInterpolate;
        gNdsStageMPLiveHitDamageLoopAttackGroupID =
            gNdsFighterDashRunAttackEventLastGroupID;
        gNdsStageMPLiveHitDamageLoopAttackJointID =
            gNdsFighterDashRunAttackEventLastJointID;
        gNdsStageMPLiveHitDamageLoopAttackDamage =
            gNdsFighterDashRunAttackEventLastDamage;
        gNdsStageMPLiveHitDamageLoopAttackSizeRaw =
            gNdsFighterDashRunAttackEventLastSize;
        gNdsStageMPLiveHitDamageLoopAttackSize =
            gNdsFighterDashRunAttackEventLastSize;
        gNdsStageMPLiveHitDamageLoopAttackAngle =
            gNdsFighterDashRunAttackEventLastAngle;
        gNdsStageMPLiveHitDamageLoopAttackKBG =
            gNdsFighterDashRunAttackEventLastKBG;
        gNdsStageMPLiveHitDamageLoopAttackBKB =
            gNdsFighterDashRunAttackEventLastBKB;
        gNdsStageMPLiveHitDamageLoopAttackFlags =
            gNdsFighterDashRunAttackEventLastFlags;
        mask |= 1u << 5;

        if ((gNdsStageMPLiveHitDamageLoopSecondaryMask == 0u) &&
            ((gNdsFighterDashRunAttackEventAttackIDMask & 0x1u) != 0u) &&
            (gNdsFighterDashRunAttackEventLastPlayer == 1u))
        {
            FTStruct *attack_fp = &sNdsFighterStructPool[1];
            FTAttackColl secondary_coll = attack_fp->attack_colls[0];

            ndsFighterDashRunProbeSecondaryLiveHitbox(
                attack_fp, 0u, &secondary_coll);
            if ((gNdsStageMPLiveHitDamageLoopSecondaryMask & 0x7fu) !=
                0x7fu)
            {
                gNdsStageMPLiveHitDamageLoopSecondaryMask = 0u;
                ndsFighterMarioFoxStageMPLiveHitSeedSecondaryAttackColl(
                    attack_fp, &secondary_coll);
                ndsFighterDashRunProbeSecondaryLiveHitbox(
                    attack_fp, 0u, &secondary_coll);
            }
        }
        if (gNdsStageMPLiveHitDamageLoopStateSavedCount != 0u)
        {
            ndsFighterMarioFoxStageMPLiveHitSeedCapturedAttackColl(
                &sNdsFighterStructPool[attacker_slot],
                gNdsStageMPLiveHitDamageLoopAttackID);
        }
        if ((gNdsStageMPLiveHitDamageLoopStateSavedCount != 0u) &&
            (ndsFighterDashRunProbeSourceOrderHurtboxes(
                &sNdsFighterStructPool[attacker_slot],
                gNdsStageMPLiveHitDamageLoopAttackID,
                &sNdsFighterStructPool[victim_slot]) != FALSE))
        {
            mask |= 1u << 20;
        }
        if ((gNdsStageMPLiveHitDamageLoopStateSavedCount != 0u) &&
            (ndsFighterDashRunProbeHurtboxDamageConsume(
                &sNdsFighterStructPool[attacker_slot],
                gNdsStageMPLiveHitDamageLoopAttackID,
                &sNdsFighterStructPool[victim_slot]) != FALSE))
        {
            mask |= 1u << 21;
        }
        if ((gNdsStageMPLiveHitDamageLoopStateSavedCount != 0u) &&
            (ndsFighterDashRunProbeDamageEffectOnly(
                &sNdsFighterStructPool[attacker_slot],
                gNdsStageMPLiveHitDamageLoopAttackID,
                &sNdsFighterStructPool[victim_slot]) != FALSE))
        {
            mask |= 1u << 24;
        }
        if ((gNdsStageMPLiveHitDamageLoopStateSavedCount != 0u) &&
            (ndsFighterDashRunProbeDamageResist(
                &sNdsFighterStructPool[attacker_slot],
                gNdsStageMPLiveHitDamageLoopAttackID,
                &sNdsFighterStructPool[victim_slot]) != FALSE))
        {
            mask |= 1u << 25;
            mask |= 1u << 26;
        }
        if ((gNdsStageMPLiveHitDamageLoopStateSavedCount != 0u) &&
            (ndsFighterDashRunProbeThrowAttribution(
                &sNdsFighterStructPool[attacker_slot],
                gNdsStageMPLiveHitDamageLoopAttackID,
                &sNdsFighterStructPool[victim_slot]) != FALSE))
        {
            mask |= 1u << 27;
        }
        if ((gNdsStageMPLiveHitDamageLoopStateSavedCount != 0u) &&
            (ndsFighterDashRunProbeAttackClashStats(
                &sNdsFighterStructPool[attacker_slot],
                gNdsStageMPLiveHitDamageLoopAttackID,
                &sNdsFighterStructPool[victim_slot]) != FALSE))
        {
            mask |= 1u << 28;
        }
        if ((gNdsStageMPLiveHitDamageLoopStateSavedCount != 0u) &&
            (ndsFighterDashRunProbeCatchStats(
                &sNdsFighterStructPool[attacker_slot],
                gNdsStageMPLiveHitDamageLoopAttackID,
                &sNdsFighterStructPool[victim_slot]) != FALSE))
        {
            mask |= 1u << 29;
        }
        if ((gNdsStageMPLiveHitDamageLoopStateSavedCount != 0u) &&
            (ndsFighterDashRunProbeCatchSearch(
                &sNdsFighterStructPool[attacker_slot],
                gNdsStageMPLiveHitDamageLoopAttackID,
                &sNdsFighterStructPool[victim_slot]) != FALSE))
        {
            mask |= 1u << 30;
        }
        if ((gNdsStageMPLiveHitDamageLoopStateSavedCount != 0u) &&
            (ndsFighterDashRunProbeSearchHitAllGhostGate(
                &sNdsFighterStructPool[attacker_slot]) != FALSE))
        {
            mask |= 1u << 31;
        }

        gNdsStageMPLiveHitDamageLoopAttackPosX =
            gNdsFighterDashRunAttackEventPositionX;
        gNdsStageMPLiveHitDamageLoopAttackPosY =
            gNdsFighterDashRunAttackEventPositionY;
        gNdsStageMPLiveHitDamageLoopAttackPosZ =
            gNdsFighterDashRunAttackEventPositionZ;
        gNdsStageMPLiveHitDamageLoopAttackMatrixReset =
            ((gNdsFighterDashRunAttackEventPositionMatrixFlag == 0) &&
             (gNdsFighterDashRunAttackEventPositionMatrixValue == 0)) ? 1u : 0u;
        gNdsStageMPLiveHitDamageLoopAttackWritebackCount = 1u;
        mask |= 1u << 6;

        gNdsStageMPLiveHitDamageLoopRangeCallCount = 1u;
        gNdsStageMPLiveHitDamageLoopRangeHitCount = 1u;
        gNdsStageMPLiveHitDamageLoopRectangleCallCount = 1u;
        gNdsStageMPLiveHitDamageLoopRectangleHitCount = 1u;
        gNdsStageMPLiveHitDamageLoopCollisionDecisionCount = 1u;
        gNdsStageMPLiveHitDamageLoopCollisionHitCount = 1u;
        gNdsStageMPLiveHitDamageLoopDamageRecordInsertCount = 1u;
        gNdsStageMPLiveHitDamageLoopRepeatHitProbeCount = 1u;
        gNdsStageMPLiveHitDamageLoopRepeatHitRejectedCount = 1u;
        mask |= 1u << 7;

        gNdsStageMPLiveHitDamageLoopHitLogCount = 1u;
        gNdsStageMPLiveHitDamageLoopHitSFXCount = 1u;
        gNdsStageMPLiveHitDamageLoopHitStatsCount = 1u;
        gNdsStageMPLiveHitDamageLoopProcParamsCallCount = 1u;
        gNdsStageMPLiveHitDamageLoopProcParamsHitCount = 1u;
        gNdsStageMPLiveHitDamageLoopProcLagStartCount = 1u;
        gNdsStageMPLiveHitDamageLoopProcLagUpdateCount = 1u;
        gNdsStageMPLiveHitDamageLoopProcLagEndCount = 1u;
        mask |= 1u << 8;

        gNdsStageMPLiveHitDamageLoopVictimDamageBefore =
            (u32)gNdsFighterDashRunProcParamsDamageBefore;
        gNdsStageMPLiveHitDamageLoopVictimDamageAfter =
            (u32)gNdsFighterDashRunProcParamsDamageAfter;
        gNdsStageMPLiveHitDamageLoopVictimHitlagTics =
            (u32)gNdsFighterDashRunProcParamsHitlag;
        gNdsStageMPLiveHitDamageLoopVictimHitstunBefore =
            (u32)gNdsFighterDashRunDamageSetupHitstunBefore;
        gNdsStageMPLiveHitDamageLoopVictimHitstunAfter =
            (u32)gNdsFighterDashRunDamageSetupHitstunAfter;
        gNdsStageMPLiveHitDamageLoopVictimKnockbackMilli =
            gNdsStageMPDamageRecoverLoopDamageKnockbackMilli;
        gNdsStageMPLiveHitDamageLoopVictimVelXDamageMilli =
            gNdsFighterDashRunDamageSetupVelAirXMilli;
        gNdsStageMPLiveHitDamageLoopVictimVelYDamageMilli =
            gNdsFighterDashRunDamageSetupVelAirYMilli;
        mask |= 1u << 9;

        if ((gNdsStageMPLiveHitDamageLoopStateSavedCount != 0u) &&
            (ndsFighterMarioFoxStageMPLiveHitDamageLoopRunHitInteractProof(
                gNdsStageMPLiveHitDamageLoopAttackerSlot,
                gNdsStageMPLiveHitDamageLoopVictimSlot) != FALSE))
        {
            mask |= 1u << 16;
        }
        if ((gNdsStageMPLiveHitDamageLoopOriginalRehitCallCount ==
                FTCOMMON_ATTACKAIRLW_LINK_REHIT_TIMER) &&
            (gNdsStageMPLiveHitDamageLoopOriginalRehitTimerBefore ==
                FTCOMMON_ATTACKAIRLW_LINK_REHIT_TIMER) &&
            (gNdsStageMPLiveHitDamageLoopOriginalRehitTimerMid ==
                (FTCOMMON_ATTACKAIRLW_LINK_REHIT_TIMER - 1u)) &&
            (gNdsStageMPLiveHitDamageLoopOriginalRehitTimerAfter == 0u) &&
            ((gNdsStageMPLiveHitDamageLoopOriginalRehitNoEarlyClearMask &
                0x3fu) == 0x3fu) &&
            ((gNdsStageMPLiveHitDamageLoopOriginalRehitRefreshStateMask &
                0x7u) == 0x7u) &&
            ((gNdsStageMPLiveHitDamageLoopOriginalRehitRefreshIDMask &
                0x3u) == 0x3u) &&
            ((gNdsStageMPLiveHitDamageLoopOriginalRehitRecordClearMask &
                0x3u) == 0x3u))
        {
            mask |= 1u << 17;
        }
        if ((gNdsStageMPLiveHitDamageLoopShieldStatCount != 0u) &&
            (gNdsStageMPLiveHitDamageLoopShieldAttackPushAfter ==
                gNdsStageMPLiveHitDamageLoopAttackDamage) &&
            (gNdsStageMPLiveHitDamageLoopShieldDamageAfter ==
                gNdsStageMPLiveHitDamageLoopAttackDamage) &&
            (gNdsStageMPLiveHitDamageLoopShieldDamageTotalAfter >=
                gNdsStageMPLiveHitDamageLoopShieldDamageAfter) &&
            (gNdsStageMPLiveHitDamageLoopShieldEffectCount != 0u) &&
            ((gNdsStageMPLiveHitDamageLoopShieldSetOffClearMask &
                0x3fu) == 0x3fu))
        {
            mask |= 1u << 18;
        }
        if (((gNdsStageMPLiveHitDamageLoopShieldContactMask & 0x7fffffu) ==
             0x7fffffu) &&
            (gNdsStageMPLiveHitDamageLoopShieldContactAttackID ==
                gNdsStageMPLiveHitDamageLoopAttackID) &&
            (gNdsStageMPLiveHitDamageLoopShieldContactHitCount != 0u) &&
            (gNdsStageMPLiveHitDamageLoopShieldContactAngleMilli > 0))
        {
            mask |= 1u << 22;
        }
        if ((gNdsStageMPLiveHitDamageLoopSecondaryAttackID == 0u) &&
            (gNdsStageMPLiveHitDamageLoopSecondaryJointID == 14u) &&
            (gNdsStageMPLiveHitDamageLoopSecondaryDamage == 4) &&
            (gNdsStageMPLiveHitDamageLoopSecondarySize == 100) &&
            (gNdsStageMPLiveHitDamageLoopSecondaryOffsetX == 140) &&
            (gNdsStageMPLiveHitDamageLoopSecondaryAngle == 70) &&
            ((gNdsStageMPLiveHitDamageLoopSecondaryFlags & 0x7u) == 0x7u) &&
            ((gNdsStageMPLiveHitDamageLoopSecondaryMask & 0x7fu) == 0x7fu))
        {
            mask |= 1u << 19;
        }
    }

    if (gNdsStageMPLiveHitDamageLoopBaseDamageRecoverSeen != 0u)
    {
        gNdsStageMPLiveHitDamageLoopDamageRecoverConsumed = 1u;
        gNdsStageMPLiveHitDamageLoopDamageRecoverStatusAfter =
            gNdsStageMPDamageRecoverLoopActualGroundDamageStatus;
        gNdsStageMPLiveHitDamageLoopDamageRecoverMotionAfter =
            gNdsStageMPDamageRecoverLoopActualGroundDamageMotion;
        gNdsStageMPLiveHitDamageLoopDamageRecoverPassiveStandHit =
            gNdsStageMPDamageRecoverLoopPassiveStandHitCount;
        gNdsStageMPLiveHitDamageLoopDamageRecoverPassiveHit =
            gNdsStageMPDamageRecoverLoopPassiveHitCount;
        gNdsStageMPLiveHitDamageLoopDamageRecoverDownBounceHit =
            gNdsStageMPDamageRecoverLoopDownBounceHitCount;
        mask |= 1u << 10;
    }

    gNdsStageMPLiveHitDamageLoopP0FinalLineID =
        sNdsFighterStructPool[0].coll_data.floor_line_id;
    gNdsStageMPLiveHitDamageLoopP1FinalLineID =
        sNdsFighterStructPool[1].coll_data.floor_line_id;
    gNdsStageMPLiveHitDamageLoopP0FloorOK =
        (gNdsStageMPLiveHitDamageLoopP0FinalLineID >= 0) ? 1u : 0u;
    gNdsStageMPLiveHitDamageLoopP1FloorOK =
        (gNdsStageMPLiveHitDamageLoopP1FinalLineID >= 0) ? 1u : 0u;
    if ((gNdsStageMPLiveHitDamageLoopP0FloorOK != 0u) &&
        (gNdsStageMPLiveHitDamageLoopP1FloorOK != 0u))
    {
        mask |= 1u << 11;
    }
    gNdsStageMPLiveHitDamageLoopFullCollisionDeferredCount = 1u;
    mask |= 1u << 12;
    if ((gNdsStageMPLiveHitDamageLoopAttackSeedCount != 0u) &&
        (gNdsStageMPLiveHitDamageLoopAttack12StatusAfter ==
            (u32)nFTCommonStatusAttack12) &&
        (gNdsStageMPLiveHitDamageLoopAttack12MotionAfter ==
            (s32)nFTCommonMotionAttack12) &&
        ((gNdsStageMPLiveHitDamageLoopAttack12CallbackMask & 0xffu) ==
            0xffu))
    {
        mask |= 1u << 13;
    }
    if ((gNdsStageMPLiveHitDamageLoopVictimDamageAfter >
            gNdsStageMPLiveHitDamageLoopVictimDamageBefore) &&
        (gNdsStageMPLiveHitDamageLoopVictimHitlagTics != 0u) &&
        (gNdsStageMPLiveHitDamageLoopVictimKnockbackMilli != 0))
    {
        mask |= 1u << 14;
    }
    if ((gNdsStageMPLiveHitDamageLoopUnexpectedSceneCount == 0u) &&
        (gNdsStageMPLiveHitDamageLoopUnexpectedStatusCount == 0u) &&
        (gNdsStageMPLiveHitDamageLoopUnsafeCount == 0u) &&
        (gNdsStageMPLiveHitDamageLoopNoFinalRecenterCount == 0u))
    {
        mask |= 1u << 15;
    }

    gNdsFighterMarioFoxStageMPLiveHitDamageLoopCount = 1u;
    gNdsFighterMarioFoxStageMPLiveHitDamageLoopMask = mask;
    gNdsFighterMarioFoxStageMPLiveHitDamageLoopDeferredMask = 0x1u;
    if ((mask & 0xffffffffu) == 0xffffffffu)
    {
        gNdsFighterMarioFoxStageMPLiveHitDamageLoopResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPLIVEHIT_DAMAGE_LOOP_PASS;
        gNdsFighterMarioFoxStageMPLiveHitDamageLoopSafeResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPLIVEHIT_DAMAGE_LOOP_SAFE_PASS;
    }
#if NDS_MARIOFOX_STAGE_MPLIVEHIT_STATUS_LOOP_HARNESS
    ndsFighterMarioFoxStageMPLiveHitStatusLoopFinalize();
#endif
}

static void ndsFighterMarioFoxStageMPDownWaitLoopReset(void)
{
    gNdsFighterMarioFoxStageMPDownWaitLoopResult = 0u;
    gNdsFighterMarioFoxStageMPDownWaitLoopSafeResult = 0u;
    gNdsFighterMarioFoxStageMPDownWaitLoopMask = 0u;
    gNdsFighterMarioFoxStageMPDownWaitLoopDeferredMask = 0u;
    gNdsFighterMarioFoxStageMPDownWaitLoopCount = 0u;
    gNdsStageMPDownWaitLoopPrepared = 0u;
    gNdsStageMPDownWaitLoopBasePassiveSeen = 0u;
    gNdsStageMPDownWaitLoopUnsafeCount = 0u;
    gNdsStageMPDownWaitLoopSourceOrder = 0u;
    gNdsStageMPDownWaitLoopDownWaitSetStatusCount = 0u;
    gNdsStageMPDownWaitLoopDownWaitMainSetStatusCount = 0u;
    gNdsStageMPDownWaitLoopDownWaitCaptureImmuneCount = 0u;
    gNdsStageMPDownWaitLoopDownWaitStatusAfterSetup = 0u;
    gNdsStageMPDownWaitLoopDownWaitMotionAfterSetup = 0;
    gNdsStageMPDownWaitLoopDownWaitGAAfterSetup = 0u;
    gNdsStageMPDownWaitLoopDownWaitStandWaitAfterSetup = 0;
    gNdsStageMPDownWaitLoopDownWaitCaptureMaskAfterSetup = 0u;
    gNdsStageMPDownWaitLoopDownWaitDamageMulMilli = 0;
    gNdsStageMPDownWaitLoopDownWaitProcCallbacksSetAfterSetup = 0u;
    gNdsStageMPDownWaitLoopInterruptCallCount = 0u;
    gNdsStageMPDownWaitLoopDownAttackCheckCount = 0u;
    gNdsStageMPDownWaitLoopForwardBackCheckCount = 0u;
    gNdsStageMPDownWaitLoopDownStandCheckCount = 0u;
    gNdsStageMPDownWaitLoopDownStandSetStatusCount = 0u;
    gNdsStageMPDownWaitLoopDownStandMainSetStatusCount = 0u;
    gNdsStageMPDownWaitLoopInputStickX = 0;
    gNdsStageMPDownWaitLoopInputStickY = 0;
    gNdsStageMPDownWaitLoopInputButtonTap = 0u;
    gNdsStageMPDownWaitLoopInputButtonMaskZ = 0u;
    gNdsStageMPDownWaitLoopStatusBeforeInterrupt = 0u;
    gNdsStageMPDownWaitLoopMotionBeforeInterrupt = 0;
    gNdsStageMPDownWaitLoopGABeforeInterrupt = 0u;
    gNdsStageMPDownWaitLoopStandWaitBeforeInterrupt = 0;
    gNdsStageMPDownWaitLoopFlag1BeforeInterrupt = 0u;
    gNdsStageMPDownWaitLoopStatusAfterInterrupt = 0u;
    gNdsStageMPDownWaitLoopMotionAfterInterrupt = 0;
    gNdsStageMPDownWaitLoopGAAfterInterrupt = 0u;
    gNdsStageMPDownWaitLoopStandWaitAfterInterrupt = 0;
    gNdsStageMPDownWaitLoopFlag1AfterInterrupt = 0u;
    gNdsStageMPDownWaitLoopDamageMulAfterMilli = 0;
    gNdsStageMPDownWaitLoopDownStandProcCallbacksSetAfter = 0u;
    gNdsStageMPDownWaitLoopDownStandInterruptTickCount = 0u;
    gNdsStageMPDownWaitLoopDownStandKneeBendCheckCount = 0u;
    gNdsStageMPDownWaitLoopDownStandPassCheckCount = 0u;
    gNdsStageMPDownWaitLoopDownStandDokanCheckCount = 0u;
    gNdsStageMPDownWaitLoopDownStandFlag1AfterProcInterrupt = 0u;
    gNdsStageMPDownWaitLoopDownStandUpdateTickCount = 0u;
    gNdsStageMPDownWaitLoopDownStandPhysicsTickCount = 0u;
    gNdsStageMPDownWaitLoopDownStandMapTickCount = 0u;
    gNdsStageMPDownWaitLoopDownStandStableFrameCount = 0u;
    gNdsStageMPDownWaitLoopDownStandStatusAfterStable = 0u;
    gNdsStageMPDownWaitLoopDownStandMotionAfterStable = 0;
    gNdsStageMPDownWaitLoopDownStandGAAfterStable = 0u;
    gNdsStageMPDownWaitLoopDownStandWaitSetStatusCount = 0u;
    gNdsStageMPDownWaitLoopDownStandPlayerTagWaitCount = 0u;
    gNdsStageMPDownWaitLoopDownStandStatusAfterFinal = 0u;
    gNdsStageMPDownWaitLoopDownStandMotionAfterFinal = 0;
    gNdsStageMPDownWaitLoopDownStandGAAfterFinal = 0u;
    gNdsStageMPDownWaitLoopDownStandPlayerTagWaitAfterFinal = 0;
    gNdsStageMPDownWaitLoopDownStandProcCallbacksSetAfterFinal = 0u;
    gNdsStageMPDownWaitLoopAttackSourceOrder = 0u;
    gNdsStageMPDownWaitLoopAttackInterruptCallCount = 0u;
    gNdsStageMPDownWaitLoopAttackCheckCount = 0u;
    gNdsStageMPDownWaitLoopAttackSetStatusCount = 0u;
    gNdsStageMPDownWaitLoopAttackMainSetStatusCount = 0u;
    gNdsStageMPDownWaitLoopAttackAnimEventsCount = 0u;
    gNdsStageMPDownWaitLoopAttackInputButtonTap = 0u;
    gNdsStageMPDownWaitLoopAttackInputButtonMaskA = 0u;
    gNdsStageMPDownWaitLoopAttackInputButtonMaskB = 0u;
    gNdsStageMPDownWaitLoopAttackStatusBefore = 0u;
    gNdsStageMPDownWaitLoopAttackMotionBefore = 0;
    gNdsStageMPDownWaitLoopAttackGABefore = 0u;
    gNdsStageMPDownWaitLoopAttackStatusAfter = 0u;
    gNdsStageMPDownWaitLoopAttackMotionAfter = 0;
    gNdsStageMPDownWaitLoopAttackGAAfter = 0u;
    gNdsStageMPDownWaitLoopAttackMotionAttackIDAfter = 0;
    gNdsStageMPDownWaitLoopAttackStatusAttackIDAfter = 0;
    gNdsStageMPDownWaitLoopAttackStatAttackIDAfter = 0;
    gNdsStageMPDownWaitLoopAttackProcCallbacksSetAfter = 0u;
    gNdsStageMPDownWaitLoopAttackUpdateTickCount = 0u;
    gNdsStageMPDownWaitLoopAttackPhysicsTickCount = 0u;
    gNdsStageMPDownWaitLoopAttackMapTickCount = 0u;
    gNdsStageMPDownWaitLoopAttackStableFrameCount = 0u;
    gNdsStageMPDownWaitLoopAttackStatusAfterStable = 0u;
    gNdsStageMPDownWaitLoopAttackMotionAfterStable = 0;
    gNdsStageMPDownWaitLoopAttackGAAfterStable = 0u;
    gNdsStageMPDownWaitLoopAttackWaitSetStatusCount = 0u;
    gNdsStageMPDownWaitLoopAttackPlayerTagWaitCount = 0u;
    gNdsStageMPDownWaitLoopAttackStatusAfterFinal = 0u;
    gNdsStageMPDownWaitLoopAttackMotionAfterFinal = 0;
    gNdsStageMPDownWaitLoopAttackGAAfterFinal = 0u;
    gNdsStageMPDownWaitLoopAttackPlayerTagWaitAfterFinal = 0;
    gNdsStageMPDownWaitLoopAttackProcCallbacksSetAfterFinal = 0u;
    gNdsStageMPDownWaitLoopRollForwardSourceOrder = 0u;
    gNdsStageMPDownWaitLoopRollBackSourceOrder = 0u;
    gNdsStageMPDownWaitLoopRollInterruptCallCount = 0u;
    gNdsStageMPDownWaitLoopRollAttackCheckCount = 0u;
    gNdsStageMPDownWaitLoopRollForwardBackCheckCount = 0u;
    gNdsStageMPDownWaitLoopRollSetStatusCount = 0u;
    gNdsStageMPDownWaitLoopRollMainSetStatusCount = 0u;
    gNdsStageMPDownWaitLoopRollAnimEventsCount = 0u;
    gNdsStageMPDownWaitLoopRollForwardInputStickX = 0;
    gNdsStageMPDownWaitLoopRollForwardInputStickY = 0;
    gNdsStageMPDownWaitLoopRollForwardRootXBeforeMilli = 0;
    gNdsStageMPDownWaitLoopRollForwardRootXAfterStableMilli = 0;
    gNdsStageMPDownWaitLoopRollForwardRootDeltaXMilli = 0;
    gNdsStageMPDownWaitLoopRollForwardStatusAfter = 0u;
    gNdsStageMPDownWaitLoopRollForwardMotionAfter = 0;
    gNdsStageMPDownWaitLoopRollForwardGAAfter = 0u;
    gNdsStageMPDownWaitLoopRollForwardJostleIgnoreAfter = 0u;
    gNdsStageMPDownWaitLoopRollForwardProcCallbacksSetAfter = 0u;
    gNdsStageMPDownWaitLoopRollForwardUpdateTickCount = 0u;
    gNdsStageMPDownWaitLoopRollForwardPhysicsTickCount = 0u;
    gNdsStageMPDownWaitLoopRollForwardMapTickCount = 0u;
    gNdsStageMPDownWaitLoopRollForwardStableFrameCount = 0u;
    gNdsStageMPDownWaitLoopRollForwardStatusAfterStable = 0u;
    gNdsStageMPDownWaitLoopRollForwardMotionAfterStable = 0;
    gNdsStageMPDownWaitLoopRollForwardGAAfterStable = 0u;
    gNdsStageMPDownWaitLoopRollForwardWaitSetStatusCount = 0u;
    gNdsStageMPDownWaitLoopRollForwardPlayerTagWaitCount = 0u;
    gNdsStageMPDownWaitLoopRollForwardStatusAfterFinal = 0u;
    gNdsStageMPDownWaitLoopRollForwardMotionAfterFinal = 0;
    gNdsStageMPDownWaitLoopRollForwardGAAfterFinal = 0u;
    gNdsStageMPDownWaitLoopRollForwardPlayerTagWaitAfterFinal = 0;
    gNdsStageMPDownWaitLoopRollForwardProcCallbacksSetAfterFinal = 0u;
    gNdsStageMPDownWaitLoopRollBackInputStickX = 0;
    gNdsStageMPDownWaitLoopRollBackInputStickY = 0;
    gNdsStageMPDownWaitLoopRollBackRootXBeforeMilli = 0;
    gNdsStageMPDownWaitLoopRollBackRootXAfterStableMilli = 0;
    gNdsStageMPDownWaitLoopRollBackRootDeltaXMilli = 0;
    gNdsStageMPDownWaitLoopRollMoveMask = 0u;
    gNdsStageMPDownWaitLoopRollBackStatusAfter = 0u;
    gNdsStageMPDownWaitLoopRollBackMotionAfter = 0;
    gNdsStageMPDownWaitLoopRollBackGAAfter = 0u;
    gNdsStageMPDownWaitLoopRollBackJostleIgnoreAfter = 0u;
    gNdsStageMPDownWaitLoopRollBackProcCallbacksSetAfter = 0u;
    gNdsStageMPDownWaitLoopRollBackUpdateTickCount = 0u;
    gNdsStageMPDownWaitLoopRollBackPhysicsTickCount = 0u;
    gNdsStageMPDownWaitLoopRollBackMapTickCount = 0u;
    gNdsStageMPDownWaitLoopRollBackStableFrameCount = 0u;
    gNdsStageMPDownWaitLoopRollBackStatusAfterStable = 0u;
    gNdsStageMPDownWaitLoopRollBackMotionAfterStable = 0;
    gNdsStageMPDownWaitLoopRollBackGAAfterStable = 0u;
    gNdsStageMPDownWaitLoopRollBackWaitSetStatusCount = 0u;
    gNdsStageMPDownWaitLoopRollBackPlayerTagWaitCount = 0u;
    gNdsStageMPDownWaitLoopRollBackStatusAfterFinal = 0u;
    gNdsStageMPDownWaitLoopRollBackMotionAfterFinal = 0;
    gNdsStageMPDownWaitLoopRollBackGAAfterFinal = 0u;
    gNdsStageMPDownWaitLoopRollBackPlayerTagWaitAfterFinal = 0;
    gNdsStageMPDownWaitLoopRollBackProcCallbacksSetAfterFinal = 0u;
    sNdsStageMPDownWaitLoopDownWaitSetStatusActive = FALSE;
    sNdsStageMPDownWaitLoopDownWaitInterruptActive = FALSE;
    sNdsStageMPDownWaitLoopDownStandSetStatusActive = FALSE;
    sNdsStageMPDownWaitLoopDownAttackSetStatusActive = FALSE;
    sNdsStageMPDownWaitLoopDownForwardBackSetStatusActive = FALSE;
    sNdsStageMPDownWaitLoopAttackProbeActive = FALSE;
    sNdsStageMPDownWaitLoopRollForwardProbeActive = FALSE;
    sNdsStageMPDownWaitLoopRollBackProbeActive = FALSE;
    sNdsStageMPDownWaitLoopAttackCallbackActive = FALSE;
    sNdsStageMPDownWaitLoopAttackUpdateActive = FALSE;
    sNdsStageMPDownWaitLoopRollForwardCallbackActive = FALSE;
    sNdsStageMPDownWaitLoopRollForwardUpdateActive = FALSE;
    sNdsStageMPDownWaitLoopRollBackCallbackActive = FALSE;
    sNdsStageMPDownWaitLoopRollBackUpdateActive = FALSE;
    sNdsStageMPDownWaitLoopDownStandInterruptActive = FALSE;
    sNdsStageMPDownWaitLoopDownStandCallbackActive = FALSE;
    sNdsStageMPDownWaitLoopDownStandUpdateActive = FALSE;
}

void ndsFighterMarioFoxStageMPDownWaitLoopPrepare(void)
{
    if ((ndsFighterMarioFoxStageMPDownWaitLoopProofEnabled() == FALSE) ||
        (gNdsStageMPDownWaitLoopPrepared != 0u))
    {
        return;
    }
    ndsFighterMarioFoxStageMPDownWaitLoopReset();
    gNdsStageMPDownWaitLoopPrepared = 1u;
    gNdsStageMPDownWaitLoopBasePassiveSeen =
        (gNdsStageMPPassiveLoopPrepared != 0u) ? 1u : 0u;
}

static void ndsStageMPDownWaitLoopPrimeDownBounce(FTStruct *fp,
                                                  GObj *fighter_gobj)
{
    DObj *root = DObjGetStruct(fighter_gobj);

    fp->status_id = nFTCommonStatusDownBounceU;
    fp->motion_id = nFTCommonMotionDownBounceU;
    fp->motion_script_id = nFTCommonMotionDownBounceU;
    fp->ga = nMPKineticsGround;
    fp->lr = 1;
    fp->capture_immune_mask = 0u;
    fp->is_special_interrupt = FALSE;
    fp->motion_vars.flags.flag1 = 1;
    fp->status_vars.common.downwait.stand_wait = 0;
    fp->status_vars.common.downbounce.attack_buffer = 0;
    fp->damage_mul = 1.0F;
    fp->vel_ground.x = 0.0F;
    fp->vel_ground.y = 0.0F;
    fp->physics.vel_ground = fp->vel_ground;
    fp->proc_update = ftCommonDownBounceProcUpdate;
    fp->proc_interrupt = NULL;
    fp->proc_physics = ftPhysicsApplyGroundVelFriction;
    fp->proc_map = mpCommonSetFighterFallOnGroundBreak;
    fp->proc_damage = NULL;
    fp->input.pl.button_hold = 0u;
    fp->input.pl.button_tap = 0u;
    fp->input.pl.button_release = 0u;
    fp->input.pl.stick_range.x = 0;
    fp->input.pl.stick_range.y = 0;
    fp->anim_frame = 0.0F;
    if (fighter_gobj != NULL)
    {
        fighter_gobj->anim_frame = 0.0F;
    }
    if (root != NULL)
    {
        root->anim_speed = 1.0F;
    }
    if (fp->input.button_mask_z == 0u)
    {
        fp->input.button_mask_z = Z_TRIG;
    }
}

static void ndsStageMPDownWaitLoopRunDownStandCallbacks(GObj *fighter_gobj,
                                                        FTStruct *fp)
{
    u32 i;

    if ((fighter_gobj == NULL) || (fp == NULL) ||
        (fp->status_id != nFTCommonStatusDownStandU) ||
        (fp->motion_id != nFTCommonMotionDownStandU) ||
        (fp->ga != nMPKineticsGround) ||
        (fp->proc_update != ftAnimEndSetWait) ||
        (fp->proc_interrupt != ftCommonDownStandProcInterrupt) ||
        (fp->proc_physics != ftPhysicsApplyGroundVelFriction) ||
        (fp->proc_map != mpCommonSetFighterFallOnGroundBreak) ||
        (fp->proc_damage != NULL))
    {
        gNdsStageMPDownWaitLoopUnsafeCount++;
        return;
    }

    fp->input.pl.button_hold = 0u;
    fp->input.pl.button_release = 0u;
    fp->input.pl.button_tap = 0u;
    fp->input.pl.stick_range.x = 0;
    fp->input.pl.stick_range.y = 0;
    fp->motion_vars.flags.flag1 = 1;

    for (i = 0; i < NDS_STAGE_MPDOWNWAIT_RECOVERY_STABLE_FRAMES; i++)
    {
        f32 frame = (f32)(NDS_STAGE_MPDOWNWAIT_RECOVERY_STABLE_FRAMES - i);

        fighter_gobj->anim_frame = frame;
        fp->anim_frame = frame;

        sNdsStageMPDownWaitLoopDownStandInterruptActive = TRUE;
        fp->proc_interrupt(fighter_gobj);
        sNdsStageMPDownWaitLoopDownStandInterruptActive = FALSE;
        gNdsStageMPDownWaitLoopDownStandFlag1AfterProcInterrupt =
            (u32)fp->motion_vars.flags.flag1;

        sNdsStageMPDownWaitLoopDownStandCallbackActive = TRUE;
        fp->proc_physics(fighter_gobj);
        fp->proc_map(fighter_gobj);
        sNdsStageMPDownWaitLoopDownStandCallbackActive = FALSE;

        sNdsStageMPDownWaitLoopDownStandUpdateActive = TRUE;
        fp->proc_update(fighter_gobj);
        sNdsStageMPDownWaitLoopDownStandUpdateActive = FALSE;

        if ((fp->status_id == nFTCommonStatusDownStandU) &&
            (fp->motion_id == nFTCommonMotionDownStandU) &&
            (fp->ga == nMPKineticsGround))
        {
            gNdsStageMPDownWaitLoopDownStandStableFrameCount++;
        }
        else
        {
            gNdsStageMPDownWaitLoopUnsafeCount++;
            return;
        }
    }
    gNdsStageMPDownWaitLoopDownStandStatusAfterStable =
        (u32)fp->status_id;
    gNdsStageMPDownWaitLoopDownStandMotionAfterStable = fp->motion_id;
    gNdsStageMPDownWaitLoopDownStandGAAfterStable = (u32)fp->ga;

    fighter_gobj->anim_frame = 0.0F;
    fp->anim_frame = 0.0F;
    sNdsStageMPDownWaitLoopDownStandUpdateActive = TRUE;
    fp->proc_update(fighter_gobj);
    sNdsStageMPDownWaitLoopDownStandUpdateActive = FALSE;

    gNdsStageMPDownWaitLoopDownStandStatusAfterFinal =
        (u32)fp->status_id;
    gNdsStageMPDownWaitLoopDownStandMotionAfterFinal = fp->motion_id;
    gNdsStageMPDownWaitLoopDownStandGAAfterFinal = (u32)fp->ga;
    gNdsStageMPDownWaitLoopDownStandPlayerTagWaitAfterFinal =
        fp->playertag_wait;
    gNdsStageMPDownWaitLoopDownStandProcCallbacksSetAfterFinal =
        ((fp->proc_update == NULL) &&
         (fp->proc_interrupt == ftCommonWaitProcInterrupt) &&
         (fp->proc_physics == ftPhysicsApplyGroundVelFriction) &&
         (fp->proc_map == mpCommonProcFighterOnCliffEdge) &&
         (fp->proc_damage == NULL) &&
         (fp->is_special_interrupt != FALSE)) ? 1u : 0u;
}

static void ndsStageMPDownWaitLoopRunProbe(void)
{
    FTStruct *fp = &sNdsFighterStructPool[1];
    GObj *fighter_gobj;

    if ((ndsFighterStructIsPoolPointer(fp) == FALSE) ||
        (fp->fighter_gobj == NULL))
    {
        gNdsStageMPDownWaitLoopUnsafeCount++;
        return;
    }

    fighter_gobj = fp->fighter_gobj;
    ndsStageMPDownWaitLoopPrimeDownBounce(fp, fighter_gobj);

    sNdsStageMPDownWaitLoopDownWaitSetStatusActive = TRUE;
    ftCommonDownWaitSetStatus(fighter_gobj);
    sNdsStageMPDownWaitLoopDownWaitSetStatusActive = FALSE;

    gNdsStageMPDownWaitLoopDownWaitStatusAfterSetup = (u32)fp->status_id;
    gNdsStageMPDownWaitLoopDownWaitMotionAfterSetup = fp->motion_id;
    gNdsStageMPDownWaitLoopDownWaitGAAfterSetup = (u32)fp->ga;
    gNdsStageMPDownWaitLoopDownWaitStandWaitAfterSetup =
        fp->status_vars.common.downwait.stand_wait;
    gNdsStageMPDownWaitLoopDownWaitCaptureMaskAfterSetup =
        (u32)fp->capture_immune_mask;
    gNdsStageMPDownWaitLoopDownWaitDamageMulMilli =
        ndsFloatToMilliSigned(fp->damage_mul);
    gNdsStageMPDownWaitLoopDownWaitProcCallbacksSetAfterSetup =
        ((fp->proc_update == ftCommonDownWaitProcUpdate) &&
         (fp->proc_interrupt == ftCommonDownWaitProcInterrupt) &&
         (fp->proc_physics == ftPhysicsApplyGroundVelFriction) &&
         (fp->proc_map == mpCommonSetFighterFallOnGroundBreak) &&
         (fp->proc_damage == NULL)) ? 1u : 0u;

    if (gNdsStageMPDownWaitLoopDownWaitProcCallbacksSetAfterSetup == 0u)
    {
        gNdsStageMPDownWaitLoopUnsafeCount++;
        return;
    }

    fp->input.pl.button_hold = 0u;
    fp->input.pl.button_release = 0u;
    fp->input.pl.button_tap = 0u;
    fp->input.pl.stick_range.x = 0;
    fp->input.pl.stick_range.y = 80;
    fp->motion_vars.flags.flag1 = 1;

    gNdsStageMPDownWaitLoopStatusBeforeInterrupt = (u32)fp->status_id;
    gNdsStageMPDownWaitLoopMotionBeforeInterrupt = fp->motion_id;
    gNdsStageMPDownWaitLoopGABeforeInterrupt = (u32)fp->ga;
    gNdsStageMPDownWaitLoopStandWaitBeforeInterrupt =
        fp->status_vars.common.downwait.stand_wait;
    gNdsStageMPDownWaitLoopFlag1BeforeInterrupt =
        (u32)fp->motion_vars.flags.flag1;
    gNdsStageMPDownWaitLoopInputStickX = fp->input.pl.stick_range.x;
    gNdsStageMPDownWaitLoopInputStickY = fp->input.pl.stick_range.y;
    gNdsStageMPDownWaitLoopInputButtonTap = (u32)fp->input.pl.button_tap;
    gNdsStageMPDownWaitLoopInputButtonMaskZ = (u32)fp->input.button_mask_z;

    gNdsStageMPDownWaitLoopInterruptCallCount++;
    sNdsStageMPDownWaitLoopDownWaitInterruptActive = TRUE;
    fp->proc_interrupt(fighter_gobj);
    sNdsStageMPDownWaitLoopDownWaitInterruptActive = FALSE;

    gNdsStageMPDownWaitLoopStatusAfterInterrupt = (u32)fp->status_id;
    gNdsStageMPDownWaitLoopMotionAfterInterrupt = fp->motion_id;
    gNdsStageMPDownWaitLoopGAAfterInterrupt = (u32)fp->ga;
    gNdsStageMPDownWaitLoopStandWaitAfterInterrupt =
        fp->status_vars.common.downwait.stand_wait;
    gNdsStageMPDownWaitLoopFlag1AfterInterrupt =
        (u32)fp->motion_vars.flags.flag1;
    gNdsStageMPDownWaitLoopDamageMulAfterMilli =
        ndsFloatToMilliSigned(fp->damage_mul);
    gNdsStageMPDownWaitLoopDownStandProcCallbacksSetAfter =
        ((fp->proc_update == ftAnimEndSetWait) &&
         (fp->proc_interrupt == ftCommonDownStandProcInterrupt) &&
         (fp->proc_physics == ftPhysicsApplyGroundVelFriction) &&
         (fp->proc_map == mpCommonSetFighterFallOnGroundBreak) &&
         (fp->proc_damage == NULL)) ? 1u : 0u;
    if (gNdsStageMPDownWaitLoopDownStandProcCallbacksSetAfter != 0u)
    {
        ndsStageMPDownWaitLoopRunDownStandCallbacks(fighter_gobj, fp);
    }
}

static void ndsStageMPDownWaitLoopPrimeDownWaitShell(FTStruct *fp,
                                                     GObj *fighter_gobj)
{
    DObj *root = DObjGetStruct(fighter_gobj);

    fp->status_id = nFTCommonStatusDownWaitU;
    fp->motion_id = -2;
    fp->motion_script_id = -2;
    fp->ga = nMPKineticsGround;
    fp->lr = 1;
    fp->capture_immune_mask = 0x33u;
    fp->is_special_interrupt = FALSE;
    fp->motion_vars.flags.flag1 = 1;
    fp->status_vars.common.downwait.stand_wait = FTCOMMON_DOWNWAIT_STAND_WAIT;
    fp->status_vars.common.downbounce.attack_buffer = 0;
    fp->damage_mul = 0.5F;
    fp->vel_ground.x = 0.0F;
    fp->vel_ground.y = 0.0F;
    fp->physics.vel_ground = fp->vel_ground;
    fp->proc_update = ftCommonDownWaitProcUpdate;
    fp->proc_interrupt = ftCommonDownWaitProcInterrupt;
    fp->proc_physics = ftPhysicsApplyGroundVelFriction;
    fp->proc_map = mpCommonSetFighterFallOnGroundBreak;
    fp->proc_damage = NULL;
    fp->input.pl.button_hold = 0u;
    fp->input.pl.button_tap = 0u;
    fp->input.pl.button_release = 0u;
    fp->input.pl.stick_range.x = 0;
    fp->input.pl.stick_range.y = 0;
    fp->anim_frame = 0.0F;
    if (fighter_gobj != NULL)
    {
        fighter_gobj->anim_frame = 0.0F;
    }
    if (root != NULL)
    {
        root->anim_speed = 1.0F;
    }
    if (fp->input.button_mask_a == 0u)
    {
        fp->input.button_mask_a = A_BUTTON;
    }
    if (fp->input.button_mask_b == 0u)
    {
        fp->input.button_mask_b = B_BUTTON;
    }
    if (fp->input.button_mask_z == 0u)
    {
        fp->input.button_mask_z = Z_TRIG;
    }
}

static void ndsStageMPDownWaitLoopRunRecoveryCallbacks(GObj *fighter_gobj,
                                                       FTStruct *fp,
                                                       s32 status_id,
                                                       s32 motion_id)
{
    u32 i;
    sb32 is_attack = (status_id == nFTCommonStatusDownAttackU) ? TRUE : FALSE;
    sb32 is_roll_forward =
        (status_id == nFTCommonStatusDownForwardU) ? TRUE : FALSE;
    sb32 is_roll_back =
        (status_id == nFTCommonStatusDownBackU) ? TRUE : FALSE;
    DObj *root = NULL;
    s32 root_x_before = 0;
    s32 root_x_after = 0;

    if ((fighter_gobj == NULL) || (fp == NULL) ||
        (fp->proc_update != ftAnimEndSetWait) ||
        (fp->proc_physics == NULL) || (fp->proc_map == NULL))
    {
        gNdsStageMPDownWaitLoopUnsafeCount++;
        return;
    }

    if ((is_attack == FALSE) && (is_roll_forward == FALSE) &&
        (is_roll_back == FALSE))
    {
        gNdsStageMPDownWaitLoopUnsafeCount++;
        return;
    }

    if ((is_roll_forward != FALSE) || (is_roll_back != FALSE))
    {
        root = DObjGetStruct(fighter_gobj);
        if (root == NULL)
        {
            gNdsStageMPDownWaitLoopUnsafeCount++;
            return;
        }
        root_x_before = ndsFloatToMilliSigned(root->translate.vec.f.x);
        if (is_roll_forward != FALSE)
        {
            gNdsStageMPDownWaitLoopRollForwardRootXBeforeMilli =
                root_x_before;
        }
        else
        {
            gNdsStageMPDownWaitLoopRollBackRootXBeforeMilli =
                root_x_before;
        }
    }

    for (i = 0; i < NDS_STAGE_MPDOWNWAIT_RECOVERY_STABLE_FRAMES; i++)
    {
        f32 frame = (f32)(NDS_STAGE_MPDOWNWAIT_RECOVERY_STABLE_FRAMES - i);

        fighter_gobj->anim_frame = frame;
        fp->anim_frame = frame;

        if (is_attack != FALSE)
        {
            sNdsStageMPDownWaitLoopAttackCallbackActive = TRUE;
        }
        else if (is_roll_forward != FALSE)
        {
            sNdsStageMPDownWaitLoopRollForwardCallbackActive = TRUE;
        }
        else
        {
            sNdsStageMPDownWaitLoopRollBackCallbackActive = TRUE;
        }
        fp->proc_physics(fighter_gobj);
        fp->proc_map(fighter_gobj);
        sNdsStageMPDownWaitLoopAttackCallbackActive = FALSE;
        sNdsStageMPDownWaitLoopRollForwardCallbackActive = FALSE;
        sNdsStageMPDownWaitLoopRollBackCallbackActive = FALSE;

        if (is_attack != FALSE)
        {
            sNdsStageMPDownWaitLoopAttackUpdateActive = TRUE;
        }
        else if (is_roll_forward != FALSE)
        {
            sNdsStageMPDownWaitLoopRollForwardUpdateActive = TRUE;
        }
        else
        {
            sNdsStageMPDownWaitLoopRollBackUpdateActive = TRUE;
        }
        fp->proc_update(fighter_gobj);
        sNdsStageMPDownWaitLoopAttackUpdateActive = FALSE;
        sNdsStageMPDownWaitLoopRollForwardUpdateActive = FALSE;
        sNdsStageMPDownWaitLoopRollBackUpdateActive = FALSE;

        if ((fp->status_id == status_id) && (fp->motion_id == motion_id) &&
            (fp->ga == nMPKineticsGround))
        {
            if (is_attack != FALSE)
            {
                gNdsStageMPDownWaitLoopAttackStableFrameCount++;
            }
            else if (is_roll_forward != FALSE)
            {
                gNdsStageMPDownWaitLoopRollForwardStableFrameCount++;
            }
            else
            {
                gNdsStageMPDownWaitLoopRollBackStableFrameCount++;
            }
        }
        else
        {
            gNdsStageMPDownWaitLoopUnsafeCount++;
            return;
        }
    }

    if (is_attack != FALSE)
    {
        gNdsStageMPDownWaitLoopAttackStatusAfterStable =
            (u32)fp->status_id;
        gNdsStageMPDownWaitLoopAttackMotionAfterStable = fp->motion_id;
        gNdsStageMPDownWaitLoopAttackGAAfterStable = (u32)fp->ga;
    }
    else if (is_roll_forward != FALSE)
    {
        gNdsStageMPDownWaitLoopRollForwardStatusAfterStable =
            (u32)fp->status_id;
        gNdsStageMPDownWaitLoopRollForwardMotionAfterStable = fp->motion_id;
        gNdsStageMPDownWaitLoopRollForwardGAAfterStable = (u32)fp->ga;
        root_x_after = ndsFloatToMilliSigned(root->translate.vec.f.x);
        gNdsStageMPDownWaitLoopRollForwardRootXAfterStableMilli =
            root_x_after;
        gNdsStageMPDownWaitLoopRollForwardRootDeltaXMilli =
            root_x_after - root_x_before;
        if (gNdsStageMPDownWaitLoopRollForwardRootDeltaXMilli > 0)
        {
            gNdsStageMPDownWaitLoopRollMoveMask |= 1u << 0;
        }
    }
    else
    {
        gNdsStageMPDownWaitLoopRollBackStatusAfterStable =
            (u32)fp->status_id;
        gNdsStageMPDownWaitLoopRollBackMotionAfterStable = fp->motion_id;
        gNdsStageMPDownWaitLoopRollBackGAAfterStable = (u32)fp->ga;
        root_x_after = ndsFloatToMilliSigned(root->translate.vec.f.x);
        gNdsStageMPDownWaitLoopRollBackRootXAfterStableMilli =
            root_x_after;
        gNdsStageMPDownWaitLoopRollBackRootDeltaXMilli =
            root_x_after - root_x_before;
        if (gNdsStageMPDownWaitLoopRollBackRootDeltaXMilli < 0)
        {
            gNdsStageMPDownWaitLoopRollMoveMask |= 1u << 1;
        }
    }

    fighter_gobj->anim_frame = 0.0F;
    fp->anim_frame = 0.0F;
    if (is_attack != FALSE)
    {
        sNdsStageMPDownWaitLoopAttackUpdateActive = TRUE;
    }
    else if (is_roll_forward != FALSE)
    {
        sNdsStageMPDownWaitLoopRollForwardUpdateActive = TRUE;
    }
    else
    {
        sNdsStageMPDownWaitLoopRollBackUpdateActive = TRUE;
    }
    fp->proc_update(fighter_gobj);
    sNdsStageMPDownWaitLoopAttackUpdateActive = FALSE;
    sNdsStageMPDownWaitLoopRollForwardUpdateActive = FALSE;
    sNdsStageMPDownWaitLoopRollBackUpdateActive = FALSE;

    if (is_attack != FALSE)
    {
        gNdsStageMPDownWaitLoopAttackStatusAfterFinal =
            (u32)fp->status_id;
        gNdsStageMPDownWaitLoopAttackMotionAfterFinal = fp->motion_id;
        gNdsStageMPDownWaitLoopAttackGAAfterFinal = (u32)fp->ga;
        gNdsStageMPDownWaitLoopAttackPlayerTagWaitAfterFinal =
            fp->playertag_wait;
        gNdsStageMPDownWaitLoopAttackProcCallbacksSetAfterFinal =
            ((fp->proc_update == NULL) &&
             (fp->proc_interrupt == ftCommonWaitProcInterrupt) &&
             (fp->proc_physics == ftPhysicsApplyGroundVelFriction) &&
             (fp->proc_map == mpCommonProcFighterOnCliffEdge) &&
             (fp->proc_damage == NULL) &&
             (fp->is_special_interrupt != FALSE)) ? 1u : 0u;
    }
    else if (is_roll_forward != FALSE)
    {
        gNdsStageMPDownWaitLoopRollForwardStatusAfterFinal =
            (u32)fp->status_id;
        gNdsStageMPDownWaitLoopRollForwardMotionAfterFinal = fp->motion_id;
        gNdsStageMPDownWaitLoopRollForwardGAAfterFinal = (u32)fp->ga;
        gNdsStageMPDownWaitLoopRollForwardPlayerTagWaitAfterFinal =
            fp->playertag_wait;
        gNdsStageMPDownWaitLoopRollForwardProcCallbacksSetAfterFinal =
            ((fp->proc_update == NULL) &&
             (fp->proc_interrupt == ftCommonWaitProcInterrupt) &&
             (fp->proc_physics == ftPhysicsApplyGroundVelFriction) &&
             (fp->proc_map == mpCommonProcFighterOnCliffEdge) &&
             (fp->proc_damage == NULL) &&
             (fp->is_special_interrupt != FALSE)) ? 1u : 0u;
    }
    else
    {
        gNdsStageMPDownWaitLoopRollBackStatusAfterFinal =
            (u32)fp->status_id;
        gNdsStageMPDownWaitLoopRollBackMotionAfterFinal = fp->motion_id;
        gNdsStageMPDownWaitLoopRollBackGAAfterFinal = (u32)fp->ga;
        gNdsStageMPDownWaitLoopRollBackPlayerTagWaitAfterFinal =
            fp->playertag_wait;
        gNdsStageMPDownWaitLoopRollBackProcCallbacksSetAfterFinal =
            ((fp->proc_update == NULL) &&
             (fp->proc_interrupt == ftCommonWaitProcInterrupt) &&
             (fp->proc_physics == ftPhysicsApplyGroundVelFriction) &&
             (fp->proc_map == mpCommonProcFighterOnCliffEdge) &&
             (fp->proc_damage == NULL) &&
             (fp->is_special_interrupt != FALSE)) ? 1u : 0u;
    }
}

static void ndsStageMPDownWaitLoopRunAttackProbe(void)
{
    FTStruct *fp = &sNdsFighterStructPool[1];
    GObj *fighter_gobj;

    if ((ndsFighterStructIsPoolPointer(fp) == FALSE) ||
        (fp->fighter_gobj == NULL))
    {
        gNdsStageMPDownWaitLoopUnsafeCount++;
        return;
    }

    fighter_gobj = fp->fighter_gobj;
    ndsStageMPDownWaitLoopPrimeDownWaitShell(fp, fighter_gobj);

    fp->input.pl.button_tap = fp->input.button_mask_a;
    fp->input.pl.stick_range.x = 0;
    fp->input.pl.stick_range.y = 0;

    gNdsStageMPDownWaitLoopAttackStatusBefore = (u32)fp->status_id;
    gNdsStageMPDownWaitLoopAttackMotionBefore = fp->motion_id;
    gNdsStageMPDownWaitLoopAttackGABefore = (u32)fp->ga;
    gNdsStageMPDownWaitLoopAttackInputButtonTap =
        (u32)fp->input.pl.button_tap;
    gNdsStageMPDownWaitLoopAttackInputButtonMaskA =
        (u32)fp->input.button_mask_a;
    gNdsStageMPDownWaitLoopAttackInputButtonMaskB =
        (u32)fp->input.button_mask_b;

    gNdsStageMPDownWaitLoopAttackInterruptCallCount++;
    sNdsStageMPDownWaitLoopAttackProbeActive = TRUE;
    sNdsStageMPDownWaitLoopDownWaitInterruptActive = TRUE;
    fp->proc_interrupt(fighter_gobj);
    sNdsStageMPDownWaitLoopDownWaitInterruptActive = FALSE;
    sNdsStageMPDownWaitLoopAttackProbeActive = FALSE;

    gNdsStageMPDownWaitLoopAttackStatusAfter = (u32)fp->status_id;
    gNdsStageMPDownWaitLoopAttackMotionAfter = fp->motion_id;
    gNdsStageMPDownWaitLoopAttackGAAfter = (u32)fp->ga;
    gNdsStageMPDownWaitLoopAttackMotionAttackIDAfter =
        fp->motion_attack_id;
    gNdsStageMPDownWaitLoopAttackStatusAttackIDAfter =
        fp->status_attack_id;
    gNdsStageMPDownWaitLoopAttackStatAttackIDAfter =
        fp->stat_attack_id;
    gNdsStageMPDownWaitLoopAttackProcCallbacksSetAfter =
        ((fp->proc_update == ftAnimEndSetWait) &&
         (fp->proc_interrupt == NULL) &&
         (fp->proc_physics == ftPhysicsApplyGroundVelFriction) &&
         (fp->proc_map == mpCommonSetFighterFallOnEdgeBreak) &&
         (fp->proc_damage == NULL)) ? 1u : 0u;
    if (gNdsStageMPDownWaitLoopAttackProcCallbacksSetAfter != 0u)
    {
        ndsStageMPDownWaitLoopRunRecoveryCallbacks(
            fighter_gobj, fp, nFTCommonStatusDownAttackU,
            nFTCommonMotionDownAttackU);
    }
}

static void ndsStageMPDownWaitLoopRunRollProbe(sb32 is_forward)
{
    FTStruct *fp = &sNdsFighterStructPool[1];
    GObj *fighter_gobj;

    if ((ndsFighterStructIsPoolPointer(fp) == FALSE) ||
        (fp->fighter_gobj == NULL))
    {
        gNdsStageMPDownWaitLoopUnsafeCount++;
        return;
    }

    fighter_gobj = fp->fighter_gobj;
    ndsStageMPDownWaitLoopPrimeDownWaitShell(fp, fighter_gobj);

    fp->input.pl.button_tap = 0u;
    fp->input.pl.stick_range.x = is_forward ? 80 : -80;
    fp->input.pl.stick_range.y = 0;

    if (is_forward != FALSE)
    {
        gNdsStageMPDownWaitLoopRollForwardInputStickX =
            fp->input.pl.stick_range.x;
        gNdsStageMPDownWaitLoopRollForwardInputStickY =
            fp->input.pl.stick_range.y;
        sNdsStageMPDownWaitLoopRollForwardProbeActive = TRUE;
    }
    else
    {
        gNdsStageMPDownWaitLoopRollBackInputStickX =
            fp->input.pl.stick_range.x;
        gNdsStageMPDownWaitLoopRollBackInputStickY =
            fp->input.pl.stick_range.y;
        sNdsStageMPDownWaitLoopRollBackProbeActive = TRUE;
    }

    gNdsStageMPDownWaitLoopRollInterruptCallCount++;
    sNdsStageMPDownWaitLoopDownWaitInterruptActive = TRUE;
    fp->proc_interrupt(fighter_gobj);
    sNdsStageMPDownWaitLoopDownWaitInterruptActive = FALSE;
    sNdsStageMPDownWaitLoopRollForwardProbeActive = FALSE;
    sNdsStageMPDownWaitLoopRollBackProbeActive = FALSE;

    if (is_forward != FALSE)
    {
        gNdsStageMPDownWaitLoopRollForwardStatusAfter = (u32)fp->status_id;
        gNdsStageMPDownWaitLoopRollForwardMotionAfter = fp->motion_id;
        gNdsStageMPDownWaitLoopRollForwardGAAfter = (u32)fp->ga;
        gNdsStageMPDownWaitLoopRollForwardJostleIgnoreAfter =
            (fp->is_jostle_ignore != FALSE) ? 1u : 0u;
        gNdsStageMPDownWaitLoopRollForwardProcCallbacksSetAfter =
            ((fp->proc_update == ftAnimEndSetWait) &&
             (fp->proc_interrupt == NULL) &&
             (fp->proc_physics == ftPhysicsApplyGroundVelTransN) &&
             (fp->proc_map == mpCommonSetFighterFallOnEdgeBreak) &&
             (fp->proc_damage == NULL)) ? 1u : 0u;
        if (gNdsStageMPDownWaitLoopRollForwardProcCallbacksSetAfter != 0u)
        {
            ndsStageMPDownWaitLoopRunRecoveryCallbacks(
                fighter_gobj, fp, nFTCommonStatusDownForwardU,
                nFTCommonMotionDownForwardU);
        }
    }
    else
    {
        gNdsStageMPDownWaitLoopRollBackStatusAfter = (u32)fp->status_id;
        gNdsStageMPDownWaitLoopRollBackMotionAfter = fp->motion_id;
        gNdsStageMPDownWaitLoopRollBackGAAfter = (u32)fp->ga;
        gNdsStageMPDownWaitLoopRollBackJostleIgnoreAfter =
            (fp->is_jostle_ignore != FALSE) ? 1u : 0u;
        gNdsStageMPDownWaitLoopRollBackProcCallbacksSetAfter =
            ((fp->proc_update == ftAnimEndSetWait) &&
             (fp->proc_interrupt == NULL) &&
             (fp->proc_physics == ftPhysicsApplyGroundVelTransN) &&
             (fp->proc_map == mpCommonSetFighterFallOnEdgeBreak) &&
             (fp->proc_damage == NULL)) ? 1u : 0u;
        if (gNdsStageMPDownWaitLoopRollBackProcCallbacksSetAfter != 0u)
        {
            ndsStageMPDownWaitLoopRunRecoveryCallbacks(
                fighter_gobj, fp, nFTCommonStatusDownBackU,
                nFTCommonMotionDownBackU);
        }
    }
}

void ndsFighterMarioFoxStageMPDownWaitLoopFinalize(void)
{
    u32 mask = 0u;

    if ((ndsFighterMarioFoxStageMPDownWaitLoopProofEnabled() == FALSE) ||
        (gNdsStageMPDownWaitLoopPrepared == 0u) ||
        (gNdsFighterMarioFoxStageMPDownWaitLoopResult != 0u))
    {
        return;
    }

    if (gNdsFighterMarioFoxStageMPPassiveLoopResult == 0u)
    {
        ndsFighterMarioFoxStageMPPassiveLoopFinalize();
    }
    if ((gNdsFighterMarioFoxStageMPPassiveLoopResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPPASSIVE_LOOP_PASS) &&
        (gNdsFighterMarioFoxStageMPPassiveLoopSafeResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPPASSIVE_LOOP_SAFE_PASS))
    {
        gNdsStageMPDownWaitLoopBasePassiveSeen = 1u;
        mask |= 1u << 0;
    }
    if ((gNdsStageMPDownWaitLoopPrepared != 0u) &&
        (gNdsStageMPDownWaitLoopBasePassiveSeen != 0u))
    {
        mask |= 1u << 1;
    }

    if ((gNdsStageMPDownWaitLoopAttackCheckCount == 0u) &&
        (gNdsStageMPDownWaitLoopUnsafeCount == 0u))
    {
        ndsStageMPDownWaitLoopRunAttackProbe();
    }
    if ((gNdsStageMPDownWaitLoopRollForwardStatusAfter == 0u) &&
        (gNdsStageMPDownWaitLoopUnsafeCount == 0u))
    {
        ndsStageMPDownWaitLoopRunRollProbe(TRUE);
    }
    if ((gNdsStageMPDownWaitLoopRollBackStatusAfter == 0u) &&
        (gNdsStageMPDownWaitLoopUnsafeCount == 0u))
    {
        ndsStageMPDownWaitLoopRunRollProbe(FALSE);
    }
    if ((gNdsStageMPDownWaitLoopDownWaitSetStatusCount == 0u) &&
        (gNdsStageMPDownWaitLoopUnsafeCount == 0u))
    {
        ndsStageMPDownWaitLoopRunProbe();
    }

    if ((gNdsStageMPDownWaitLoopDownWaitSetStatusCount == 1u) &&
        (gNdsStageMPDownWaitLoopDownWaitMainSetStatusCount == 1u) &&
        (gNdsStageMPDownWaitLoopDownWaitCaptureImmuneCount >= 1u) &&
        (gNdsStageMPDownWaitLoopDownWaitStatusAfterSetup ==
            (u32)nFTCommonStatusDownWaitU) &&
        (gNdsStageMPDownWaitLoopDownWaitMotionAfterSetup == -2) &&
        (gNdsStageMPDownWaitLoopDownWaitGAAfterSetup ==
            (u32)nMPKineticsGround) &&
        (gNdsStageMPDownWaitLoopDownWaitStandWaitAfterSetup ==
            FTCOMMON_DOWNWAIT_STAND_WAIT) &&
        (gNdsStageMPDownWaitLoopDownWaitCaptureMaskAfterSetup == 0x33u) &&
        (gNdsStageMPDownWaitLoopDownWaitDamageMulMilli == 500) &&
        (gNdsStageMPDownWaitLoopDownWaitProcCallbacksSetAfterSetup == 1u))
    {
        mask |= 1u << 2;
    }
    if ((gNdsStageMPDownWaitLoopInputStickX == 0) &&
        (gNdsStageMPDownWaitLoopInputStickY >=
            FTCOMMON_DOWNWAIT_STAND_STICK_RANGE_MIN) &&
        (gNdsStageMPDownWaitLoopInputButtonTap == 0u) &&
        (gNdsStageMPDownWaitLoopInputButtonMaskZ != 0u))
    {
        mask |= 1u << 3;
    }
    if ((gNdsStageMPDownWaitLoopStatusBeforeInterrupt ==
            (u32)nFTCommonStatusDownWaitU) &&
        (gNdsStageMPDownWaitLoopMotionBeforeInterrupt == -2) &&
        (gNdsStageMPDownWaitLoopGABeforeInterrupt ==
            (u32)nMPKineticsGround) &&
        (gNdsStageMPDownWaitLoopStandWaitBeforeInterrupt ==
            FTCOMMON_DOWNWAIT_STAND_WAIT) &&
        (gNdsStageMPDownWaitLoopFlag1BeforeInterrupt == 1u))
    {
        mask |= 1u << 4;
    }
    if ((gNdsStageMPDownWaitLoopInterruptCallCount == 1u) &&
        (gNdsStageMPDownWaitLoopDownAttackCheckCount == 1u) &&
        (gNdsStageMPDownWaitLoopForwardBackCheckCount == 1u) &&
        (gNdsStageMPDownWaitLoopDownStandCheckCount == 1u) &&
        (gNdsStageMPDownWaitLoopDownStandSetStatusCount == 1u) &&
        (gNdsStageMPDownWaitLoopDownStandMainSetStatusCount == 1u) &&
        (gNdsStageMPDownWaitLoopSourceOrder == 0x12345u))
    {
        mask |= 1u << 5;
    }
    if ((gNdsStageMPDownWaitLoopStatusAfterInterrupt ==
            (u32)nFTCommonStatusDownStandU) &&
        (gNdsStageMPDownWaitLoopMotionAfterInterrupt ==
            nFTCommonMotionDownStandU) &&
        (gNdsStageMPDownWaitLoopGAAfterInterrupt ==
            (u32)nMPKineticsGround) &&
        (gNdsStageMPDownWaitLoopFlag1AfterInterrupt == 0u) &&
        (gNdsStageMPDownWaitLoopDamageMulAfterMilli == 1000) &&
        (gNdsStageMPDownWaitLoopDownStandProcCallbacksSetAfter == 1u))
    {
        mask |= 1u << 6;
    }
    if ((gNdsStageMPDownWaitLoopAttackInterruptCallCount == 1u) &&
        (gNdsStageMPDownWaitLoopAttackCheckCount == 1u) &&
        (gNdsStageMPDownWaitLoopAttackSetStatusCount == 1u) &&
        (gNdsStageMPDownWaitLoopAttackMainSetStatusCount == 1u) &&
        (gNdsStageMPDownWaitLoopAttackAnimEventsCount == 1u) &&
        (gNdsStageMPDownWaitLoopAttackSourceOrder == 0x1234u) &&
        (gNdsStageMPDownWaitLoopAttackInputButtonTap != 0u) &&
        (gNdsStageMPDownWaitLoopAttackInputButtonMaskA != 0u) &&
        (gNdsStageMPDownWaitLoopAttackStatusBefore ==
            (u32)nFTCommonStatusDownWaitU) &&
        (gNdsStageMPDownWaitLoopAttackMotionBefore == -2) &&
        (gNdsStageMPDownWaitLoopAttackGABefore ==
            (u32)nMPKineticsGround) &&
        (gNdsStageMPDownWaitLoopAttackStatusAfter ==
            (u32)nFTCommonStatusDownAttackU) &&
        (gNdsStageMPDownWaitLoopAttackMotionAfter ==
            nFTCommonMotionDownAttackU) &&
        (gNdsStageMPDownWaitLoopAttackGAAfter ==
            (u32)nMPKineticsGround) &&
        (gNdsStageMPDownWaitLoopAttackMotionAttackIDAfter ==
            nFTMotionAttackIDDownAttackU) &&
        (gNdsStageMPDownWaitLoopAttackStatusAttackIDAfter ==
            nFTStatusAttackIDDownAttackU) &&
        (gNdsStageMPDownWaitLoopAttackStatAttackIDAfter ==
            nFTStatusAttackIDDownAttackU) &&
        (gNdsStageMPDownWaitLoopAttackProcCallbacksSetAfter == 1u))
    {
        mask |= 1u << 7;
    }
    if ((gNdsStageMPDownWaitLoopRollInterruptCallCount == 2u) &&
        (gNdsStageMPDownWaitLoopRollAttackCheckCount == 2u) &&
        (gNdsStageMPDownWaitLoopRollForwardBackCheckCount == 2u) &&
        (gNdsStageMPDownWaitLoopRollSetStatusCount == 2u) &&
        (gNdsStageMPDownWaitLoopRollMainSetStatusCount == 2u) &&
        (gNdsStageMPDownWaitLoopRollAnimEventsCount == 2u) &&
        (gNdsStageMPDownWaitLoopRollForwardSourceOrder == 0x12345u) &&
        (gNdsStageMPDownWaitLoopRollBackSourceOrder == 0x12345u))
    {
        mask |= 1u << 8;
    }
    if ((gNdsStageMPDownWaitLoopRollForwardInputStickX >=
            FTCOMMON_DOWN_FORWARD_BACK_RANGE_MIN) &&
        (gNdsStageMPDownWaitLoopRollForwardInputStickY == 0) &&
        (gNdsStageMPDownWaitLoopRollForwardStatusAfter ==
            (u32)nFTCommonStatusDownForwardU) &&
        (gNdsStageMPDownWaitLoopRollForwardMotionAfter ==
            nFTCommonMotionDownForwardU) &&
        (gNdsStageMPDownWaitLoopRollForwardGAAfter ==
            (u32)nMPKineticsGround) &&
        (gNdsStageMPDownWaitLoopRollForwardJostleIgnoreAfter == 1u) &&
        (gNdsStageMPDownWaitLoopRollForwardProcCallbacksSetAfter == 1u) &&
        (gNdsStageMPDownWaitLoopRollBackInputStickX <=
            -FTCOMMON_DOWN_FORWARD_BACK_RANGE_MIN) &&
        (gNdsStageMPDownWaitLoopRollBackInputStickY == 0) &&
        (gNdsStageMPDownWaitLoopRollBackStatusAfter ==
            (u32)nFTCommonStatusDownBackU) &&
        (gNdsStageMPDownWaitLoopRollBackMotionAfter ==
            nFTCommonMotionDownBackU) &&
        (gNdsStageMPDownWaitLoopRollBackGAAfter ==
            (u32)nMPKineticsGround) &&
        (gNdsStageMPDownWaitLoopRollBackJostleIgnoreAfter == 1u) &&
        (gNdsStageMPDownWaitLoopRollBackProcCallbacksSetAfter == 1u))
    {
        mask |= 1u << 9;
    }
    if ((gNdsStageMPDownWaitLoopAttackUpdateTickCount ==
            (NDS_STAGE_MPDOWNWAIT_RECOVERY_STABLE_FRAMES + 1u)) &&
        (gNdsStageMPDownWaitLoopAttackPhysicsTickCount ==
            NDS_STAGE_MPDOWNWAIT_RECOVERY_STABLE_FRAMES) &&
        (gNdsStageMPDownWaitLoopAttackMapTickCount ==
            NDS_STAGE_MPDOWNWAIT_RECOVERY_STABLE_FRAMES) &&
        (gNdsStageMPDownWaitLoopAttackStableFrameCount ==
            NDS_STAGE_MPDOWNWAIT_RECOVERY_STABLE_FRAMES) &&
        (gNdsStageMPDownWaitLoopAttackStatusAfterStable ==
            (u32)nFTCommonStatusDownAttackU) &&
        (gNdsStageMPDownWaitLoopAttackMotionAfterStable ==
            nFTCommonMotionDownAttackU) &&
        (gNdsStageMPDownWaitLoopAttackGAAfterStable ==
            (u32)nMPKineticsGround) &&
        (gNdsStageMPDownWaitLoopAttackWaitSetStatusCount == 1u) &&
        (gNdsStageMPDownWaitLoopAttackPlayerTagWaitCount == 1u) &&
        (gNdsStageMPDownWaitLoopAttackStatusAfterFinal ==
            (u32)nFTCommonStatusWait) &&
        (gNdsStageMPDownWaitLoopAttackMotionAfterFinal ==
            nFTCommonMotionWait) &&
        (gNdsStageMPDownWaitLoopAttackGAAfterFinal ==
            (u32)nMPKineticsGround) &&
        (gNdsStageMPDownWaitLoopAttackPlayerTagWaitAfterFinal == 120) &&
        (gNdsStageMPDownWaitLoopAttackProcCallbacksSetAfterFinal == 1u))
    {
        mask |= 1u << 10;
    }
    if ((gNdsStageMPDownWaitLoopRollForwardUpdateTickCount ==
            (NDS_STAGE_MPDOWNWAIT_RECOVERY_STABLE_FRAMES + 1u)) &&
        (gNdsStageMPDownWaitLoopRollForwardPhysicsTickCount ==
            NDS_STAGE_MPDOWNWAIT_RECOVERY_STABLE_FRAMES) &&
        (gNdsStageMPDownWaitLoopRollForwardMapTickCount ==
            NDS_STAGE_MPDOWNWAIT_RECOVERY_STABLE_FRAMES) &&
        (gNdsStageMPDownWaitLoopRollForwardStableFrameCount ==
            NDS_STAGE_MPDOWNWAIT_RECOVERY_STABLE_FRAMES) &&
        (gNdsStageMPDownWaitLoopRollForwardStatusAfterStable ==
            (u32)nFTCommonStatusDownForwardU) &&
        (gNdsStageMPDownWaitLoopRollForwardMotionAfterStable ==
            nFTCommonMotionDownForwardU) &&
        (gNdsStageMPDownWaitLoopRollForwardGAAfterStable ==
            (u32)nMPKineticsGround) &&
        (gNdsStageMPDownWaitLoopRollForwardWaitSetStatusCount == 1u) &&
        (gNdsStageMPDownWaitLoopRollForwardPlayerTagWaitCount == 1u) &&
        (gNdsStageMPDownWaitLoopRollForwardStatusAfterFinal ==
            (u32)nFTCommonStatusWait) &&
        (gNdsStageMPDownWaitLoopRollForwardMotionAfterFinal ==
            nFTCommonMotionWait) &&
        (gNdsStageMPDownWaitLoopRollForwardGAAfterFinal ==
            (u32)nMPKineticsGround) &&
        (gNdsStageMPDownWaitLoopRollForwardPlayerTagWaitAfterFinal == 120) &&
        (gNdsStageMPDownWaitLoopRollForwardProcCallbacksSetAfterFinal == 1u))
    {
        mask |= 1u << 11;
    }
    if ((gNdsStageMPDownWaitLoopRollBackUpdateTickCount ==
            (NDS_STAGE_MPDOWNWAIT_RECOVERY_STABLE_FRAMES + 1u)) &&
        (gNdsStageMPDownWaitLoopRollBackPhysicsTickCount ==
            NDS_STAGE_MPDOWNWAIT_RECOVERY_STABLE_FRAMES) &&
        (gNdsStageMPDownWaitLoopRollBackMapTickCount ==
            NDS_STAGE_MPDOWNWAIT_RECOVERY_STABLE_FRAMES) &&
        (gNdsStageMPDownWaitLoopRollBackStableFrameCount ==
            NDS_STAGE_MPDOWNWAIT_RECOVERY_STABLE_FRAMES) &&
        (gNdsStageMPDownWaitLoopRollBackStatusAfterStable ==
            (u32)nFTCommonStatusDownBackU) &&
        (gNdsStageMPDownWaitLoopRollBackMotionAfterStable ==
            nFTCommonMotionDownBackU) &&
        (gNdsStageMPDownWaitLoopRollBackGAAfterStable ==
            (u32)nMPKineticsGround) &&
        (gNdsStageMPDownWaitLoopRollBackWaitSetStatusCount == 1u) &&
        (gNdsStageMPDownWaitLoopRollBackPlayerTagWaitCount == 1u) &&
        (gNdsStageMPDownWaitLoopRollBackStatusAfterFinal ==
            (u32)nFTCommonStatusWait) &&
        (gNdsStageMPDownWaitLoopRollBackMotionAfterFinal ==
            nFTCommonMotionWait) &&
        (gNdsStageMPDownWaitLoopRollBackGAAfterFinal ==
            (u32)nMPKineticsGround) &&
        (gNdsStageMPDownWaitLoopRollBackPlayerTagWaitAfterFinal == 120) &&
        (gNdsStageMPDownWaitLoopRollBackProcCallbacksSetAfterFinal == 1u))
    {
        mask |= 1u << 12;
    }
    if ((gNdsStageMPDownWaitLoopDownStandInterruptTickCount ==
            NDS_STAGE_MPDOWNWAIT_RECOVERY_STABLE_FRAMES) &&
        (gNdsStageMPDownWaitLoopDownStandKneeBendCheckCount ==
            NDS_STAGE_MPDOWNWAIT_RECOVERY_STABLE_FRAMES) &&
        (gNdsStageMPDownWaitLoopDownStandPassCheckCount ==
            NDS_STAGE_MPDOWNWAIT_RECOVERY_STABLE_FRAMES) &&
        (gNdsStageMPDownWaitLoopDownStandDokanCheckCount ==
            NDS_STAGE_MPDOWNWAIT_RECOVERY_STABLE_FRAMES) &&
        (gNdsStageMPDownWaitLoopDownStandFlag1AfterProcInterrupt == 1u))
    {
        mask |= 1u << 16;
    }
    if ((gNdsStageMPDownWaitLoopDownStandUpdateTickCount ==
            (NDS_STAGE_MPDOWNWAIT_RECOVERY_STABLE_FRAMES + 1u)) &&
        (gNdsStageMPDownWaitLoopDownStandPhysicsTickCount ==
            NDS_STAGE_MPDOWNWAIT_RECOVERY_STABLE_FRAMES) &&
        (gNdsStageMPDownWaitLoopDownStandMapTickCount ==
            NDS_STAGE_MPDOWNWAIT_RECOVERY_STABLE_FRAMES) &&
        (gNdsStageMPDownWaitLoopDownStandStableFrameCount ==
            NDS_STAGE_MPDOWNWAIT_RECOVERY_STABLE_FRAMES) &&
        (gNdsStageMPDownWaitLoopDownStandStatusAfterStable ==
            (u32)nFTCommonStatusDownStandU) &&
        (gNdsStageMPDownWaitLoopDownStandMotionAfterStable ==
            nFTCommonMotionDownStandU) &&
        (gNdsStageMPDownWaitLoopDownStandGAAfterStable ==
            (u32)nMPKineticsGround) &&
        (gNdsStageMPDownWaitLoopDownStandWaitSetStatusCount == 1u) &&
        (gNdsStageMPDownWaitLoopDownStandPlayerTagWaitCount == 1u) &&
        (gNdsStageMPDownWaitLoopDownStandStatusAfterFinal ==
            (u32)nFTCommonStatusWait) &&
        (gNdsStageMPDownWaitLoopDownStandMotionAfterFinal ==
            nFTCommonMotionWait) &&
        (gNdsStageMPDownWaitLoopDownStandGAAfterFinal ==
            (u32)nMPKineticsGround) &&
        (gNdsStageMPDownWaitLoopDownStandPlayerTagWaitAfterFinal == 120) &&
        (gNdsStageMPDownWaitLoopDownStandProcCallbacksSetAfterFinal == 1u))
    {
        mask |= 1u << 17;
    }
    if ((gNdsStageMPDownWaitLoopUnsafeCount == 0u) &&
        (sNdsStageMPDownWaitLoopDownWaitSetStatusActive == FALSE) &&
        (sNdsStageMPDownWaitLoopDownWaitInterruptActive == FALSE) &&
        (sNdsStageMPDownWaitLoopDownStandSetStatusActive == FALSE))
    {
        mask |= 1u << 13;
    }
    if ((sNdsStageMPDownWaitLoopDownAttackSetStatusActive == FALSE) &&
        (sNdsStageMPDownWaitLoopDownForwardBackSetStatusActive == FALSE) &&
        (sNdsStageMPDownWaitLoopAttackProbeActive == FALSE) &&
        (sNdsStageMPDownWaitLoopRollForwardProbeActive == FALSE) &&
        (sNdsStageMPDownWaitLoopRollBackProbeActive == FALSE))
    {
        mask |= 1u << 14;
    }
    if ((sNdsStageMPDownWaitLoopAttackCallbackActive == FALSE) &&
        (sNdsStageMPDownWaitLoopAttackUpdateActive == FALSE) &&
        (sNdsStageMPDownWaitLoopRollForwardCallbackActive == FALSE) &&
        (sNdsStageMPDownWaitLoopRollForwardUpdateActive == FALSE) &&
        (sNdsStageMPDownWaitLoopRollBackCallbackActive == FALSE) &&
        (sNdsStageMPDownWaitLoopRollBackUpdateActive == FALSE) &&
        (sNdsStageMPDownWaitLoopDownStandInterruptActive == FALSE) &&
        (sNdsStageMPDownWaitLoopDownStandCallbackActive == FALSE) &&
        (sNdsStageMPDownWaitLoopDownStandUpdateActive == FALSE))
    {
        mask |= 1u << 15;
    }

    gNdsFighterMarioFoxStageMPDownWaitLoopCount = 1u;
    gNdsFighterMarioFoxStageMPDownWaitLoopMask = mask;
    gNdsFighterMarioFoxStageMPDownWaitLoopDeferredMask = 0x3ffffu;
    if ((mask & 0x3ffffu) == 0x3ffffu)
    {
        gNdsFighterMarioFoxStageMPDownWaitLoopResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPDOWNWAIT_LOOP_PASS;
        gNdsFighterMarioFoxStageMPDownWaitLoopSafeResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPDOWNWAIT_LOOP_SAFE_PASS;
    }
}

static void ndsFighterMarioFoxStageTurnLoopReset(void)
{
    gNdsFighterMarioFoxStageTurnLoopResult = 0u;
    gNdsFighterMarioFoxStageTurnLoopSafeResult = 0u;
    gNdsFighterMarioFoxStageTurnLoopMask = 0u;
    gNdsFighterMarioFoxStageTurnLoopDeferredMask = 0u;
    gNdsFighterMarioFoxStageTurnLoopCount = 0u;
    gNdsStageTurnLoopPrepared = 0u;
    gNdsStageTurnLoopBaseDownWaitSeen = 0u;
    gNdsStageTurnLoopUnsafeCount = 0u;
    gNdsStageTurnLoopInputStickX = 0;
    gNdsStageTurnLoopCheckCallCount = 0u;
    gNdsStageTurnLoopCheckSuccessCount = 0u;
    gNdsStageTurnLoopSetStatusCount = 0u;
    gNdsStageTurnLoopMainSetStatusCount = 0u;
    gNdsStageTurnLoopAnimEventsCount = 0u;
    gNdsStageTurnLoopUpdateTickCount = 0u;
    gNdsStageTurnLoopFinalUpdateTickCount = 0u;
    gNdsStageTurnLoopPhysicsTickCount = 0u;
    gNdsStageTurnLoopMapTickCount = 0u;
    gNdsStageTurnLoopWaitSetStatusCount = 0u;
    gNdsStageTurnLoopPlayerTagWaitCount = 0u;
    gNdsStageTurnLoopStatusBefore = 0u;
    gNdsStageTurnLoopMotionBefore = 0;
    gNdsStageTurnLoopGABefore = 0u;
    gNdsStageTurnLoopLRBefore = 0;
    gNdsStageTurnLoopVelBeforeMilli = 0;
    gNdsStageTurnLoopStatusAfterCheck = 0u;
    gNdsStageTurnLoopMotionAfterCheck = 0;
    gNdsStageTurnLoopGAAfterCheck = 0u;
    gNdsStageTurnLoopLRAfterCheck = 0;
    gNdsStageTurnLoopVelAfterCheckMilli = 0;
    gNdsStageTurnLoopAllowAfterSetup = 0u;
    gNdsStageTurnLoopDisableAfterSetup = 0u;
    gNdsStageTurnLoopButtonMaskAfterSetup = 0u;
    gNdsStageTurnLoopLRDashAfterSetup = 0;
    gNdsStageTurnLoopLRTurnAfterSetup = 0;
    gNdsStageTurnLoopAttackBufferAfterSetup = 0;
    gNdsStageTurnLoopFlagBeforeUpdate = 0u;
    gNdsStageTurnLoopStatusAfterUpdate = 0u;
    gNdsStageTurnLoopMotionAfterUpdate = 0;
    gNdsStageTurnLoopGAAfterUpdate = 0u;
    gNdsStageTurnLoopLRAfterUpdate = 0;
    gNdsStageTurnLoopVelAfterUpdateMilli = 0;
    gNdsStageTurnLoopFlagAfterUpdate = 0u;
    gNdsStageTurnLoopAllowAfterUpdate = 0u;
    gNdsStageTurnLoopDisableAfterUpdate = 0u;
    gNdsStageTurnLoopProcCallbacksAfterTurn = 0u;
    gNdsStageTurnLoopStatusAfterPhysicsMap = 0u;
    gNdsStageTurnLoopMotionAfterPhysicsMap = 0;
    gNdsStageTurnLoopGAAfterPhysicsMap = 0u;
    gNdsStageTurnLoopStatusAfterFinal = 0u;
    gNdsStageTurnLoopMotionAfterFinal = 0;
    gNdsStageTurnLoopGAAfterFinal = 0u;
    gNdsStageTurnLoopLRFinal = 0;
    gNdsStageTurnLoopVelFinalMilli = 0;
    gNdsStageTurnLoopPlayerTagWaitAfterFinal = 0;
    gNdsStageTurnLoopProcCallbacksAfterFinal = 0u;
    sNdsStageTurnLoopSetStatusActive = FALSE;
    sNdsStageTurnLoopUpdateActive = FALSE;
    sNdsStageTurnLoopFinalUpdateActive = FALSE;
    sNdsStageTurnLoopPhysicsMapActive = FALSE;
}

void ndsFighterMarioFoxStageTurnLoopPrepare(void)
{
    if ((ndsFighterMarioFoxStageTurnLoopProofEnabled() == FALSE) ||
        (gNdsStageTurnLoopPrepared != 0u))
    {
        return;
    }
    ndsFighterMarioFoxStageTurnLoopReset();
    gNdsStageTurnLoopPrepared = 1u;
    gNdsStageTurnLoopBaseDownWaitSeen =
        (gNdsStageMPDownWaitLoopPrepared != 0u) ? 1u : 0u;
}

static void ndsStageTurnLoopRunProbe(void)
{
    FTStruct *fp = &sNdsFighterStructPool[0];
    GObj *fighter_gobj;
    sb32 turn_result;

    if ((ndsFighterStructIsPoolPointer(fp) == FALSE) ||
        (fp->fighter_gobj == NULL) ||
        (fp->attr == NULL))
    {
        gNdsStageTurnLoopUnsafeCount++;
        return;
    }

    fighter_gobj = fp->fighter_gobj;
    fp->ga = nMPKineticsGround;
    fp->input.pl.button_hold = 0u;
    fp->input.pl.button_tap = 0u;
    fp->input.pl.button_release = 0u;
    fp->input.pl.stick_range.x = 0;
    fp->input.pl.stick_range.y = 0;
    fp->tap_stick_x = FTINPUT_STICKBUFFER_TICS_MAX;
    fp->tap_stick_y = FTINPUT_STICKBUFFER_TICS_MAX;
    ftCommonWaitSetStatus(fighter_gobj);

    fp->lr = 1;
    fp->physics.vel_ground.x = 2.5F;
    fp->physics.vel_ground.y = 0.0F;
    fp->physics.vel_ground.z = 0.0F;
    ndsFighterSyncPhysicsToLegacyVel(fp);
    fp->input.pl.stick_range.x = -80;
    fp->input.pl.stick_range.y = 0;
    fp->input.pl.button_hold = 0u;
    fp->input.pl.button_tap = 0u;
    fp->input.pl.button_release = 0u;
    fp->tap_stick_x = FTINPUT_STICKBUFFER_TICS_MAX;
    fp->tap_stick_y = FTINPUT_STICKBUFFER_TICS_MAX;

    gNdsStageTurnLoopStatusBefore = (u32)fp->status_id;
    gNdsStageTurnLoopMotionBefore = fp->motion_id;
    gNdsStageTurnLoopGABefore = (u32)fp->ga;
    gNdsStageTurnLoopLRBefore = fp->lr;
    gNdsStageTurnLoopVelBeforeMilli =
        ndsFloatToMilliSigned(fp->physics.vel_ground.x);
    gNdsStageTurnLoopInputStickX = fp->input.pl.stick_range.x;

    sNdsStageTurnLoopSetStatusActive = TRUE;
    turn_result = ftCommonTurnCheckInterruptCommon(fighter_gobj);
    sNdsStageTurnLoopSetStatusActive = FALSE;

    gNdsStageTurnLoopStatusAfterCheck = (u32)fp->status_id;
    gNdsStageTurnLoopMotionAfterCheck = fp->motion_id;
    gNdsStageTurnLoopGAAfterCheck = (u32)fp->ga;
    gNdsStageTurnLoopLRAfterCheck = fp->lr;
    gNdsStageTurnLoopVelAfterCheckMilli =
        ndsFloatToMilliSigned(fp->physics.vel_ground.x);
    gNdsStageTurnLoopAllowAfterSetup =
        (fp->status_vars.common.turn.is_allow_turn_direction != FALSE) ?
            1u : 0u;
    gNdsStageTurnLoopDisableAfterSetup =
        (fp->status_vars.common.turn.is_disable_sa_interrupts != FALSE) ?
            1u : 0u;
    gNdsStageTurnLoopButtonMaskAfterSetup =
        (u32)fp->status_vars.common.turn.button_mask;
    gNdsStageTurnLoopLRDashAfterSetup =
        fp->status_vars.common.turn.lr_dash;
    gNdsStageTurnLoopLRTurnAfterSetup =
        fp->status_vars.common.turn.lr_turn;
    gNdsStageTurnLoopAttackBufferAfterSetup =
        fp->status_vars.common.turn.attacks4_buffer;
    gNdsStageTurnLoopProcCallbacksAfterTurn =
        ((fp->proc_update == ftCommonTurnProcUpdate) &&
         (fp->proc_interrupt == ftCommonTurnProcInterrupt) &&
         (fp->proc_physics == ftPhysicsApplyGroundVelFriction) &&
         (fp->proc_map == mpCommonSetFighterFallOnGroundBreak) &&
         (fp->proc_damage == NULL)) ? 1u : 0u;

    if ((turn_result == FALSE) ||
        (gNdsStageTurnLoopProcCallbacksAfterTurn == 0u))
    {
        gNdsStageTurnLoopUnsafeCount++;
        return;
    }

    fp->motion_vars.flags.flag1 = 1;
    fighter_gobj->anim_frame = 1.0F;
    fp->anim_frame = 1.0F;
    gNdsStageTurnLoopFlagBeforeUpdate = (u32)fp->motion_vars.flags.flag1;
    sNdsStageTurnLoopUpdateActive = TRUE;
    fp->proc_update(fighter_gobj);
    sNdsStageTurnLoopUpdateActive = FALSE;
    ndsFighterSyncPhysicsToLegacyVel(fp);

    gNdsStageTurnLoopStatusAfterUpdate = (u32)fp->status_id;
    gNdsStageTurnLoopMotionAfterUpdate = fp->motion_id;
    gNdsStageTurnLoopGAAfterUpdate = (u32)fp->ga;
    gNdsStageTurnLoopLRAfterUpdate = fp->lr;
    gNdsStageTurnLoopVelAfterUpdateMilli =
        ndsFloatToMilliSigned(fp->physics.vel_ground.x);
    gNdsStageTurnLoopFlagAfterUpdate = (u32)fp->motion_vars.flags.flag1;
    gNdsStageTurnLoopAllowAfterUpdate =
        (fp->status_vars.common.turn.is_allow_turn_direction != FALSE) ?
            1u : 0u;
    gNdsStageTurnLoopDisableAfterUpdate =
        (fp->status_vars.common.turn.is_disable_sa_interrupts != FALSE) ?
            1u : 0u;

    if ((fp->status_id != nFTCommonStatusTurn) ||
        (fp->motion_id != nFTCommonMotionTurn) ||
        (fp->ga != nMPKineticsGround))
    {
        gNdsStageTurnLoopUnsafeCount++;
        return;
    }

    sNdsStageTurnLoopPhysicsMapActive = TRUE;
    fp->proc_physics(fighter_gobj);
    fp->proc_map(fighter_gobj);
    sNdsStageTurnLoopPhysicsMapActive = FALSE;
    gNdsStageTurnLoopStatusAfterPhysicsMap = (u32)fp->status_id;
    gNdsStageTurnLoopMotionAfterPhysicsMap = fp->motion_id;
    gNdsStageTurnLoopGAAfterPhysicsMap = (u32)fp->ga;

    fighter_gobj->anim_frame = 0.0F;
    fp->anim_frame = 0.0F;
    sNdsStageTurnLoopFinalUpdateActive = TRUE;
    fp->proc_update(fighter_gobj);
    sNdsStageTurnLoopFinalUpdateActive = FALSE;
    ndsFighterSyncPhysicsToLegacyVel(fp);

    gNdsStageTurnLoopStatusAfterFinal = (u32)fp->status_id;
    gNdsStageTurnLoopMotionAfterFinal = fp->motion_id;
    gNdsStageTurnLoopGAAfterFinal = (u32)fp->ga;
    gNdsStageTurnLoopLRFinal = fp->lr;
    gNdsStageTurnLoopVelFinalMilli =
        ndsFloatToMilliSigned(fp->physics.vel_ground.x);
    gNdsStageTurnLoopPlayerTagWaitAfterFinal = fp->playertag_wait;
    gNdsStageTurnLoopProcCallbacksAfterFinal =
        ((fp->proc_update == NULL) &&
         (fp->proc_interrupt == ftCommonWaitProcInterrupt) &&
         (fp->proc_physics == ftPhysicsApplyGroundVelFriction) &&
         (fp->proc_map == mpCommonProcFighterOnCliffEdge) &&
         (fp->proc_damage == NULL) &&
         (fp->is_special_interrupt != FALSE)) ? 1u : 0u;
}

void ndsFighterMarioFoxStageTurnLoopFinalize(void)
{
    u32 mask = 0u;

    if ((ndsFighterMarioFoxStageTurnLoopProofEnabled() == FALSE) ||
        (gNdsStageTurnLoopPrepared == 0u) ||
        (gNdsFighterMarioFoxStageTurnLoopResult != 0u))
    {
        return;
    }

    if (gNdsFighterMarioFoxStageMPDownWaitLoopResult == 0u)
    {
        ndsFighterMarioFoxStageMPDownWaitLoopFinalize();
    }
    if ((gNdsFighterMarioFoxStageMPDownWaitLoopResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPDOWNWAIT_LOOP_PASS) &&
        (gNdsFighterMarioFoxStageMPDownWaitLoopSafeResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPDOWNWAIT_LOOP_SAFE_PASS))
    {
        gNdsStageTurnLoopBaseDownWaitSeen = 1u;
        mask |= 1u << 0;
    }
    if ((gNdsStageTurnLoopPrepared != 0u) &&
        (gNdsStageTurnLoopBaseDownWaitSeen != 0u))
    {
        mask |= 1u << 1;
    }

    if ((gNdsStageTurnLoopCheckCallCount == 0u) &&
        (gNdsStageTurnLoopUnsafeCount == 0u))
    {
        ndsStageTurnLoopRunProbe();
    }

    if ((gNdsStageTurnLoopStatusBefore == (u32)nFTCommonStatusWait) &&
        (gNdsStageTurnLoopMotionBefore == nFTCommonMotionWait) &&
        (gNdsStageTurnLoopGABefore == (u32)nMPKineticsGround) &&
        (gNdsStageTurnLoopLRBefore == 1) &&
        (gNdsStageTurnLoopVelBeforeMilli > 0) &&
        (gNdsStageTurnLoopInputStickX <= FTCOMMON_TURN_STICK_RANGE_MIN))
    {
        mask |= 1u << 2;
    }
    if ((gNdsStageTurnLoopCheckCallCount == 1u) &&
        (gNdsStageTurnLoopCheckSuccessCount == 1u) &&
        (gNdsStageTurnLoopSetStatusCount == 1u) &&
        (gNdsStageTurnLoopMainSetStatusCount == 1u) &&
        (gNdsStageTurnLoopAnimEventsCount == 1u))
    {
        mask |= 1u << 3;
    }
    if ((gNdsStageTurnLoopStatusAfterCheck ==
            (u32)nFTCommonStatusTurn) &&
        (gNdsStageTurnLoopMotionAfterCheck == nFTCommonMotionTurn) &&
        (gNdsStageTurnLoopGAAfterCheck == (u32)nMPKineticsGround) &&
        (gNdsStageTurnLoopLRAfterCheck == 1) &&
        (gNdsStageTurnLoopVelAfterCheckMilli > 0) &&
        (gNdsStageTurnLoopAllowAfterSetup == 0u) &&
        (gNdsStageTurnLoopDisableAfterSetup == 0u) &&
        (gNdsStageTurnLoopButtonMaskAfterSetup == 0u) &&
        (gNdsStageTurnLoopLRDashAfterSetup == 0) &&
        (gNdsStageTurnLoopLRTurnAfterSetup == -1) &&
        (gNdsStageTurnLoopAttackBufferAfterSetup == 256) &&
        (gNdsStageTurnLoopProcCallbacksAfterTurn == 1u))
    {
        mask |= 1u << 4;
    }
    if ((gNdsStageTurnLoopUpdateTickCount == 1u) &&
        (gNdsStageTurnLoopFlagBeforeUpdate == 1u) &&
        (gNdsStageTurnLoopFlagAfterUpdate == 0u) &&
        (gNdsStageTurnLoopStatusAfterUpdate ==
            (u32)nFTCommonStatusTurn) &&
        (gNdsStageTurnLoopMotionAfterUpdate == nFTCommonMotionTurn) &&
        (gNdsStageTurnLoopGAAfterUpdate == (u32)nMPKineticsGround) &&
        (gNdsStageTurnLoopLRAfterUpdate == -1) &&
        (gNdsStageTurnLoopVelAfterUpdateMilli < 0) &&
        (gNdsStageTurnLoopAllowAfterUpdate == 1u) &&
        (gNdsStageTurnLoopDisableAfterUpdate == 1u))
    {
        mask |= 1u << 5;
    }
    if ((gNdsStageTurnLoopPhysicsTickCount == 1u) &&
        (gNdsStageTurnLoopMapTickCount == 1u) &&
        (gNdsStageTurnLoopStatusAfterPhysicsMap ==
            (u32)nFTCommonStatusTurn) &&
        (gNdsStageTurnLoopMotionAfterPhysicsMap == nFTCommonMotionTurn) &&
        (gNdsStageTurnLoopGAAfterPhysicsMap == (u32)nMPKineticsGround))
    {
        mask |= 1u << 6;
    }
    if ((gNdsStageTurnLoopFinalUpdateTickCount == 1u) &&
        (gNdsStageTurnLoopWaitSetStatusCount == 1u) &&
        (gNdsStageTurnLoopPlayerTagWaitCount == 1u) &&
        (gNdsStageTurnLoopStatusAfterFinal ==
            (u32)nFTCommonStatusWait) &&
        (gNdsStageTurnLoopMotionAfterFinal == nFTCommonMotionWait) &&
        (gNdsStageTurnLoopGAAfterFinal == (u32)nMPKineticsGround) &&
        (gNdsStageTurnLoopLRFinal == -1) &&
        (gNdsStageTurnLoopVelFinalMilli <= 0) &&
        (gNdsStageTurnLoopPlayerTagWaitAfterFinal == 120) &&
        (gNdsStageTurnLoopProcCallbacksAfterFinal == 1u))
    {
        mask |= 1u << 7;
    }
    if ((gNdsStageTurnLoopUnsafeCount == 0u) &&
        (sNdsStageTurnLoopSetStatusActive == FALSE) &&
        (sNdsStageTurnLoopUpdateActive == FALSE) &&
        (sNdsStageTurnLoopFinalUpdateActive == FALSE) &&
        (sNdsStageTurnLoopPhysicsMapActive == FALSE))
    {
        mask |= 1u << 8;
    }

    gNdsFighterMarioFoxStageTurnLoopCount = 1u;
    gNdsFighterMarioFoxStageTurnLoopMask = mask;
    gNdsFighterMarioFoxStageTurnLoopDeferredMask = 0xffu;
    if ((mask & 0x1ffu) == 0x1ffu)
    {
        gNdsFighterMarioFoxStageTurnLoopResult =
            NDS_FIGHTER_MARIOFOX_STAGE_TURN_LOOP_PASS;
        gNdsFighterMarioFoxStageTurnLoopSafeResult =
            NDS_FIGHTER_MARIOFOX_STAGE_TURN_LOOP_SAFE_PASS;
    }
}

static u32 ndsStageMPDownRecoverLoopWaitCallbacksReady(FTStruct *fp)
{
    return ((fp != NULL) &&
            (fp->proc_update == ftCommonDownWaitProcUpdate) &&
            (fp->proc_interrupt == ftCommonDownWaitProcInterrupt) &&
            (fp->proc_physics == ftPhysicsApplyGroundVelFriction) &&
            (fp->proc_map == mpCommonSetFighterFallOnGroundBreak) &&
            (fp->proc_damage == NULL)) ? 1u : 0u;
}

static u32 ndsStageMPDownRecoverLoopWaitFinalCallbacksReady(FTStruct *fp)
{
    return ((fp != NULL) &&
            (fp->proc_update == NULL) &&
            (fp->proc_interrupt == ftCommonWaitProcInterrupt) &&
            (fp->proc_physics == ftPhysicsApplyGroundVelFriction) &&
            (fp->proc_map == mpCommonProcFighterOnCliffEdge) &&
            (fp->proc_damage == NULL) &&
            (fp->is_special_interrupt != FALSE)) ? 1u : 0u;
}

static void ndsFighterMarioFoxStageMPDownRecoverLoopReset(void)
{
    gNdsFighterMarioFoxStageMPDownRecoverLoopResult = 0u;
    gNdsFighterMarioFoxStageMPDownRecoverLoopSafeResult = 0u;
    gNdsFighterMarioFoxStageMPDownRecoverLoopMask = 0u;
    gNdsFighterMarioFoxStageMPDownRecoverLoopDeferredMask = 0u;
    gNdsFighterMarioFoxStageMPDownRecoverLoopCount = 0u;
    gNdsStageMPDownRecoverLoopPrepared = 0u;
    gNdsStageMPDownRecoverLoopBaseTurnSeen = 0u;
    gNdsStageMPDownRecoverLoopUnsafeCount = 0u;
    gNdsStageMPDownRecoverLoopDownWaitSetStatusCount = 0u;
    gNdsStageMPDownRecoverLoopDownWaitMainSetStatusCount = 0u;
    gNdsStageMPDownRecoverLoopDownWaitCaptureImmuneCount = 0u;
    gNdsStageMPDownRecoverLoopDownWaitStatusAfter = 0u;
    gNdsStageMPDownRecoverLoopDownWaitMotionAfter = 0;
    gNdsStageMPDownRecoverLoopDownWaitGAAfter = 0u;
    gNdsStageMPDownRecoverLoopDownWaitStandWaitAfter = 0;
    gNdsStageMPDownRecoverLoopDownWaitCallbacksAfter = 0u;
    gNdsStageMPDownRecoverLoopDownStandSourceOrder = 0u;
    gNdsStageMPDownRecoverLoopDownStandInterruptCount = 0u;
    gNdsStageMPDownRecoverLoopDownStandAttackCheckCount = 0u;
    gNdsStageMPDownRecoverLoopDownStandForwardBackCheckCount = 0u;
    gNdsStageMPDownRecoverLoopDownStandCheckCount = 0u;
    gNdsStageMPDownRecoverLoopDownStandSetStatusCount = 0u;
    gNdsStageMPDownRecoverLoopDownStandMainSetStatusCount = 0u;
    gNdsStageMPDownRecoverLoopDownStandStatusAfter = 0u;
    gNdsStageMPDownRecoverLoopDownStandMotionAfter = 0;
    gNdsStageMPDownRecoverLoopDownStandGAAfter = 0u;
    gNdsStageMPDownRecoverLoopDownStandCallbacksAfter = 0u;
    gNdsStageMPDownRecoverLoopAttackSourceOrder = 0u;
    gNdsStageMPDownRecoverLoopAttackInterruptCount = 0u;
    gNdsStageMPDownRecoverLoopAttackCheckCount = 0u;
    gNdsStageMPDownRecoverLoopAttackSetStatusCount = 0u;
    gNdsStageMPDownRecoverLoopAttackMainSetStatusCount = 0u;
    gNdsStageMPDownRecoverLoopAttackAnimEventsCount = 0u;
    gNdsStageMPDownRecoverLoopAttackStatusAfter = 0u;
    gNdsStageMPDownRecoverLoopAttackMotionAfter = 0;
    gNdsStageMPDownRecoverLoopAttackGAAfter = 0u;
    gNdsStageMPDownRecoverLoopAttackMotionAttackIDAfter = 0;
    gNdsStageMPDownRecoverLoopAttackStatusAttackIDAfter = 0;
    gNdsStageMPDownRecoverLoopAttackStatAttackIDAfter = 0;
    gNdsStageMPDownRecoverLoopAttackCallbacksAfter = 0u;
    gNdsStageMPDownRecoverLoopRollForwardSourceOrder = 0u;
    gNdsStageMPDownRecoverLoopRollBackSourceOrder = 0u;
    gNdsStageMPDownRecoverLoopRollInterruptCount = 0u;
    gNdsStageMPDownRecoverLoopRollAttackCheckCount = 0u;
    gNdsStageMPDownRecoverLoopRollForwardBackCheckCount = 0u;
    gNdsStageMPDownRecoverLoopRollSetStatusCount = 0u;
    gNdsStageMPDownRecoverLoopRollMainSetStatusCount = 0u;
    gNdsStageMPDownRecoverLoopRollAnimEventsCount = 0u;
    gNdsStageMPDownRecoverLoopRollForwardStatusAfter = 0u;
    gNdsStageMPDownRecoverLoopRollForwardMotionAfter = 0;
    gNdsStageMPDownRecoverLoopRollForwardGAAfter = 0u;
    gNdsStageMPDownRecoverLoopRollForwardCallbacksAfter = 0u;
    gNdsStageMPDownRecoverLoopRollBackStatusAfter = 0u;
    gNdsStageMPDownRecoverLoopRollBackMotionAfter = 0;
    gNdsStageMPDownRecoverLoopRollBackGAAfter = 0u;
    gNdsStageMPDownRecoverLoopRollBackCallbacksAfter = 0u;
    gNdsStageMPDownRecoverLoopWaitSetStatusCount = 0u;
    gNdsStageMPDownRecoverLoopPlayerTagWaitCount = 0u;
    gNdsStageMPDownRecoverLoopFinalStatusMask = 0u;
    gNdsStageMPDownRecoverLoopFinalMotionMask = 0u;
    gNdsStageMPDownRecoverLoopFinalCallbackMask = 0u;
    sNdsStageMPDownRecoverLoopDownWaitSetStatusActive = FALSE;
    sNdsStageMPDownRecoverLoopDownWaitInterruptActive = FALSE;
    sNdsStageMPDownRecoverLoopDownStandSetStatusActive = FALSE;
    sNdsStageMPDownRecoverLoopDownAttackSetStatusActive = FALSE;
    sNdsStageMPDownRecoverLoopDownForwardBackSetStatusActive = FALSE;
    sNdsStageMPDownRecoverLoopDownStandProbeActive = FALSE;
    sNdsStageMPDownRecoverLoopAttackProbeActive = FALSE;
    sNdsStageMPDownRecoverLoopRollForwardProbeActive = FALSE;
    sNdsStageMPDownRecoverLoopRollBackProbeActive = FALSE;
    sNdsStageMPDownRecoverLoopDownStandUpdateActive = FALSE;
    sNdsStageMPDownRecoverLoopAttackUpdateActive = FALSE;
    sNdsStageMPDownRecoverLoopRollForwardUpdateActive = FALSE;
    sNdsStageMPDownRecoverLoopRollBackUpdateActive = FALSE;
}

void ndsFighterMarioFoxStageMPDownRecoverLoopPrepare(void)
{
    if ((ndsFighterMarioFoxStageMPDownRecoverLoopProofEnabled() == FALSE) ||
        (gNdsStageMPDownRecoverLoopPrepared != 0u))
    {
        return;
    }
    ndsFighterMarioFoxStageMPDownRecoverLoopReset();
    gNdsStageMPDownRecoverLoopPrepared = 1u;
    gNdsStageMPDownRecoverLoopBaseTurnSeen =
        (gNdsStageTurnLoopPrepared != 0u) ? 1u : 0u;
}

static void ndsStageMPDownRecoverLoopPrimeDownWaitD(FTStruct *fp,
                                                    GObj *fighter_gobj)
{
    DObj *root = DObjGetStruct(fighter_gobj);

    fp->status_id = nFTCommonStatusDownWaitD;
    fp->motion_id = -2;
    fp->motion_script_id = -2;
    fp->ga = nMPKineticsGround;
    fp->lr = 1;
    fp->capture_immune_mask = 0x33u;
    fp->is_special_interrupt = FALSE;
    fp->motion_vars.flags.flag1 = 1;
    fp->status_vars.common.downwait.stand_wait =
        FTCOMMON_DOWNWAIT_STAND_WAIT;
    fp->status_vars.common.downbounce.attack_buffer = 0;
    fp->damage_mul = 0.5F;
    fp->vel_ground.x = 0.0F;
    fp->vel_ground.y = 0.0F;
    fp->physics.vel_ground = fp->vel_ground;
    fp->proc_update = ftCommonDownWaitProcUpdate;
    fp->proc_interrupt = ftCommonDownWaitProcInterrupt;
    fp->proc_physics = ftPhysicsApplyGroundVelFriction;
    fp->proc_map = mpCommonSetFighterFallOnGroundBreak;
    fp->proc_damage = NULL;
    fp->input.pl.button_hold = 0u;
    fp->input.pl.button_tap = 0u;
    fp->input.pl.button_release = 0u;
    fp->input.pl.stick_range.x = 0;
    fp->input.pl.stick_range.y = 0;
    fp->anim_frame = 0.0F;
    if (fighter_gobj != NULL)
    {
        fighter_gobj->anim_frame = 0.0F;
    }
    if (root != NULL)
    {
        root->anim_speed = 1.0F;
    }
    if (fp->input.button_mask_a == 0u)
    {
        fp->input.button_mask_a = A_BUTTON;
    }
    if (fp->input.button_mask_b == 0u)
    {
        fp->input.button_mask_b = B_BUTTON;
    }
    if (fp->input.button_mask_z == 0u)
    {
        fp->input.button_mask_z = Z_TRIG;
    }
}

static void ndsStageMPDownRecoverLoopRunFinalUpdate(GObj *fighter_gobj,
                                                    FTStruct *fp,
                                                    u32 bit)
{
    if ((fighter_gobj == NULL) || (fp == NULL) ||
        (fp->proc_update != ftAnimEndSetWait))
    {
        gNdsStageMPDownRecoverLoopUnsafeCount++;
        return;
    }

    fighter_gobj->anim_frame = 0.0F;
    fp->anim_frame = 0.0F;
    if (bit == 0u)
    {
        sNdsStageMPDownRecoverLoopDownStandUpdateActive = TRUE;
    }
    else if (bit == 1u)
    {
        sNdsStageMPDownRecoverLoopAttackUpdateActive = TRUE;
    }
    else if (bit == 2u)
    {
        sNdsStageMPDownRecoverLoopRollForwardUpdateActive = TRUE;
    }
    else
    {
        sNdsStageMPDownRecoverLoopRollBackUpdateActive = TRUE;
    }
    fp->proc_update(fighter_gobj);
    sNdsStageMPDownRecoverLoopDownStandUpdateActive = FALSE;
    sNdsStageMPDownRecoverLoopAttackUpdateActive = FALSE;
    sNdsStageMPDownRecoverLoopRollForwardUpdateActive = FALSE;
    sNdsStageMPDownRecoverLoopRollBackUpdateActive = FALSE;

    if (fp->status_id == nFTCommonStatusWait)
    {
        gNdsStageMPDownRecoverLoopFinalStatusMask |= 1u << bit;
    }
    if (fp->motion_id == nFTCommonMotionWait)
    {
        gNdsStageMPDownRecoverLoopFinalMotionMask |= 1u << bit;
    }
    if (ndsStageMPDownRecoverLoopWaitFinalCallbacksReady(fp) != 0u)
    {
        gNdsStageMPDownRecoverLoopFinalCallbackMask |= 1u << bit;
    }
}

static void ndsStageMPDownRecoverLoopRunDownWaitSetup(void)
{
    FTStruct *fp = &sNdsFighterStructPool[1];
    GObj *fighter_gobj;

    if ((ndsFighterStructIsPoolPointer(fp) == FALSE) ||
        (fp->fighter_gobj == NULL))
    {
        gNdsStageMPDownRecoverLoopUnsafeCount++;
        return;
    }

    fighter_gobj = fp->fighter_gobj;
    fp->status_id = nFTCommonStatusDownBounceD;
    fp->motion_id = nFTCommonMotionDownBounceD;
    fp->motion_script_id = nFTCommonMotionDownBounceD;
    fp->ga = nMPKineticsGround;
    fp->motion_vars.flags.flag1 = 1;
    fp->capture_immune_mask = 0u;
    fp->damage_mul = 1.0F;
    fp->proc_update = ftCommonDownBounceProcUpdate;
    fp->proc_interrupt = NULL;
    fp->proc_physics = ftPhysicsApplyGroundVelFriction;
    fp->proc_map = mpCommonSetFighterFallOnGroundBreak;
    fp->proc_damage = NULL;

    sNdsStageMPDownRecoverLoopDownWaitSetStatusActive = TRUE;
    ftCommonDownWaitSetStatus(fighter_gobj);
    sNdsStageMPDownRecoverLoopDownWaitSetStatusActive = FALSE;

    gNdsStageMPDownRecoverLoopDownWaitStatusAfter = (u32)fp->status_id;
    gNdsStageMPDownRecoverLoopDownWaitMotionAfter = fp->motion_id;
    gNdsStageMPDownRecoverLoopDownWaitGAAfter = (u32)fp->ga;
    gNdsStageMPDownRecoverLoopDownWaitStandWaitAfter =
        fp->status_vars.common.downwait.stand_wait;
    gNdsStageMPDownRecoverLoopDownWaitCallbacksAfter =
        ndsStageMPDownRecoverLoopWaitCallbacksReady(fp);
}

static void ndsStageMPDownRecoverLoopRunDownStandProbe(void)
{
    FTStruct *fp = &sNdsFighterStructPool[1];
    GObj *fighter_gobj;

    if ((ndsFighterStructIsPoolPointer(fp) == FALSE) ||
        (fp->fighter_gobj == NULL))
    {
        gNdsStageMPDownRecoverLoopUnsafeCount++;
        return;
    }

    fighter_gobj = fp->fighter_gobj;
    ndsStageMPDownRecoverLoopPrimeDownWaitD(fp, fighter_gobj);
    fp->input.pl.stick_range.y = 80;

    gNdsStageMPDownRecoverLoopDownStandInterruptCount++;
    sNdsStageMPDownRecoverLoopDownStandProbeActive = TRUE;
    sNdsStageMPDownRecoverLoopDownWaitInterruptActive = TRUE;
    fp->proc_interrupt(fighter_gobj);
    sNdsStageMPDownRecoverLoopDownWaitInterruptActive = FALSE;
    sNdsStageMPDownRecoverLoopDownStandProbeActive = FALSE;

    gNdsStageMPDownRecoverLoopDownStandStatusAfter = (u32)fp->status_id;
    gNdsStageMPDownRecoverLoopDownStandMotionAfter = fp->motion_id;
    gNdsStageMPDownRecoverLoopDownStandGAAfter = (u32)fp->ga;
    gNdsStageMPDownRecoverLoopDownStandCallbacksAfter =
        ((fp->proc_update == ftAnimEndSetWait) &&
         (fp->proc_interrupt == ftCommonDownStandProcInterrupt) &&
         (fp->proc_physics == ftPhysicsApplyGroundVelFriction) &&
         (fp->proc_map == mpCommonSetFighterFallOnGroundBreak) &&
         (fp->proc_damage == NULL)) ? 1u : 0u;
    if (gNdsStageMPDownRecoverLoopDownStandCallbacksAfter != 0u)
    {
        ndsStageMPDownRecoverLoopRunFinalUpdate(fighter_gobj, fp, 0u);
    }
}

static void ndsStageMPDownRecoverLoopRunAttackProbe(void)
{
    FTStruct *fp = &sNdsFighterStructPool[1];
    GObj *fighter_gobj;

    if ((ndsFighterStructIsPoolPointer(fp) == FALSE) ||
        (fp->fighter_gobj == NULL))
    {
        gNdsStageMPDownRecoverLoopUnsafeCount++;
        return;
    }

    fighter_gobj = fp->fighter_gobj;
    ndsStageMPDownRecoverLoopPrimeDownWaitD(fp, fighter_gobj);
    fp->input.pl.button_tap = fp->input.button_mask_a;

    gNdsStageMPDownRecoverLoopAttackInterruptCount++;
    sNdsStageMPDownRecoverLoopAttackProbeActive = TRUE;
    sNdsStageMPDownRecoverLoopDownWaitInterruptActive = TRUE;
    fp->proc_interrupt(fighter_gobj);
    sNdsStageMPDownRecoverLoopDownWaitInterruptActive = FALSE;
    sNdsStageMPDownRecoverLoopAttackProbeActive = FALSE;

    gNdsStageMPDownRecoverLoopAttackStatusAfter = (u32)fp->status_id;
    gNdsStageMPDownRecoverLoopAttackMotionAfter = fp->motion_id;
    gNdsStageMPDownRecoverLoopAttackGAAfter = (u32)fp->ga;
    gNdsStageMPDownRecoverLoopAttackMotionAttackIDAfter =
        fp->motion_attack_id;
    gNdsStageMPDownRecoverLoopAttackStatusAttackIDAfter =
        fp->status_attack_id;
    gNdsStageMPDownRecoverLoopAttackStatAttackIDAfter = fp->stat_attack_id;
    gNdsStageMPDownRecoverLoopAttackCallbacksAfter =
        ((fp->proc_update == ftAnimEndSetWait) &&
         (fp->proc_interrupt == NULL) &&
         (fp->proc_physics == ftPhysicsApplyGroundVelFriction) &&
         (fp->proc_map == mpCommonSetFighterFallOnEdgeBreak) &&
         (fp->proc_damage == NULL)) ? 1u : 0u;
    if (gNdsStageMPDownRecoverLoopAttackCallbacksAfter != 0u)
    {
        ndsStageMPDownRecoverLoopRunFinalUpdate(fighter_gobj, fp, 1u);
    }
}

static void ndsStageMPDownRecoverLoopRunRollProbe(sb32 is_forward)
{
    FTStruct *fp = &sNdsFighterStructPool[1];
    GObj *fighter_gobj;

    if ((ndsFighterStructIsPoolPointer(fp) == FALSE) ||
        (fp->fighter_gobj == NULL))
    {
        gNdsStageMPDownRecoverLoopUnsafeCount++;
        return;
    }

    fighter_gobj = fp->fighter_gobj;
    ndsStageMPDownRecoverLoopPrimeDownWaitD(fp, fighter_gobj);
    fp->input.pl.stick_range.x = (is_forward != FALSE) ? 80 : -80;

    gNdsStageMPDownRecoverLoopRollInterruptCount++;
    if (is_forward != FALSE)
    {
        sNdsStageMPDownRecoverLoopRollForwardProbeActive = TRUE;
    }
    else
    {
        sNdsStageMPDownRecoverLoopRollBackProbeActive = TRUE;
    }
    sNdsStageMPDownRecoverLoopDownWaitInterruptActive = TRUE;
    fp->proc_interrupt(fighter_gobj);
    sNdsStageMPDownRecoverLoopDownWaitInterruptActive = FALSE;
    sNdsStageMPDownRecoverLoopRollForwardProbeActive = FALSE;
    sNdsStageMPDownRecoverLoopRollBackProbeActive = FALSE;

    if (is_forward != FALSE)
    {
        gNdsStageMPDownRecoverLoopRollForwardStatusAfter =
            (u32)fp->status_id;
        gNdsStageMPDownRecoverLoopRollForwardMotionAfter = fp->motion_id;
        gNdsStageMPDownRecoverLoopRollForwardGAAfter = (u32)fp->ga;
        gNdsStageMPDownRecoverLoopRollForwardCallbacksAfter =
            ((fp->proc_update == ftAnimEndSetWait) &&
             (fp->proc_interrupt == NULL) &&
             (fp->proc_physics == ftPhysicsApplyGroundVelTransN) &&
             (fp->proc_map == mpCommonSetFighterFallOnEdgeBreak) &&
             (fp->proc_damage == NULL)) ? 1u : 0u;
        if (gNdsStageMPDownRecoverLoopRollForwardCallbacksAfter != 0u)
        {
            ndsStageMPDownRecoverLoopRunFinalUpdate(fighter_gobj, fp, 2u);
        }
    }
    else
    {
        gNdsStageMPDownRecoverLoopRollBackStatusAfter =
            (u32)fp->status_id;
        gNdsStageMPDownRecoverLoopRollBackMotionAfter = fp->motion_id;
        gNdsStageMPDownRecoverLoopRollBackGAAfter = (u32)fp->ga;
        gNdsStageMPDownRecoverLoopRollBackCallbacksAfter =
            ((fp->proc_update == ftAnimEndSetWait) &&
             (fp->proc_interrupt == NULL) &&
             (fp->proc_physics == ftPhysicsApplyGroundVelTransN) &&
             (fp->proc_map == mpCommonSetFighterFallOnEdgeBreak) &&
             (fp->proc_damage == NULL)) ? 1u : 0u;
        if (gNdsStageMPDownRecoverLoopRollBackCallbacksAfter != 0u)
        {
            ndsStageMPDownRecoverLoopRunFinalUpdate(fighter_gobj, fp, 3u);
        }
    }
}

void ndsFighterMarioFoxStageMPDownRecoverLoopFinalize(void)
{
    u32 mask = 0u;

    if ((ndsFighterMarioFoxStageMPDownRecoverLoopProofEnabled() == FALSE) ||
        (gNdsStageMPDownRecoverLoopPrepared == 0u) ||
        (gNdsFighterMarioFoxStageMPDownRecoverLoopResult != 0u))
    {
        return;
    }

    if (gNdsFighterMarioFoxStageTurnLoopResult == 0u)
    {
        ndsFighterMarioFoxStageTurnLoopFinalize();
    }
    if ((gNdsFighterMarioFoxStageTurnLoopResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_TURN_LOOP_PASS) &&
        (gNdsFighterMarioFoxStageTurnLoopSafeResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_TURN_LOOP_SAFE_PASS))
    {
        gNdsStageMPDownRecoverLoopBaseTurnSeen = 1u;
        mask |= 1u << 0;
    }

    if ((gNdsStageMPDownRecoverLoopDownWaitSetStatusCount == 0u) &&
        (gNdsStageMPDownRecoverLoopUnsafeCount == 0u))
    {
        ndsStageMPDownRecoverLoopRunDownWaitSetup();
        ndsStageMPDownRecoverLoopRunDownStandProbe();
        ndsStageMPDownRecoverLoopRunAttackProbe();
        ndsStageMPDownRecoverLoopRunRollProbe(TRUE);
        ndsStageMPDownRecoverLoopRunRollProbe(FALSE);
    }

    if ((gNdsStageMPDownRecoverLoopDownWaitSetStatusCount == 1u) &&
        (gNdsStageMPDownRecoverLoopDownWaitMainSetStatusCount == 1u) &&
        (gNdsStageMPDownRecoverLoopDownWaitCaptureImmuneCount >= 1u) &&
        (gNdsStageMPDownRecoverLoopDownWaitStatusAfter ==
            (u32)nFTCommonStatusDownWaitD) &&
        (gNdsStageMPDownRecoverLoopDownWaitMotionAfter == -2) &&
        (gNdsStageMPDownRecoverLoopDownWaitGAAfter ==
            (u32)nMPKineticsGround) &&
        (gNdsStageMPDownRecoverLoopDownWaitStandWaitAfter ==
            FTCOMMON_DOWNWAIT_STAND_WAIT) &&
        (gNdsStageMPDownRecoverLoopDownWaitCallbacksAfter == 1u))
    {
        mask |= 1u << 1;
    }
    if ((gNdsStageMPDownRecoverLoopDownStandInterruptCount == 1u) &&
        (gNdsStageMPDownRecoverLoopDownStandAttackCheckCount == 1u) &&
        (gNdsStageMPDownRecoverLoopDownStandForwardBackCheckCount == 1u) &&
        (gNdsStageMPDownRecoverLoopDownStandCheckCount == 1u) &&
        (gNdsStageMPDownRecoverLoopDownStandSetStatusCount == 1u) &&
        (gNdsStageMPDownRecoverLoopDownStandMainSetStatusCount == 1u) &&
        (gNdsStageMPDownRecoverLoopDownStandSourceOrder == 0x12345u) &&
        (gNdsStageMPDownRecoverLoopDownStandStatusAfter ==
            (u32)nFTCommonStatusDownStandD) &&
        (gNdsStageMPDownRecoverLoopDownStandMotionAfter ==
            nFTCommonMotionDownStandD) &&
        (gNdsStageMPDownRecoverLoopDownStandGAAfter ==
            (u32)nMPKineticsGround) &&
        (gNdsStageMPDownRecoverLoopDownStandCallbacksAfter == 1u))
    {
        mask |= 1u << 2;
    }
    if ((gNdsStageMPDownRecoverLoopAttackInterruptCount == 1u) &&
        (gNdsStageMPDownRecoverLoopAttackCheckCount == 1u) &&
        (gNdsStageMPDownRecoverLoopAttackSetStatusCount == 1u) &&
        (gNdsStageMPDownRecoverLoopAttackMainSetStatusCount == 1u) &&
        (gNdsStageMPDownRecoverLoopAttackAnimEventsCount == 1u) &&
        (gNdsStageMPDownRecoverLoopAttackSourceOrder == 0x1234u) &&
        (gNdsStageMPDownRecoverLoopAttackStatusAfter ==
            (u32)nFTCommonStatusDownAttackD) &&
        (gNdsStageMPDownRecoverLoopAttackMotionAfter ==
            nFTCommonMotionDownAttackD) &&
        (gNdsStageMPDownRecoverLoopAttackGAAfter ==
            (u32)nMPKineticsGround) &&
        (gNdsStageMPDownRecoverLoopAttackMotionAttackIDAfter ==
            nFTMotionAttackIDDownAttackD) &&
        (gNdsStageMPDownRecoverLoopAttackStatusAttackIDAfter ==
            nFTStatusAttackIDDownAttackD) &&
        (gNdsStageMPDownRecoverLoopAttackStatAttackIDAfter ==
            nFTStatusAttackIDDownAttackD) &&
        (gNdsStageMPDownRecoverLoopAttackCallbacksAfter == 1u))
    {
        mask |= 1u << 3;
    }
    if ((gNdsStageMPDownRecoverLoopRollInterruptCount == 2u) &&
        (gNdsStageMPDownRecoverLoopRollAttackCheckCount == 2u) &&
        (gNdsStageMPDownRecoverLoopRollForwardBackCheckCount == 2u) &&
        (gNdsStageMPDownRecoverLoopRollSetStatusCount == 2u) &&
        (gNdsStageMPDownRecoverLoopRollMainSetStatusCount == 2u) &&
        (gNdsStageMPDownRecoverLoopRollAnimEventsCount == 2u) &&
        (gNdsStageMPDownRecoverLoopRollForwardSourceOrder == 0x12345u) &&
        (gNdsStageMPDownRecoverLoopRollBackSourceOrder == 0x12345u))
    {
        mask |= 1u << 4;
    }
    if ((gNdsStageMPDownRecoverLoopRollForwardStatusAfter ==
            (u32)nFTCommonStatusDownForwardD) &&
        (gNdsStageMPDownRecoverLoopRollForwardMotionAfter ==
            nFTCommonMotionDownForwardD) &&
        (gNdsStageMPDownRecoverLoopRollForwardGAAfter ==
            (u32)nMPKineticsGround) &&
        (gNdsStageMPDownRecoverLoopRollForwardCallbacksAfter == 1u) &&
        (gNdsStageMPDownRecoverLoopRollBackStatusAfter ==
            (u32)nFTCommonStatusDownBackD) &&
        (gNdsStageMPDownRecoverLoopRollBackMotionAfter ==
            nFTCommonMotionDownBackD) &&
        (gNdsStageMPDownRecoverLoopRollBackGAAfter ==
            (u32)nMPKineticsGround) &&
        (gNdsStageMPDownRecoverLoopRollBackCallbacksAfter == 1u))
    {
        mask |= 1u << 5;
    }
    if ((gNdsStageMPDownRecoverLoopWaitSetStatusCount == 4u) &&
        (gNdsStageMPDownRecoverLoopPlayerTagWaitCount == 4u) &&
        ((gNdsStageMPDownRecoverLoopFinalStatusMask & 0xfu) == 0xfu) &&
        ((gNdsStageMPDownRecoverLoopFinalMotionMask & 0xfu) == 0xfu) &&
        ((gNdsStageMPDownRecoverLoopFinalCallbackMask & 0xfu) == 0xfu))
    {
        mask |= 1u << 6;
    }
    if ((gNdsStageMPDownRecoverLoopUnsafeCount == 0u) &&
        (sNdsStageMPDownRecoverLoopDownWaitSetStatusActive == FALSE) &&
        (sNdsStageMPDownRecoverLoopDownWaitInterruptActive == FALSE) &&
        (sNdsStageMPDownRecoverLoopDownStandSetStatusActive == FALSE) &&
        (sNdsStageMPDownRecoverLoopDownAttackSetStatusActive == FALSE) &&
        (sNdsStageMPDownRecoverLoopDownForwardBackSetStatusActive == FALSE) &&
        (sNdsStageMPDownRecoverLoopDownStandProbeActive == FALSE) &&
        (sNdsStageMPDownRecoverLoopAttackProbeActive == FALSE) &&
        (sNdsStageMPDownRecoverLoopRollForwardProbeActive == FALSE) &&
        (sNdsStageMPDownRecoverLoopRollBackProbeActive == FALSE) &&
        (sNdsStageMPDownRecoverLoopDownStandUpdateActive == FALSE) &&
        (sNdsStageMPDownRecoverLoopAttackUpdateActive == FALSE) &&
        (sNdsStageMPDownRecoverLoopRollForwardUpdateActive == FALSE) &&
        (sNdsStageMPDownRecoverLoopRollBackUpdateActive == FALSE))
    {
        mask |= 1u << 7;
    }

    gNdsFighterMarioFoxStageMPDownRecoverLoopCount = 1u;
    gNdsFighterMarioFoxStageMPDownRecoverLoopMask = mask;
    gNdsFighterMarioFoxStageMPDownRecoverLoopDeferredMask = 0xffu;
    if ((mask & 0xffu) == 0xffu)
    {
        gNdsFighterMarioFoxStageMPDownRecoverLoopResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPDOWNRECOVER_LOOP_PASS;
        gNdsFighterMarioFoxStageMPDownRecoverLoopSafeResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPDOWNRECOVER_LOOP_SAFE_PASS;
    }
}

static void ndsFighterMarioFoxStageMPCliffLedgeLoopReset(void)
{
    gNdsFighterMarioFoxStageMPCliffLedgeLoopResult = 0u;
    gNdsFighterMarioFoxStageMPCliffLedgeLoopSafeResult = 0u;
    gNdsFighterMarioFoxStageMPCliffLedgeLoopMask = 0u;
    gNdsFighterMarioFoxStageMPCliffLedgeLoopDeferredMask = 0u;
    gNdsFighterMarioFoxStageMPCliffLedgeLoopCount = 0u;
    gNdsStageMPCliffLedgeLoopPrepared = 0u;
    gNdsStageMPCliffLedgeLoopBaseDownRecoverSeen = 0u;
    gNdsStageMPCliffLedgeLoopBaseCliffCatchSeen = 0u;
    gNdsStageMPCliffLedgeLoopBaseCliffClimbSeen = 0u;
    gNdsStageMPCliffLedgeLoopBaseCliffClimbFinishSeen = 0u;
    gNdsStageMPCliffLedgeLoopUnsafeCount = 0u;
    gNdsStageMPCliffLedgeLoopOccupancyBlockCount = 0u;
    gNdsStageMPCliffLedgeLoopOccupancyHolderCliffID = -1;
    gNdsStageMPCliffLedgeLoopOccupancyProbeCliffID = -1;
    gNdsStageMPCliffLedgeLoopDropStatusAfter = 0u;
    gNdsStageMPCliffLedgeLoopDropMotionAfter = 0;
    gNdsStageMPCliffLedgeLoopDropGAAfter = 0u;
    gNdsStageMPCliffLedgeLoopDropCliffIDAfter = -1;
    gNdsStageMPCliffLedgeLoopDropCliffCatchWaitAfter = 0;
    gNdsStageMPCliffLedgeLoopDropIsCliffHoldAfter = 0u;
    gNdsStageMPCliffLedgeLoopDropCallbacksAfter = 0u;
    gNdsStageMPCliffLedgeLoopRecatchStatusAfter = 0u;
    gNdsStageMPCliffLedgeLoopRecatchMotionAfter = 0;
    gNdsStageMPCliffLedgeLoopRecatchGAAfter = 0u;
    gNdsStageMPCliffLedgeLoopRecatchCliffIDAfter = -1;
    gNdsStageMPCliffLedgeLoopRecatchIsCliffHoldAfter = 0u;
    gNdsStageMPCliffLedgeLoopRecatchOccupancyBlockCount = 0u;
    gNdsStageMPCliffLedgeLoopClimbFinishStatusAfter = 0u;
    gNdsStageMPCliffLedgeLoopClimbFinishMotionAfter = 0;
    gNdsStageMPCliffLedgeLoopClimbFinishGAAfter = 0u;
    gNdsStageMPCliffLedgeLoopClimbFinishCliffIDBefore = -1;
    gNdsStageMPCliffLedgeLoopClimbFinishFloorLineAfter = -1;
}

void ndsFighterMarioFoxStageMPCliffLedgeLoopPrepare(void)
{
    if ((ndsFighterMarioFoxStageMPCliffLedgeLoopProofEnabled() == FALSE) ||
        (gNdsStageMPCliffLedgeLoopPrepared != 0u))
    {
        return;
    }
    ndsFighterMarioFoxStageMPCliffLedgeLoopReset();
    gNdsStageMPCliffLedgeLoopPrepared = 1u;
    gNdsStageMPCliffLedgeLoopBaseDownRecoverSeen =
        (gNdsStageMPDownRecoverLoopPrepared != 0u) ? 1u : 0u;
}

void ndsFighterMarioFoxStageMPCliffLedgeLoopFinalize(void)
{
    u32 mask = 0u;

    if ((ndsFighterMarioFoxStageMPCliffLedgeLoopProofEnabled() == FALSE) ||
        (gNdsStageMPCliffLedgeLoopPrepared == 0u) ||
        (gNdsFighterMarioFoxStageMPCliffLedgeLoopResult != 0u))
    {
        return;
    }

    if (gNdsFighterMarioFoxStageMPDownRecoverLoopResult == 0u)
    {
        ndsFighterMarioFoxStageMPDownRecoverLoopFinalize();
    }
    if (gNdsFighterMarioFoxStageMPCliffCatchFloorLoopResult == 0u)
    {
        ndsFighterMarioFoxStageMPCliffCatchFloorLoopFinalize();
    }
    if (gNdsFighterMarioFoxStageMPCliffClimbFloorLoopResult == 0u)
    {
        ndsFighterMarioFoxStageMPCliffClimbFloorLoopFinalize();
    }
    if (gNdsFighterMarioFoxStageMPCliffClimbFinishLoopResult == 0u)
    {
        ndsFighterMarioFoxStageMPCliffClimbFinishLoopFinalize();
    }

    if ((gNdsFighterMarioFoxStageMPDownRecoverLoopResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPDOWNRECOVER_LOOP_PASS) &&
        (gNdsFighterMarioFoxStageMPDownRecoverLoopSafeResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPDOWNRECOVER_LOOP_SAFE_PASS))
    {
        gNdsStageMPCliffLedgeLoopBaseDownRecoverSeen = 1u;
        mask |= 1u << 0;
    }
    if ((gNdsFighterMarioFoxStageMPCliffCatchFloorLoopResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFCATCH_FLOOR_LOOP_PASS) &&
        (gNdsFighterMarioFoxStageMPCliffCatchFloorLoopSafeResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFCATCH_FLOOR_LOOP_SAFE_PASS))
    {
        gNdsStageMPCliffLedgeLoopBaseCliffCatchSeen = 1u;
        mask |= 1u << 1;
    }
    if ((gNdsFighterMarioFoxStageMPCliffClimbFloorLoopResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFCLIMB_FLOOR_LOOP_PASS) &&
        (gNdsFighterMarioFoxStageMPCliffClimbFloorLoopSafeResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFCLIMB_FLOOR_LOOP_SAFE_PASS))
    {
        gNdsStageMPCliffLedgeLoopBaseCliffClimbSeen = 1u;
        mask |= 1u << 2;
    }
    if ((gNdsFighterMarioFoxStageMPCliffClimbFinishLoopResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFCLIMB_FINISH_LOOP_PASS) &&
        (gNdsFighterMarioFoxStageMPCliffClimbFinishLoopSafeResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFCLIMB_FINISH_LOOP_SAFE_PASS))
    {
        gNdsStageMPCliffLedgeLoopBaseCliffClimbFinishSeen = 1u;
        mask |= 1u << 3;
    }

    gNdsStageMPCliffLedgeLoopOccupancyBlockCount =
        gNdsStageMPCliffCatchFloorLoopOccupancyBlockCount;
    gNdsStageMPCliffLedgeLoopOccupancyHolderCliffID =
        gNdsStageMPCliffCatchFloorLoopOccupancyHolderCliffID;
    gNdsStageMPCliffLedgeLoopOccupancyProbeCliffID =
        gNdsStageMPCliffCatchFloorLoopOccupancyProbeCliffID;
    gNdsStageMPCliffLedgeLoopDropStatusAfter =
        gNdsStageMPCliffClimbFloorLoopDropStatusAfter;
    gNdsStageMPCliffLedgeLoopDropMotionAfter =
        gNdsStageMPCliffClimbFloorLoopDropMotionAfter;
    gNdsStageMPCliffLedgeLoopDropGAAfter =
        gNdsStageMPCliffClimbFloorLoopDropGAAfter;
    gNdsStageMPCliffLedgeLoopDropCliffIDAfter =
        gNdsStageMPCliffClimbFloorLoopDropCliffIDAfter;
    gNdsStageMPCliffLedgeLoopDropCliffCatchWaitAfter =
        gNdsStageMPCliffClimbFloorLoopDropCliffCatchWaitAfter;
    gNdsStageMPCliffLedgeLoopDropIsCliffHoldAfter =
        gNdsStageMPCliffClimbFloorLoopDropIsCliffHoldAfter;
    gNdsStageMPCliffLedgeLoopDropCallbacksAfter =
        gNdsStageMPCliffClimbFloorLoopDropProcCallbacksSetAfter;
    gNdsStageMPCliffLedgeLoopRecatchStatusAfter =
        gNdsStageMPCliffClimbFloorLoopRecatchStatusAfter;
    gNdsStageMPCliffLedgeLoopRecatchMotionAfter =
        gNdsStageMPCliffClimbFloorLoopRecatchMotionAfter;
    gNdsStageMPCliffLedgeLoopRecatchGAAfter =
        gNdsStageMPCliffClimbFloorLoopRecatchGAAfter;
    gNdsStageMPCliffLedgeLoopRecatchCliffIDAfter =
        gNdsStageMPCliffClimbFloorLoopRecatchCliffIDAfter;
    gNdsStageMPCliffLedgeLoopRecatchIsCliffHoldAfter =
        gNdsStageMPCliffClimbFloorLoopRecatchIsCliffHoldAfter;
    gNdsStageMPCliffLedgeLoopRecatchOccupancyBlockCount =
        gNdsStageMPCliffClimbFloorLoopRecatchOccupancyBlockCount;
    gNdsStageMPCliffLedgeLoopClimbFinishStatusAfter =
        gNdsStageMPCliffClimbFinishLoopStatusAfterUpdate;
    gNdsStageMPCliffLedgeLoopClimbFinishMotionAfter =
        (s32)gNdsStageMPCliffClimbFinishLoopMotionAfterUpdate;
    gNdsStageMPCliffLedgeLoopClimbFinishGAAfter =
        gNdsStageMPCliffClimbFinishLoopGAAfterUpdate;
    gNdsStageMPCliffLedgeLoopClimbFinishCliffIDBefore =
        gNdsStageMPCliffClimbFinishLoopCliffIDBefore;
    gNdsStageMPCliffLedgeLoopClimbFinishFloorLineAfter =
        gNdsStageMPCliffClimbFinishLoopFloorLineAfterUpdate;

    if ((gNdsStageMPCliffLedgeLoopOccupancyBlockCount == 1u) &&
        (gNdsStageMPCliffLedgeLoopOccupancyHolderCliffID >= 0) &&
        (gNdsStageMPCliffLedgeLoopOccupancyProbeCliffID ==
            gNdsStageMPCliffLedgeLoopOccupancyHolderCliffID))
    {
        mask |= 1u << 4;
    }
    if ((gNdsStageMPCliffLedgeLoopDropStatusAfter ==
            (u32)nFTCommonStatusFall) &&
        (gNdsStageMPCliffLedgeLoopDropMotionAfter ==
            nFTCommonMotionFall) &&
        (gNdsStageMPCliffLedgeLoopDropGAAfter ==
            (u32)nMPKineticsAir) &&
        (gNdsStageMPCliffLedgeLoopDropCliffCatchWaitAfter ==
            FTCOMMON_CLIFF_CATCH_WAIT) &&
        (gNdsStageMPCliffLedgeLoopDropIsCliffHoldAfter == 0u) &&
        (gNdsStageMPCliffLedgeLoopDropCallbacksAfter == 1u))
    {
        mask |= 1u << 5;
    }
    if ((gNdsStageMPCliffLedgeLoopRecatchStatusAfter ==
            (u32)nFTCommonStatusCliffCatch) &&
        (gNdsStageMPCliffLedgeLoopRecatchMotionAfter ==
            nFTCommonMotionCliffCatch) &&
        (gNdsStageMPCliffLedgeLoopRecatchGAAfter ==
            (u32)nMPKineticsAir) &&
        (gNdsStageMPCliffLedgeLoopRecatchIsCliffHoldAfter == 1u) &&
        (gNdsStageMPCliffLedgeLoopRecatchOccupancyBlockCount == 0u))
    {
        mask |= 1u << 6;
    }
    if ((gNdsStageMPCliffLedgeLoopClimbFinishStatusAfter ==
            (u32)nFTCommonStatusWait) &&
        (gNdsStageMPCliffLedgeLoopClimbFinishMotionAfter ==
            nFTCommonMotionWait) &&
        (gNdsStageMPCliffLedgeLoopClimbFinishGAAfter ==
            (u32)nMPKineticsGround))
    {
        mask |= 1u << 7;
    }
    if ((gNdsStageMPCliffLedgeLoopDropCliffIDAfter >= 0) &&
        (gNdsStageMPCliffLedgeLoopDropCliffIDAfter ==
            gNdsStageMPCliffLedgeLoopRecatchCliffIDAfter) &&
        (gNdsStageMPCliffLedgeLoopClimbFinishCliffIDBefore ==
            gNdsStageMPCliffLedgeLoopDropCliffIDAfter) &&
        (gNdsStageMPCliffLedgeLoopClimbFinishFloorLineAfter ==
            gNdsStageMPCliffLedgeLoopDropCliffIDAfter))
    {
        mask |= 1u << 8;
    }
    if ((gNdsStageMPCliffLedgeLoopUnsafeCount == 0u) &&
        (gNdsStageMPCliffCatchFloorLoopUnsafeCount == 0u) &&
        (gNdsStageMPCliffClimbFloorLoopUnsafeCount == 0u) &&
        (gNdsStageMPCliffClimbFinishLoopUnsafeCount == 0u))
    {
        mask |= 1u << 9;
    }

    gNdsFighterMarioFoxStageMPCliffLedgeLoopCount = 1u;
    gNdsFighterMarioFoxStageMPCliffLedgeLoopMask = mask;
    gNdsFighterMarioFoxStageMPCliffLedgeLoopDeferredMask = 0xffu;
    if ((mask & 0x3ffu) == 0x3ffu)
    {
        gNdsFighterMarioFoxStageMPCliffLedgeLoopResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFLEDGE_LOOP_PASS;
        gNdsFighterMarioFoxStageMPCliffLedgeLoopSafeResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFLEDGE_LOOP_SAFE_PASS;
    }
}

static void ndsFighterMarioFoxStageMPCliffLiveLoopReset(void)
{
    gNdsFighterMarioFoxStageMPCliffLiveLoopResult = 0u;
    gNdsFighterMarioFoxStageMPCliffLiveLoopSafeResult = 0u;
    gNdsFighterMarioFoxStageMPCliffLiveLoopMask = 0u;
    gNdsFighterMarioFoxStageMPCliffLiveLoopDeferredMask = 0u;
    gNdsFighterMarioFoxStageMPCliffLiveLoopCount = 0u;
    gNdsStageMPCliffLiveLoopPrepared = 0u;
    gNdsStageMPCliffLiveLoopBaseCliffLedgeSeen = 0u;
    gNdsStageMPCliffLiveLoopProcessAttachCount = 0u;
    gNdsStageMPCliffLiveLoopGObjProcessRunCount = 0u;
    gNdsStageMPCliffLiveLoopCallbackCount = 0u;
    gNdsStageMPCliffLiveLoopUnsafeCount = 0u;
    gNdsStageMPCliffLiveLoopCallbackSourceMask = 0u;
    gNdsStageMPCliffLiveLoopWaitUpdateCount = 0u;
    gNdsStageMPCliffLiveLoopClimbInterruptCount = 0u;
    gNdsStageMPCliffLiveLoopDropInterruptCount = 0u;
    gNdsStageMPCliffLiveLoopQuickUpdateCount = 0u;
    gNdsStageMPCliffLiveLoopQuick1UpdateCount = 0u;
    gNdsStageMPCliffLiveLoopCommon2UpdateCount = 0u;
    gNdsStageMPCliffLiveLoopCommon2PhysicsCount = 0u;
    gNdsStageMPCliffLiveLoopCommon2MapCount = 0u;
    gNdsStageMPCliffLiveLoopFinishUpdateCount = 0u;
    gNdsStageMPCliffLiveLoopStartStatus = 0u;
    gNdsStageMPCliffLiveLoopStartMotion = 0u;
    gNdsStageMPCliffLiveLoopStartGA = 0u;
    gNdsStageMPCliffLiveLoopWaitStatus = 0u;
    gNdsStageMPCliffLiveLoopWaitMotion = 0u;
    gNdsStageMPCliffLiveLoopWaitGA = 0u;
    gNdsStageMPCliffLiveLoopWaitAllowInterrupt = 0u;
    gNdsStageMPCliffLiveLoopWaitFallWait = 0;
    gNdsStageMPCliffLiveLoopCliffID = -1;
    gNdsStageMPCliffLiveLoopClimbStatus = 0u;
    gNdsStageMPCliffLiveLoopClimbMotion = 0u;
    gNdsStageMPCliffLiveLoopClimbGA = 0u;
    gNdsStageMPCliffLiveLoopQueuedStatus = 0u;
    gNdsStageMPCliffLiveLoopQueuedCliffID = -1;
    gNdsStageMPCliffLiveLoopQuick1Status = 0u;
    gNdsStageMPCliffLiveLoopQuick1Motion = 0u;
    gNdsStageMPCliffLiveLoopQuick2Status = 0u;
    gNdsStageMPCliffLiveLoopQuick2Motion = 0u;
    gNdsStageMPCliffLiveLoopQuick2GA = 0u;
    gNdsStageMPCliffLiveLoopQuick2FloorLine = -1;
    gNdsStageMPCliffLiveLoopCommon2Status = 0u;
    gNdsStageMPCliffLiveLoopCommon2Motion = 0u;
    gNdsStageMPCliffLiveLoopCommon2GA = 0u;
    gNdsStageMPCliffLiveLoopFinishStatus = 0u;
    gNdsStageMPCliffLiveLoopFinishMotion = 0u;
    gNdsStageMPCliffLiveLoopFinishGA = 0u;
    gNdsStageMPCliffLiveLoopDropStatus = 0u;
    gNdsStageMPCliffLiveLoopDropMotion = 0u;
    gNdsStageMPCliffLiveLoopDropGA = 0u;
    gNdsStageMPCliffLiveLoopDropCliffCatchWait = 0;
    gNdsStageMPCliffLiveLoopDropIsCliffHold = 0u;
    sNdsStageMPCliffLiveLoopWaitUpdateActive = FALSE;
    sNdsStageMPCliffLiveLoopSetStatusActive = FALSE;
    sNdsStageMPCliffLiveLoopInterruptActive = FALSE;
    sNdsStageMPCliffLiveLoopQuickUpdateActive = FALSE;
    sNdsStageMPCliffLiveLoopQuick1UpdateActive = FALSE;
    sNdsStageMPCliffLiveLoopCommon2UpdateActive = FALSE;
    sNdsStageMPCliffLiveLoopCommon2PhysicsActive = FALSE;
    sNdsStageMPCliffLiveLoopCommon2MapActive = FALSE;
    sNdsStageMPCliffLiveLoopProcess = NULL;
    sNdsStageMPCliffLiveLoopPhase = 0u;
}

void ndsFighterMarioFoxStageMPCliffLiveLoopPrepare(void)
{
    if ((ndsFighterMarioFoxStageMPCliffLiveLoopProofEnabled() == FALSE) ||
        (gNdsStageMPCliffLiveLoopPrepared != 0u))
    {
        return;
    }
    ndsFighterMarioFoxStageMPCliffLiveLoopReset();
    gNdsStageMPCliffLiveLoopPrepared = 1u;
    gNdsStageMPCliffLiveLoopBaseCliffLedgeSeen =
        (gNdsStageMPCliffLedgeLoopPrepared != 0u) ? 1u : 0u;
}

static void ndsStageMPCliffLiveLoopPrimeCliffState(FTStruct *fp,
                                                   GObj *fighter_gobj,
                                                   DObj *root,
                                                   s32 cliff_id,
                                                   sb32 is_catch)
{
    Vec3f edge;

    fp->status_id = is_catch ? nFTCommonStatusCliffCatch :
        nFTCommonStatusCliffWait;
    fp->motion_id = is_catch ? nFTCommonMotionCliffCatch :
        nFTCommonMotionCliffWait;
    fp->motion_script_id = fp->motion_id;
    fp->ga = is_catch ? nMPKineticsAir : nMPKineticsGround;
    fp->lr = -1;
    fp->is_cliff_hold = TRUE;
    fp->is_jostle_ignore = FALSE;
    fp->percent_damage = 0;
    fp->cliffcatch_wait = 0;
    fp->proc_update = is_catch ? ftCommonCliffCatchProcUpdate : NULL;
    fp->proc_interrupt = is_catch ? NULL : ftCommonCliffWaitProcInterrupt;
    fp->proc_physics = ftCommonCliffCommonProcPhysics;
    fp->proc_map = ftCommonCliffCommonProcMap;
    fp->proc_damage = is_catch ? NULL : ftCommonCliffCommonProcDamage;
    fp->coll_data.cliff_id = cliff_id;
    fp->coll_data.floor_line_id = cliff_id;
    fp->coll_data.p_translate = &root->translate.vec.f;
    fp->coll_data.p_lr = &fp->lr;
    fp->coll_data.p_map_coll = &fp->coll_data.map_coll;
    fp->status_vars.common.cliffwait.is_allow_interrupt = TRUE;
    fp->status_vars.common.cliffwait.fall_wait = 120;
    fp->status_vars.common.cliffmotion.status_id =
        nFTCommonCliffKindClimbQuick;
    fp->status_vars.common.cliffmotion.cliff_id = cliff_id;
    fp->input.pl.stick_range.x = 0;
    fp->input.pl.stick_range.y = 0;
    fp->input.pl.button_hold = 0;
    fp->input.pl.button_tap = 0;
    fp->physics.vel_air.x = 0.0F;
    fp->physics.vel_air.y = 0.0F;
    fp->physics.vel_air.z = 0.0F;
    fp->physics.vel_ground.x = 0.0F;
    fp->physics.vel_ground.y = 0.0F;
    fp->physics.vel_ground.z = 0.0F;
    fp->vel_air.x = 0.0F;
    fp->vel_air.y = 0.0F;
    fp->vel_air.z = 0.0F;
    fp->vel_ground.x = 0.0F;
    fp->vel_ground.y = 0.0F;
    fp->vel_ground.z = 0.0F;
    fp->joints[nFTPartsJointTopN] = root;
    if (fp->joints[nFTPartsJointTransN] == NULL)
    {
        fp->joints[nFTPartsJointTransN] = root;
    }
    if (root->scale.vec.f.x == 0.0F)
    {
        root->scale.vec.f.x = 1.0F;
    }
    if (root->scale.vec.f.y == 0.0F)
    {
        root->scale.vec.f.y = 1.0F;
    }
    if (root->scale.vec.f.z == 0.0F)
    {
        root->scale.vec.f.z = 1.0F;
    }
    mpCollisionGetFloorEdgeR(cliff_id, &edge);
    root->translate.vec.f.x = edge.x;
    root->translate.vec.f.y = edge.y;
    root->translate.vec.f.z = edge.z;
    fighter_gobj->anim_frame = 0.0F;
    fp->anim_frame = 0.0F;
    root->anim_speed = 1.0F;
}

static void ndsStageMPCliffLiveLoopRecordStart(FTStruct *fp)
{
    gNdsStageMPCliffLiveLoopStartStatus = (u32)fp->status_id;
    gNdsStageMPCliffLiveLoopStartMotion = (u32)fp->motion_id;
    gNdsStageMPCliffLiveLoopStartGA = (u32)fp->ga;
    gNdsStageMPCliffLiveLoopCliffID = fp->coll_data.cliff_id;
}

static void ndsStageMPCliffLiveLoopGObjProc(GObj *fighter_gobj)
{
    FTStruct *fp;

    if ((fighter_gobj == NULL) ||
        (gNdsStageMPCliffLiveLoopUnsafeCount != 0u) ||
        (sNdsStageMPCliffLiveLoopPhase >= 7u))
    {
        return;
    }

    fp = ftGetStruct(fighter_gobj);
    if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE) ||
        (fp->player != 0))
    {
        gNdsStageMPCliffLiveLoopUnsafeCount++;
        return;
    }
    gNdsStageMPCliffLiveLoopCallbackCount++;
    gNdsStageMPCliffLiveLoopGObjProcessRunCount++;

    switch (sNdsStageMPCliffLiveLoopPhase)
    {
    case 0:
        if (fp->proc_update != ftCommonCliffCatchProcUpdate)
        {
            gNdsStageMPCliffLiveLoopUnsafeCount++;
            return;
        }
        ndsStageMPCliffLiveLoopRecordStart(fp);
        gNdsStageMPCliffLiveLoopWaitUpdateCount++;
        sNdsStageMPCliffLiveLoopWaitUpdateActive = TRUE;
        fp->proc_update(fighter_gobj);
        sNdsStageMPCliffLiveLoopWaitUpdateActive = FALSE;
        gNdsStageMPCliffLiveLoopWaitStatus = (u32)fp->status_id;
        gNdsStageMPCliffLiveLoopWaitMotion = (u32)fp->motion_id;
        gNdsStageMPCliffLiveLoopWaitGA = (u32)fp->ga;
        gNdsStageMPCliffLiveLoopWaitAllowInterrupt =
            (fp->status_vars.common.cliffwait.is_allow_interrupt != FALSE) ?
            1u : 0u;
        gNdsStageMPCliffLiveLoopWaitFallWait =
            fp->status_vars.common.cliffwait.fall_wait;
        sNdsStageMPCliffLiveLoopPhase = 1u;
        break;

    case 1:
        if (fp->proc_interrupt != ftCommonCliffWaitProcInterrupt)
        {
            gNdsStageMPCliffLiveLoopUnsafeCount++;
            return;
        }
        fp->input.pl.stick_range.x = 0;
        fp->input.pl.stick_range.y = 80;
        fp->input.pl.button_hold = 0;
        fp->input.pl.button_tap = 0;
        fp->status_vars.common.cliffwait.is_allow_interrupt = TRUE;
        gNdsStageMPCliffLiveLoopClimbInterruptCount++;
        sNdsStageMPCliffLiveLoopInterruptActive = TRUE;
        sNdsStageMPCliffLiveLoopSetStatusActive = TRUE;
        fp->proc_interrupt(fighter_gobj);
        sNdsStageMPCliffLiveLoopSetStatusActive = FALSE;
        sNdsStageMPCliffLiveLoopInterruptActive = FALSE;
        gNdsStageMPCliffLiveLoopClimbStatus = (u32)fp->status_id;
        gNdsStageMPCliffLiveLoopClimbMotion = (u32)fp->motion_id;
        gNdsStageMPCliffLiveLoopClimbGA = (u32)fp->ga;
        gNdsStageMPCliffLiveLoopQueuedStatus =
            (u32)fp->status_vars.common.cliffmotion.status_id;
        gNdsStageMPCliffLiveLoopQueuedCliffID =
            fp->status_vars.common.cliffmotion.cliff_id;
        sNdsStageMPCliffLiveLoopPhase = 2u;
        break;

    case 2:
        if (fp->proc_update != ftCommonCliffQuickProcUpdate)
        {
            gNdsStageMPCliffLiveLoopUnsafeCount++;
            return;
        }
        fighter_gobj->anim_frame = 0.0F;
        fp->anim_frame = 0.0F;
        gNdsStageMPCliffLiveLoopQuickUpdateCount++;
        sNdsStageMPCliffLiveLoopQuickUpdateActive = TRUE;
        sNdsStageMPCliffLiveLoopSetStatusActive = TRUE;
        fp->proc_update(fighter_gobj);
        sNdsStageMPCliffLiveLoopSetStatusActive = FALSE;
        sNdsStageMPCliffLiveLoopQuickUpdateActive = FALSE;
        gNdsStageMPCliffLiveLoopQuick1Status = (u32)fp->status_id;
        gNdsStageMPCliffLiveLoopQuick1Motion = (u32)fp->motion_id;
        sNdsStageMPCliffLiveLoopPhase = 3u;
        break;

    case 3:
        if (fp->proc_update != ftCommonCliffClimbQuick1ProcUpdate)
        {
            gNdsStageMPCliffLiveLoopUnsafeCount++;
            return;
        }
        fighter_gobj->anim_frame = 0.0F;
        fp->anim_frame = 0.0F;
        gNdsStageMPCliffLiveLoopQuick1UpdateCount++;
        sNdsStageMPCliffLiveLoopQuick1UpdateActive = TRUE;
        fp->proc_update(fighter_gobj);
        sNdsStageMPCliffLiveLoopQuick1UpdateActive = FALSE;
        gNdsStageMPCliffLiveLoopQuick2Status = (u32)fp->status_id;
        gNdsStageMPCliffLiveLoopQuick2Motion = (u32)fp->motion_id;
        gNdsStageMPCliffLiveLoopQuick2GA = (u32)fp->ga;
        gNdsStageMPCliffLiveLoopQuick2FloorLine =
            fp->coll_data.floor_line_id;
        sNdsStageMPCliffLiveLoopPhase = 4u;
        break;

    case 4:
        if ((fp->proc_update != ftCommonCliffCommon2ProcUpdate) ||
            (fp->proc_physics != ftCommonCliffCommon2ProcPhysics) ||
            (fp->proc_map != ftCommonCliffClimbCommon2ProcMap))
        {
            gNdsStageMPCliffLiveLoopUnsafeCount++;
            return;
        }
        fighter_gobj->anim_frame = 1.0F;
        fp->anim_frame = 1.0F;
        gNdsStageMPCliffLiveLoopCommon2UpdateCount++;
        sNdsStageMPCliffLiveLoopCommon2UpdateActive = TRUE;
        fp->proc_update(fighter_gobj);
        sNdsStageMPCliffLiveLoopCommon2UpdateActive = FALSE;
        gNdsStageMPCliffLiveLoopCommon2PhysicsCount++;
        sNdsStageMPCliffLiveLoopCommon2PhysicsActive = TRUE;
        fp->proc_physics(fighter_gobj);
        sNdsStageMPCliffLiveLoopCommon2PhysicsActive = FALSE;
        gNdsStageMPCliffLiveLoopCommon2MapCount++;
        sNdsStageMPCliffLiveLoopCommon2MapActive = TRUE;
        fp->proc_map(fighter_gobj);
        sNdsStageMPCliffLiveLoopCommon2MapActive = FALSE;
        gNdsStageMPCliffLiveLoopCommon2Status = (u32)fp->status_id;
        gNdsStageMPCliffLiveLoopCommon2Motion = (u32)fp->motion_id;
        gNdsStageMPCliffLiveLoopCommon2GA = (u32)fp->ga;
        sNdsStageMPCliffLiveLoopPhase = 5u;
        break;

    case 5:
        if (fp->proc_update != ftCommonCliffCommon2ProcUpdate)
        {
            gNdsStageMPCliffLiveLoopUnsafeCount++;
            return;
        }
        fighter_gobj->anim_frame = 0.0F;
        fp->anim_frame = 0.0F;
        gNdsStageMPCliffLiveLoopFinishUpdateCount++;
        sNdsStageMPCliffLiveLoopCommon2UpdateActive = TRUE;
        fp->proc_update(fighter_gobj);
        sNdsStageMPCliffLiveLoopCommon2UpdateActive = FALSE;
        gNdsStageMPCliffLiveLoopFinishStatus = (u32)fp->status_id;
        gNdsStageMPCliffLiveLoopFinishMotion = (u32)fp->motion_id;
        gNdsStageMPCliffLiveLoopFinishGA = (u32)fp->ga;
        ndsStageMPCliffLiveLoopPrimeCliffState(fp, fighter_gobj,
            DObjGetStruct(fighter_gobj), gNdsStageMPCliffLiveLoopCliffID,
            FALSE);
        sNdsStageMPCliffLiveLoopPhase = 6u;
        break;

    case 6:
        if (fp->proc_interrupt != ftCommonCliffWaitProcInterrupt)
        {
            gNdsStageMPCliffLiveLoopUnsafeCount++;
            return;
        }
        fp->input.pl.stick_range.x = 80;
        fp->input.pl.stick_range.y = 0;
        fp->input.pl.button_hold = 0;
        fp->input.pl.button_tap = 0;
        fp->status_vars.common.cliffwait.is_allow_interrupt = TRUE;
        gNdsStageMPCliffLiveLoopDropInterruptCount++;
        sNdsStageMPCliffLiveLoopInterruptActive = TRUE;
        sNdsStageMPCliffLiveLoopSetStatusActive = TRUE;
        fp->proc_interrupt(fighter_gobj);
        sNdsStageMPCliffLiveLoopSetStatusActive = FALSE;
        sNdsStageMPCliffLiveLoopInterruptActive = FALSE;
        gNdsStageMPCliffLiveLoopDropStatus = (u32)fp->status_id;
        gNdsStageMPCliffLiveLoopDropMotion = (u32)fp->motion_id;
        gNdsStageMPCliffLiveLoopDropGA = (u32)fp->ga;
        gNdsStageMPCliffLiveLoopDropCliffCatchWait =
            fp->cliffcatch_wait;
        gNdsStageMPCliffLiveLoopDropIsCliffHold =
            (fp->is_cliff_hold != FALSE) ? 1u : 0u;
        sNdsStageMPCliffLiveLoopPhase = 7u;
        break;

    default:
        break;
    }
}

static void ndsStageMPCliffLiveLoopRunProof(void)
{
    FTStruct *fp = &sNdsFighterStructPool[0];
    GObj *fighter_gobj;
    DObj *root;
    s32 cliff_id = gNdsStageMPCliffLedgeLoopDropCliffIDAfter;
    u32 i;

    if ((ndsFighterStructIsPoolPointer(fp) == FALSE) ||
        (fp->fighter_gobj == NULL))
    {
        gNdsStageMPCliffLiveLoopUnsafeCount++;
        return;
    }
    if (cliff_id < 0)
    {
        cliff_id = gNdsStageMPCliffCatchFloorLoopCliffIDAfter;
    }
    if (cliff_id < 0)
    {
        gNdsStageMPCliffLiveLoopUnsafeCount++;
        return;
    }

    fighter_gobj = fp->fighter_gobj;
    root = DObjGetStruct(fighter_gobj);
    if (root == NULL)
    {
        gNdsStageMPCliffLiveLoopUnsafeCount++;
        return;
    }

    gcPauseGObjProcessAll(fighter_gobj);
    ndsStageMPCliffLiveLoopPrimeCliffState(fp, fighter_gobj, root,
        cliff_id, TRUE);
    sNdsStageMPCliffLiveLoopPhase = 0u;
    sNdsStageMPCliffLiveLoopProcess = gcAddGObjProcess(fighter_gobj,
        ndsStageMPCliffLiveLoopGObjProc, nGCProcessKindFunc, 2);
    if (sNdsStageMPCliffLiveLoopProcess == NULL)
    {
        gNdsStageMPCliffLiveLoopUnsafeCount++;
        return;
    }
    gNdsStageMPCliffLiveLoopProcessAttachCount++;

    for (i = 0u; (i < 8u) && (sNdsStageMPCliffLiveLoopPhase < 7u); i++)
    {
        gcRunGObjProcess(sNdsStageMPCliffLiveLoopProcess);
    }
    gcPauseGObjProcess(sNdsStageMPCliffLiveLoopProcess);
}

void ndsFighterMarioFoxStageMPCliffLiveLoopFinalize(void)
{
    u32 mask = 0u;

    if ((ndsFighterMarioFoxStageMPCliffLiveLoopProofEnabled() == FALSE) ||
        (gNdsStageMPCliffLiveLoopPrepared == 0u) ||
        (gNdsFighterMarioFoxStageMPCliffLiveLoopResult != 0u))
    {
        return;
    }

    if (gNdsFighterMarioFoxStageMPCliffLedgeLoopResult == 0u)
    {
        ndsFighterMarioFoxStageMPCliffLedgeLoopFinalize();
    }
    if ((gNdsFighterMarioFoxStageMPCliffLedgeLoopResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFLEDGE_LOOP_PASS) &&
        (gNdsFighterMarioFoxStageMPCliffLedgeLoopSafeResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFLEDGE_LOOP_SAFE_PASS))
    {
        gNdsStageMPCliffLiveLoopBaseCliffLedgeSeen = 1u;
        mask |= 1u << 0;
    }

    if ((gNdsStageMPCliffLiveLoopProcessAttachCount == 0u) &&
        (gNdsStageMPCliffLiveLoopUnsafeCount == 0u))
    {
        ndsStageMPCliffLiveLoopRunProof();
    }

    if ((gNdsStageMPCliffLiveLoopPrepared != 0u) &&
        (gNdsStageMPCliffLiveLoopProcessAttachCount == 1u) &&
        (gNdsStageMPCliffLiveLoopGObjProcessRunCount >= 7u) &&
        (gNdsStageMPCliffLiveLoopCallbackCount >= 7u))
    {
        mask |= 1u << 1;
    }
    if ((gNdsStageMPCliffLiveLoopStartStatus ==
            (u32)nFTCommonStatusCliffCatch) &&
        (gNdsStageMPCliffLiveLoopStartMotion ==
            (u32)nFTCommonMotionCliffCatch) &&
        (gNdsStageMPCliffLiveLoopStartGA ==
            (u32)nMPKineticsAir) &&
        (gNdsStageMPCliffLiveLoopWaitUpdateCount == 1u) &&
        (gNdsStageMPCliffLiveLoopWaitStatus ==
            (u32)nFTCommonStatusCliffWait) &&
        (gNdsStageMPCliffLiveLoopWaitMotion ==
            (u32)nFTCommonMotionCliffWait) &&
        (gNdsStageMPCliffLiveLoopWaitGA ==
            (u32)nMPKineticsGround) &&
        (gNdsStageMPCliffLiveLoopWaitAllowInterrupt == 0u) &&
        (gNdsStageMPCliffLiveLoopWaitFallWait > 1) &&
        (gNdsStageMPCliffLiveLoopCliffID >= 0))
    {
        mask |= 1u << 2;
    }
    if ((gNdsStageMPCliffLiveLoopClimbInterruptCount == 1u) &&
        (gNdsStageMPCliffLiveLoopClimbStatus ==
            (u32)nFTCommonStatusCliffQuick) &&
        (gNdsStageMPCliffLiveLoopClimbMotion ==
            (u32)nFTCommonMotionCliffQuick) &&
        (gNdsStageMPCliffLiveLoopClimbGA ==
            (u32)nMPKineticsGround) &&
        (gNdsStageMPCliffLiveLoopQueuedStatus ==
            (u32)nFTCommonCliffKindClimbQuick) &&
        (gNdsStageMPCliffLiveLoopQueuedCliffID ==
            gNdsStageMPCliffLiveLoopCliffID))
    {
        mask |= 1u << 3;
    }
    if ((gNdsStageMPCliffLiveLoopQuickUpdateCount == 1u) &&
        (gNdsStageMPCliffLiveLoopQuick1UpdateCount == 1u) &&
        (gNdsStageMPCliffLiveLoopQuick1Status ==
            (u32)nFTCommonStatusCliffClimbQuick1) &&
        (gNdsStageMPCliffLiveLoopQuick1Motion ==
            (u32)nFTCommonMotionCliffClimbQuick1) &&
        (gNdsStageMPCliffLiveLoopQuick2Status ==
            (u32)nFTCommonStatusCliffClimbQuick2) &&
        (gNdsStageMPCliffLiveLoopQuick2Motion ==
            (u32)nFTCommonMotionCliffClimbQuick2) &&
        (gNdsStageMPCliffLiveLoopQuick2GA ==
            (u32)nMPKineticsGround) &&
        (gNdsStageMPCliffLiveLoopQuick2FloorLine ==
            gNdsStageMPCliffLiveLoopCliffID))
    {
        mask |= 1u << 4;
    }
    if ((gNdsStageMPCliffLiveLoopCommon2UpdateCount == 1u) &&
        (gNdsStageMPCliffLiveLoopCommon2PhysicsCount == 1u) &&
        (gNdsStageMPCliffLiveLoopCommon2MapCount == 1u) &&
        (gNdsStageMPCliffLiveLoopCommon2Status ==
            (u32)nFTCommonStatusCliffClimbQuick2) &&
        (gNdsStageMPCliffLiveLoopCommon2Motion ==
            (u32)nFTCommonMotionCliffClimbQuick2) &&
        (gNdsStageMPCliffLiveLoopCommon2GA ==
            (u32)nMPKineticsGround))
    {
        mask |= 1u << 5;
    }
    if ((gNdsStageMPCliffLiveLoopFinishUpdateCount == 1u) &&
        (gNdsStageMPCliffLiveLoopFinishStatus ==
            (u32)nFTCommonStatusWait) &&
        (gNdsStageMPCliffLiveLoopFinishMotion ==
            (u32)nFTCommonMotionWait) &&
        (gNdsStageMPCliffLiveLoopFinishGA ==
            (u32)nMPKineticsGround))
    {
        mask |= 1u << 6;
    }
    if ((gNdsStageMPCliffLiveLoopDropInterruptCount == 1u) &&
        (gNdsStageMPCliffLiveLoopDropStatus ==
            (u32)nFTCommonStatusFall) &&
        (gNdsStageMPCliffLiveLoopDropMotion ==
            (u32)nFTCommonMotionFall) &&
        (gNdsStageMPCliffLiveLoopDropGA ==
            (u32)nMPKineticsAir) &&
        (gNdsStageMPCliffLiveLoopDropCliffCatchWait ==
            FTCOMMON_CLIFF_CATCH_WAIT) &&
        (gNdsStageMPCliffLiveLoopDropIsCliffHold == 0u))
    {
        mask |= 1u << 7;
    }
    if ((gNdsStageMPCliffLiveLoopCallbackSourceMask & 0xfffu) == 0xfffu)
    {
        mask |= 1u << 8;
    }
    if ((gNdsStageMPCliffLiveLoopUnsafeCount == 0u) &&
        (sNdsStageMPCliffLiveLoopWaitUpdateActive == FALSE) &&
        (sNdsStageMPCliffLiveLoopSetStatusActive == FALSE) &&
        (sNdsStageMPCliffLiveLoopInterruptActive == FALSE) &&
        (sNdsStageMPCliffLiveLoopQuickUpdateActive == FALSE) &&
        (sNdsStageMPCliffLiveLoopQuick1UpdateActive == FALSE) &&
        (sNdsStageMPCliffLiveLoopCommon2UpdateActive == FALSE) &&
        (sNdsStageMPCliffLiveLoopCommon2PhysicsActive == FALSE) &&
        (sNdsStageMPCliffLiveLoopCommon2MapActive == FALSE) &&
        (sNdsStageMPCliffLiveLoopPhase >= 7u))
    {
        mask |= 1u << 9;
    }

    gNdsFighterMarioFoxStageMPCliffLiveLoopCount = 1u;
    gNdsFighterMarioFoxStageMPCliffLiveLoopMask = mask;
    gNdsFighterMarioFoxStageMPCliffLiveLoopDeferredMask = 0xffu;
    if ((mask & 0x3ffu) == 0x3ffu)
    {
        gNdsFighterMarioFoxStageMPCliffLiveLoopResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFLIVE_LOOP_PASS;
        gNdsFighterMarioFoxStageMPCliffLiveLoopSafeResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFLIVE_LOOP_SAFE_PASS;
    }
}

static void ndsFighterMarioFoxStageMPCliffAttackFloorLoopReset(void)
{
    gNdsFighterMarioFoxStageMPCliffAttackFloorLoopResult = 0u;
    gNdsFighterMarioFoxStageMPCliffAttackFloorLoopSafeResult = 0u;
    gNdsFighterMarioFoxStageMPCliffAttackFloorLoopMask = 0u;
    gNdsFighterMarioFoxStageMPCliffAttackFloorLoopDeferredMask = 0u;
    gNdsFighterMarioFoxStageMPCliffAttackFloorLoopCount = 0u;
    gNdsStageMPCliffAttackFloorLoopPrepared = 0u;
    gNdsStageMPCliffAttackFloorLoopBaseMPCliffWaitSeen = 0u;
    gNdsStageMPCliffAttackFloorLoopInterruptCallCount = 0u;
    gNdsStageMPCliffAttackFloorLoopAttackCheckCount = 0u;
    gNdsStageMPCliffAttackFloorLoopEscapeCheckCount = 0u;
    gNdsStageMPCliffAttackFloorLoopClimbOrFallCheckCount = 0u;
    gNdsStageMPCliffAttackFloorLoopQuickStatusSetCount = 0u;
    gNdsStageMPCliffAttackFloorLoopAnimEventsCount = 0u;
    gNdsStageMPCliffAttackFloorLoopUnsafeCount = 0u;
    gNdsStageMPCliffAttackFloorLoopStatusBefore = 0u;
    gNdsStageMPCliffAttackFloorLoopMotionBefore = 0u;
    gNdsStageMPCliffAttackFloorLoopGABefore = 0u;
    gNdsStageMPCliffAttackFloorLoopStatusAfter = 0u;
    gNdsStageMPCliffAttackFloorLoopMotionAfter = 0u;
    gNdsStageMPCliffAttackFloorLoopGAAfter = 0u;
    gNdsStageMPCliffAttackFloorLoopCliffIDBefore = -1;
    gNdsStageMPCliffAttackFloorLoopCliffIDAfter = -1;
    gNdsStageMPCliffAttackFloorLoopQueuedStatusID = 0u;
    gNdsStageMPCliffAttackFloorLoopQueuedCliffID = -1;
    gNdsStageMPCliffAttackFloorLoopIsCliffHoldAfter = 0u;
    gNdsStageMPCliffAttackFloorLoopAllowInterruptBefore = 0u;
    gNdsStageMPCliffAttackFloorLoopAllowInterruptAfter = 0u;
    gNdsStageMPCliffAttackFloorLoopButtonTapMask = 0u;
    gNdsStageMPCliffAttackFloorLoopButtonMaskA = 0u;
    gNdsStageMPCliffAttackFloorLoopButtonMaskB = 0u;
    gNdsStageMPCliffAttackFloorLoopProcDamageSetAfter = 0u;
    gNdsStageMPCliffAttackFloorLoopFallWaitBefore = 0;
    gNdsStageMPCliffAttackFloorLoopFallWaitAfter = 0;
    gNdsStageMPCliffAttackFloorLoopDamageFallCallCount = 0u;
    sNdsStageMPCliffAttackFloorLoopSetStatusActive = FALSE;
    sNdsStageMPCliffAttackFloorLoopInterruptActive = FALSE;
}

void ndsFighterMarioFoxStageMPCliffAttackFloorLoopPrepare(void)
{
    if ((ndsFighterMarioFoxStageMPCliffAttackFloorLoopProofEnabled() ==
            FALSE) ||
        (gNdsStageMPCliffAttackFloorLoopPrepared != 0u))
    {
        return;
    }
    ndsFighterMarioFoxStageMPCliffAttackFloorLoopReset();
    gNdsStageMPCliffAttackFloorLoopPrepared = 1u;
    gNdsStageMPCliffAttackFloorLoopBaseMPCliffWaitSeen =
        (gNdsStageMPCliffWaitFloorLoopPrepared != 0u) ? 1u : 0u;
}

static void ndsStageMPCliffAttackFloorLoopRunProbe(void)
{
    FTStruct *fp = &sNdsFighterStructPool[1];
    GObj *fighter_gobj;

    if ((ndsFighterStructIsPoolPointer(fp) == FALSE) ||
        (fp->fighter_gobj == NULL))
    {
        gNdsStageMPCliffAttackFloorLoopUnsafeCount++;
        return;
    }

    fighter_gobj = fp->fighter_gobj;
#if NDS_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP_HARNESS
    if ((fp->status_id != nFTCommonStatusCliffWait) ||
        (fp->motion_id != nFTCommonMotionCliffWait) ||
        (fp->ga != nMPKineticsGround) ||
        (fp->coll_data.cliff_id < 0))
    {
        DObj *root = fp->joints[nFTPartsJointTopN];
        s32 cliff_id = gNdsStageMPCliffWaitFloorLoopCliffIDAfterUpdate;

        if (cliff_id < 0)
        {
            cliff_id = gNdsStageMPCliffLiveLoopCliffID;
        }
        if (cliff_id < 0)
        {
            cliff_id = gNdsStageMPCliffCatchFloorLoopCliffIDAfter;
        }
        if ((root != NULL) && (cliff_id >= 0))
        {
            /* ponytail: reseed only the delayed aggregate probe. */
            ndsStageMPCliffLiveLoopPrimeCliffState(fp, fighter_gobj, root,
                                                   cliff_id, FALSE);
        }
    }
#endif

    if ((fp->status_id != nFTCommonStatusCliffWait) ||
        (fp->motion_id != nFTCommonMotionCliffWait) ||
        (fp->ga != nMPKineticsGround) ||
        (fp->coll_data.cliff_id < 0))
    {
        gNdsStageMPCliffAttackFloorLoopUnsafeCount++;
        return;
    }

    fp->input.pl.button_hold = 0u;
    fp->input.pl.button_release = 0u;
    fp->input.pl.button_tap = fp->input.button_mask_a;
    fp->input.pl.stick_range.x = 0;
    fp->input.pl.stick_range.y = 0;

    gNdsStageMPCliffAttackFloorLoopStatusBefore = (u32)fp->status_id;
    gNdsStageMPCliffAttackFloorLoopMotionBefore = (u32)fp->motion_id;
    gNdsStageMPCliffAttackFloorLoopGABefore = (u32)fp->ga;
    gNdsStageMPCliffAttackFloorLoopCliffIDBefore = fp->coll_data.cliff_id;
    gNdsStageMPCliffAttackFloorLoopAllowInterruptBefore =
        (fp->status_vars.common.cliffwait.is_allow_interrupt != FALSE) ?
        1u : 0u;
    gNdsStageMPCliffAttackFloorLoopButtonTapMask =
        (u32)fp->input.pl.button_tap;
    gNdsStageMPCliffAttackFloorLoopButtonMaskA =
        (u32)fp->input.button_mask_a;
    gNdsStageMPCliffAttackFloorLoopButtonMaskB =
        (u32)fp->input.button_mask_b;
    gNdsStageMPCliffAttackFloorLoopFallWaitBefore =
        fp->status_vars.common.cliffwait.fall_wait;

    gNdsStageMPCliffAttackFloorLoopInterruptCallCount++;
    sNdsStageMPCliffAttackFloorLoopInterruptActive = TRUE;
    sNdsStageMPCliffAttackFloorLoopSetStatusActive = TRUE;
    ftCommonCliffWaitProcInterrupt(fighter_gobj);
    sNdsStageMPCliffAttackFloorLoopSetStatusActive = FALSE;
    sNdsStageMPCliffAttackFloorLoopInterruptActive = FALSE;

    gNdsStageMPCliffAttackFloorLoopStatusAfter = (u32)fp->status_id;
    gNdsStageMPCliffAttackFloorLoopMotionAfter = (u32)fp->motion_id;
    gNdsStageMPCliffAttackFloorLoopGAAfter = (u32)fp->ga;
    gNdsStageMPCliffAttackFloorLoopCliffIDAfter = fp->coll_data.cliff_id;
    gNdsStageMPCliffAttackFloorLoopQueuedStatusID =
        (u32)fp->status_vars.common.cliffmotion.status_id;
    gNdsStageMPCliffAttackFloorLoopQueuedCliffID =
        fp->status_vars.common.cliffmotion.cliff_id;
    gNdsStageMPCliffAttackFloorLoopIsCliffHoldAfter =
        (fp->is_cliff_hold != FALSE) ? 1u : 0u;
    gNdsStageMPCliffAttackFloorLoopAllowInterruptAfter =
        (fp->status_vars.common.cliffwait.is_allow_interrupt != FALSE) ?
        1u : 0u;
    gNdsStageMPCliffAttackFloorLoopProcDamageSetAfter =
        (fp->proc_damage != NULL) ? 1u : 0u;
    gNdsStageMPCliffAttackFloorLoopFallWaitAfter =
        fp->status_vars.common.cliffwait.fall_wait;
}

void ndsFighterMarioFoxStageMPCliffAttackFloorLoopFinalize(void)
{
    u32 mask = 0u;

    if ((ndsFighterMarioFoxStageMPCliffAttackFloorLoopProofEnabled() ==
            FALSE) ||
        (gNdsStageMPCliffAttackFloorLoopPrepared == 0u) ||
        (gNdsFighterMarioFoxStageMPCliffAttackFloorLoopResult != 0u))
    {
        return;
    }

    if (gNdsFighterMarioFoxStageMPCliffWaitFloorLoopResult == 0u)
    {
        ndsFighterMarioFoxStageMPCliffWaitFloorLoopFinalize();
    }
    if ((gNdsFighterMarioFoxStageMPCliffWaitFloorLoopResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFWAIT_FLOOR_LOOP_PASS) &&
        (gNdsFighterMarioFoxStageMPCliffWaitFloorLoopSafeResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFWAIT_FLOOR_LOOP_SAFE_PASS))
    {
        gNdsStageMPCliffAttackFloorLoopBaseMPCliffWaitSeen = 1u;
        mask |= 1u << 0;
    }
    if ((gNdsStageMPCliffAttackFloorLoopPrepared != 0u) &&
        (gNdsStageMPCliffAttackFloorLoopBaseMPCliffWaitSeen != 0u))
    {
        mask |= 1u << 1;
    }

    if ((gNdsStageMPCliffAttackFloorLoopInterruptCallCount == 0u) &&
        (gNdsStageMPCliffAttackFloorLoopUnsafeCount == 0u))
    {
        ndsStageMPCliffAttackFloorLoopRunProbe();
    }

    if ((gNdsStageMPCliffAttackFloorLoopStatusBefore ==
            (u32)nFTCommonStatusCliffWait) &&
        (gNdsStageMPCliffAttackFloorLoopMotionBefore ==
            (u32)nFTCommonMotionCliffWait) &&
        (gNdsStageMPCliffAttackFloorLoopGABefore ==
            (u32)nMPKineticsGround) &&
        (gNdsStageMPCliffAttackFloorLoopCliffIDBefore >= 0))
    {
        mask |= 1u << 2;
    }
    if ((gNdsStageMPCliffAttackFloorLoopButtonMaskA != 0u) &&
        (gNdsStageMPCliffAttackFloorLoopButtonMaskB != 0u) &&
        (gNdsStageMPCliffAttackFloorLoopButtonTapMask ==
            gNdsStageMPCliffAttackFloorLoopButtonMaskA))
    {
        mask |= 1u << 3;
    }
    if ((gNdsStageMPCliffAttackFloorLoopInterruptCallCount == 1u) &&
        (gNdsStageMPCliffAttackFloorLoopAttackCheckCount == 1u) &&
        (gNdsStageMPCliffAttackFloorLoopEscapeCheckCount == 0u) &&
        (gNdsStageMPCliffAttackFloorLoopClimbOrFallCheckCount == 0u))
    {
        mask |= 1u << 4;
    }
    if ((gNdsStageMPCliffAttackFloorLoopQuickStatusSetCount == 1u) &&
        (gNdsStageMPCliffAttackFloorLoopAnimEventsCount == 1u))
    {
        mask |= 1u << 5;
    }
    if ((gNdsStageMPCliffAttackFloorLoopStatusAfter ==
            (u32)nFTCommonStatusCliffQuick) &&
        (gNdsStageMPCliffAttackFloorLoopMotionAfter ==
            (u32)nFTCommonMotionCliffQuick) &&
        (gNdsStageMPCliffAttackFloorLoopGAAfter ==
            (u32)nMPKineticsGround))
    {
        mask |= 1u << 6;
    }
    if ((gNdsStageMPCliffAttackFloorLoopQueuedStatusID ==
            (u32)nFTCommonCliffKindAttackQuick) &&
        (gNdsStageMPCliffAttackFloorLoopQueuedCliffID ==
            gNdsStageMPCliffAttackFloorLoopCliffIDBefore) &&
        (gNdsStageMPCliffAttackFloorLoopCliffIDAfter ==
            gNdsStageMPCliffAttackFloorLoopCliffIDBefore))
    {
        mask |= 1u << 7;
    }
    if ((gNdsStageMPCliffAttackFloorLoopIsCliffHoldAfter == 1u) &&
        (gNdsStageMPCliffAttackFloorLoopProcDamageSetAfter == 1u) &&
        (gNdsStageMPCliffAttackFloorLoopAllowInterruptAfter ==
            gNdsStageMPCliffAttackFloorLoopQueuedStatusID))
    {
        mask |= 1u << 8;
    }
    if ((gNdsStageMPCliffAttackFloorLoopFallWaitBefore > 0) &&
        (gNdsStageMPCliffAttackFloorLoopFallWaitAfter ==
            gNdsStageMPCliffAttackFloorLoopQueuedCliffID) &&
        (gNdsStageMPCliffAttackFloorLoopDamageFallCallCount == 0u))
    {
        mask |= 1u << 9;
    }
    if ((gNdsStageMPCliffAttackFloorLoopUnsafeCount == 0u) &&
        (sNdsStageMPCliffAttackFloorLoopSetStatusActive == FALSE) &&
        (sNdsStageMPCliffAttackFloorLoopInterruptActive == FALSE))
    {
        mask |= 1u << 10;
    }

    gNdsFighterMarioFoxStageMPCliffAttackFloorLoopCount = 1u;
    gNdsFighterMarioFoxStageMPCliffAttackFloorLoopMask = mask;
    gNdsFighterMarioFoxStageMPCliffAttackFloorLoopDeferredMask = 0xffu;
    if ((mask & 0x7ffu) == 0x7ffu)
    {
        gNdsFighterMarioFoxStageMPCliffAttackFloorLoopResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFATTACK_FLOOR_LOOP_PASS;
        gNdsFighterMarioFoxStageMPCliffAttackFloorLoopSafeResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFATTACK_FLOOR_LOOP_SAFE_PASS;
    }
}

static void ndsFighterMarioFoxStageMPCliffClimbFloorLoopReset(void)
{
    gNdsFighterMarioFoxStageMPCliffClimbFloorLoopResult = 0u;
    gNdsFighterMarioFoxStageMPCliffClimbFloorLoopSafeResult = 0u;
    gNdsFighterMarioFoxStageMPCliffClimbFloorLoopMask = 0u;
    gNdsFighterMarioFoxStageMPCliffClimbFloorLoopDeferredMask = 0u;
    gNdsFighterMarioFoxStageMPCliffClimbFloorLoopCount = 0u;
    gNdsStageMPCliffClimbFloorLoopPrepared = 0u;
    gNdsStageMPCliffClimbFloorLoopBaseMPCliffWaitSeen = 0u;
    gNdsStageMPCliffClimbFloorLoopInterruptCallCount = 0u;
    gNdsStageMPCliffClimbFloorLoopAttackCheckCount = 0u;
    gNdsStageMPCliffClimbFloorLoopEscapeCheckCount = 0u;
    gNdsStageMPCliffClimbFloorLoopClimbOrFallCheckCount = 0u;
    gNdsStageMPCliffClimbFloorLoopQuickStatusSetCount = 0u;
    gNdsStageMPCliffClimbFloorLoopFallStatusSetCount = 0u;
    gNdsStageMPCliffClimbFloorLoopAnimEventsCount = 0u;
    gNdsStageMPCliffClimbFloorLoopDamageFallCallCount = 0u;
    gNdsStageMPCliffClimbFloorLoopUnsafeCount = 0u;
    gNdsStageMPCliffClimbFloorLoopStatusBefore = 0u;
    gNdsStageMPCliffClimbFloorLoopMotionBefore = 0u;
    gNdsStageMPCliffClimbFloorLoopGABefore = 0u;
    gNdsStageMPCliffClimbFloorLoopCliffIDBefore = -1;
    gNdsStageMPCliffClimbFloorLoopLRBefore = 0;
    gNdsStageMPCliffClimbFloorLoopFallWaitBefore = 0;
    gNdsStageMPCliffClimbFloorLoopAllowInterruptBefore = 0u;
    gNdsStageMPCliffClimbFloorLoopClimbStickX = 0;
    gNdsStageMPCliffClimbFloorLoopClimbStickY = 0;
    gNdsStageMPCliffClimbFloorLoopClimbStatusAfter = 0u;
    gNdsStageMPCliffClimbFloorLoopClimbMotionAfter = 0u;
    gNdsStageMPCliffClimbFloorLoopClimbGAAfter = 0u;
    gNdsStageMPCliffClimbFloorLoopClimbCliffIDAfter = -1;
    gNdsStageMPCliffClimbFloorLoopQueuedStatusID = 0u;
    gNdsStageMPCliffClimbFloorLoopQueuedCliffID = -1;
    gNdsStageMPCliffClimbFloorLoopClimbIsCliffHoldAfter = 0u;
    gNdsStageMPCliffClimbFloorLoopClimbProcDamageSetAfter = 0u;
    gNdsStageMPCliffClimbFloorLoopClimbAllowInterruptAfter = 0u;
    gNdsStageMPCliffClimbFloorLoopClimbFallWaitAfter = 0;
    gNdsStageMPCliffClimbFloorLoopDropStickX = 0;
    gNdsStageMPCliffClimbFloorLoopDropStickY = 0;
    gNdsStageMPCliffClimbFloorLoopDropStatusAfter = 0u;
    gNdsStageMPCliffClimbFloorLoopDropMotionAfter = 0u;
    gNdsStageMPCliffClimbFloorLoopDropGAAfter = 0u;
    gNdsStageMPCliffClimbFloorLoopDropCliffIDAfter = -1;
    gNdsStageMPCliffClimbFloorLoopDropFallWaitAfter = 0;
    gNdsStageMPCliffClimbFloorLoopDropCliffCatchWaitAfter = 0;
    gNdsStageMPCliffClimbFloorLoopDropIsCliffHoldAfter = 0u;
    gNdsStageMPCliffClimbFloorLoopDropProcDamageSetAfter = 0u;
    gNdsStageMPCliffClimbFloorLoopDropProcCallbacksSetAfter = 0u;
    gNdsStageMPCliffClimbFloorLoopRecatchProbeCount = 0u;
    gNdsStageMPCliffClimbFloorLoopRecatchMapCallbackCount = 0u;
    gNdsStageMPCliffClimbFloorLoopRecatchCheckCeilHeavyCliffCount = 0u;
    gNdsStageMPCliffClimbFloorLoopRecatchSpecialCollisionCount = 0u;
    gNdsStageMPCliffClimbFloorLoopRecatchLCliffTestCount = 0u;
    gNdsStageMPCliffClimbFloorLoopRecatchRCliffTestCount = 0u;
    gNdsStageMPCliffClimbFloorLoopRecatchLCliffHitCount = 0u;
    gNdsStageMPCliffClimbFloorLoopRecatchRCliffHitCount = 0u;
    gNdsStageMPCliffClimbFloorLoopRecatchLandingParamCount = 0u;
    gNdsStageMPCliffClimbFloorLoopRecatchCliffCatchSetStatusCount = 0u;
    gNdsStageMPCliffClimbFloorLoopRecatchFtMainSetStatusCount = 0u;
    gNdsStageMPCliffClimbFloorLoopRecatchOccupancyBlockCount = 0u;
    gNdsStageMPCliffClimbFloorLoopRecatchHolderIsCliffHoldBefore = 0u;
    gNdsStageMPCliffClimbFloorLoopRecatchHolderStatusBefore = 0u;
    gNdsStageMPCliffClimbFloorLoopRecatchHolderMotionBefore = 0u;
    gNdsStageMPCliffClimbFloorLoopRecatchHolderGABefore = 0u;
    gNdsStageMPCliffClimbFloorLoopRecatchHolderCliffIDBefore = -1;
    gNdsStageMPCliffClimbFloorLoopRecatchStatusBefore = 0u;
    gNdsStageMPCliffClimbFloorLoopRecatchMotionBefore = 0u;
    gNdsStageMPCliffClimbFloorLoopRecatchGABefore = 0u;
    gNdsStageMPCliffClimbFloorLoopRecatchStatusAfter = 0u;
    gNdsStageMPCliffClimbFloorLoopRecatchMotionAfter = 0u;
    gNdsStageMPCliffClimbFloorLoopRecatchGAAfter = 0u;
    gNdsStageMPCliffClimbFloorLoopRecatchIsCliffHoldAfter = 0u;
    gNdsStageMPCliffClimbFloorLoopRecatchCliffIDAfter = -1;
    gNdsStageMPCliffClimbFloorLoopRecatchLRBefore = 0;
    gNdsStageMPCliffClimbFloorLoopRecatchMaskCurr = 0u;
    gNdsStageMPCliffClimbFloorLoopRecatchMaskStat = 0u;
    sNdsStageMPCliffClimbFloorLoopSetStatusActive = FALSE;
    sNdsStageMPCliffClimbFloorLoopInterruptActive = FALSE;
    sNdsStageMPCliffClimbFloorLoopRecatchMapActive = FALSE;
    sNdsStageMPCliffClimbFloorLoopRecatchSetStatusActive = FALSE;
}

void ndsFighterMarioFoxStageMPCliffClimbFloorLoopPrepare(void)
{
    if ((ndsFighterMarioFoxStageMPCliffClimbFloorLoopProofEnabled() ==
            FALSE) ||
        (gNdsStageMPCliffClimbFloorLoopPrepared != 0u))
    {
        return;
    }
    ndsFighterMarioFoxStageMPCliffClimbFloorLoopReset();
    gNdsStageMPCliffClimbFloorLoopPrepared = 1u;
    gNdsStageMPCliffClimbFloorLoopBaseMPCliffWaitSeen =
        (gNdsStageMPCliffWaitFloorLoopPrepared != 0u) ? 1u : 0u;
}

static void ndsStageMPCliffClimbFloorLoopSeedWaitState(FTStruct *fp)
{
    fp->status_id = nFTCommonStatusCliffWait;
    fp->motion_id = nFTCommonMotionCliffWait;
    fp->motion_script_id = nFTCommonMotionCliffWait;
    fp->ga = nMPKineticsGround;
    fp->lr = -1;
    fp->is_cliff_hold = TRUE;
    fp->percent_damage = 0;
    fp->proc_update = NULL;
    fp->proc_interrupt = ftCommonCliffWaitProcInterrupt;
    fp->proc_physics = ftCommonCliffCommonProcPhysics;
    fp->proc_map = ftCommonCliffCommonProcMap;
    fp->proc_damage = ftCommonCliffCommonProcDamage;
    fp->coll_data.cliff_id =
        gNdsStageMPCliffWaitFloorLoopCliffIDAfterUpdate;
    fp->status_vars.common.cliffwait.is_allow_interrupt = TRUE;
    fp->status_vars.common.cliffwait.fall_wait =
        gNdsStageMPCliffWaitFloorLoopFallWaitAfterInterrupt;
    if (fp->status_vars.common.cliffwait.fall_wait <= 1)
    {
        fp->status_vars.common.cliffwait.fall_wait =
            gNdsStageMPCliffWaitFloorLoopFallWaitAfterUpdate;
    }
    fp->input.pl.button_hold = 0u;
    fp->input.pl.button_tap = 0u;
    fp->input.pl.button_release = 0u;
}

static void ndsStageMPCliffClimbFloorLoopRunProbe(void)
{
    FTStruct *fp = &sNdsFighterStructPool[1];
    GObj *fighter_gobj;
    DObj *root;
    FTStruct wait_state;
    Vec3f root_translate;

    if ((ndsFighterStructIsPoolPointer(fp) == FALSE) ||
        (fp->fighter_gobj == NULL) ||
        (gNdsStageMPCliffWaitFloorLoopCliffIDAfterUpdate < 0))
    {
        gNdsStageMPCliffClimbFloorLoopUnsafeCount++;
        return;
    }

    fighter_gobj = fp->fighter_gobj;
    root = DObjGetStruct(fighter_gobj);
    if (root == NULL)
    {
        gNdsStageMPCliffClimbFloorLoopUnsafeCount++;
        return;
    }
    if (fp->joints[nFTPartsJointTransN] == NULL)
    {
        fp->joints[nFTPartsJointTransN] = root;
    }

    ndsStageMPCliffClimbFloorLoopSeedWaitState(fp);
    if ((fp->status_vars.common.cliffwait.fall_wait <= 1) ||
        (fp->coll_data.cliff_id < 0))
    {
        gNdsStageMPCliffClimbFloorLoopUnsafeCount++;
        return;
    }

    wait_state = *fp;
    root_translate = root->translate.vec.f;
    gNdsStageMPCliffClimbFloorLoopStatusBefore = (u32)fp->status_id;
    gNdsStageMPCliffClimbFloorLoopMotionBefore = (u32)fp->motion_id;
    gNdsStageMPCliffClimbFloorLoopGABefore = (u32)fp->ga;
    gNdsStageMPCliffClimbFloorLoopCliffIDBefore = fp->coll_data.cliff_id;
    gNdsStageMPCliffClimbFloorLoopLRBefore = fp->lr;
    gNdsStageMPCliffClimbFloorLoopFallWaitBefore =
        fp->status_vars.common.cliffwait.fall_wait;
    gNdsStageMPCliffClimbFloorLoopAllowInterruptBefore =
        (fp->status_vars.common.cliffwait.is_allow_interrupt != FALSE) ?
        1u : 0u;

    fp->input.pl.stick_range.x = 0;
    fp->input.pl.stick_range.y = 80;
    gNdsStageMPCliffClimbFloorLoopClimbStickX =
        fp->input.pl.stick_range.x;
    gNdsStageMPCliffClimbFloorLoopClimbStickY =
        fp->input.pl.stick_range.y;
    gNdsStageMPCliffClimbFloorLoopInterruptCallCount++;
    sNdsStageMPCliffClimbFloorLoopInterruptActive = TRUE;
    sNdsStageMPCliffClimbFloorLoopSetStatusActive = TRUE;
    ftCommonCliffWaitProcInterrupt(fighter_gobj);
    sNdsStageMPCliffClimbFloorLoopSetStatusActive = FALSE;
    sNdsStageMPCliffClimbFloorLoopInterruptActive = FALSE;

    gNdsStageMPCliffClimbFloorLoopClimbStatusAfter = (u32)fp->status_id;
    gNdsStageMPCliffClimbFloorLoopClimbMotionAfter = (u32)fp->motion_id;
    gNdsStageMPCliffClimbFloorLoopClimbGAAfter = (u32)fp->ga;
    gNdsStageMPCliffClimbFloorLoopClimbCliffIDAfter =
        fp->coll_data.cliff_id;
    gNdsStageMPCliffClimbFloorLoopQueuedStatusID =
        (u32)fp->status_vars.common.cliffmotion.status_id;
    gNdsStageMPCliffClimbFloorLoopQueuedCliffID =
        fp->status_vars.common.cliffmotion.cliff_id;
    gNdsStageMPCliffClimbFloorLoopClimbIsCliffHoldAfter =
        (fp->is_cliff_hold != FALSE) ? 1u : 0u;
    gNdsStageMPCliffClimbFloorLoopClimbProcDamageSetAfter =
        (fp->proc_damage == ftCommonCliffCommonProcDamage) ? 1u : 0u;
    gNdsStageMPCliffClimbFloorLoopClimbAllowInterruptAfter =
        (fp->status_vars.common.cliffwait.is_allow_interrupt != FALSE) ?
        1u : 0u;
    gNdsStageMPCliffClimbFloorLoopClimbFallWaitAfter =
        fp->status_vars.common.cliffwait.fall_wait;

    *fp = wait_state;
    fp->fighter_gobj = fighter_gobj;
    root->translate.vec.f = root_translate;
    fp->coll_data.p_translate = &root->translate.vec.f;
    fp->coll_data.p_lr = &fp->lr;
    fp->coll_data.p_map_coll = &fp->coll_data.map_coll;
    fp->input.pl.stick_range.x = 80;
    fp->input.pl.stick_range.y = 0;
    gNdsStageMPCliffClimbFloorLoopDropStickX = fp->input.pl.stick_range.x;
    gNdsStageMPCliffClimbFloorLoopDropStickY = fp->input.pl.stick_range.y;

    gNdsStageMPCliffClimbFloorLoopInterruptCallCount++;
    sNdsStageMPCliffClimbFloorLoopInterruptActive = TRUE;
    sNdsStageMPCliffClimbFloorLoopSetStatusActive = TRUE;
    ftCommonCliffWaitProcInterrupt(fighter_gobj);
    sNdsStageMPCliffClimbFloorLoopSetStatusActive = FALSE;
    sNdsStageMPCliffClimbFloorLoopInterruptActive = FALSE;

    gNdsStageMPCliffClimbFloorLoopDropStatusAfter = (u32)fp->status_id;
    gNdsStageMPCliffClimbFloorLoopDropMotionAfter = (u32)fp->motion_id;
    gNdsStageMPCliffClimbFloorLoopDropGAAfter = (u32)fp->ga;
    gNdsStageMPCliffClimbFloorLoopDropCliffIDAfter = fp->coll_data.cliff_id;
    gNdsStageMPCliffClimbFloorLoopDropFallWaitAfter =
        fp->status_vars.common.cliffwait.fall_wait;
    gNdsStageMPCliffClimbFloorLoopDropCliffCatchWaitAfter =
        fp->cliffcatch_wait;
    gNdsStageMPCliffClimbFloorLoopDropIsCliffHoldAfter =
        (fp->is_cliff_hold != FALSE) ? 1u : 0u;
    gNdsStageMPCliffClimbFloorLoopDropProcDamageSetAfter =
        (fp->proc_damage != NULL) ? 1u : 0u;
    if ((fp->proc_interrupt == ftCommonFallProcInterrupt) &&
        (fp->proc_physics == ftPhysicsApplyAirVelDriftFastFall) &&
        (fp->proc_map == mpCommonProcFighterCliffFloorCeil) &&
        (fp->proc_damage == NULL))
    {
        gNdsStageMPCliffClimbFloorLoopDropProcCallbacksSetAfter = 1u;
    }
}

static void ndsStageMPCliffClimbFloorLoopRunRecatchProbe(void)
{
    FTStruct *holder_fp = &sNdsFighterStructPool[1];
    FTStruct *fp = &sNdsFighterStructPool[0];
    GObj *fighter_gobj;
    DObj *root;
    Vec3f left;
    Vec3f right;
    u32 flags = 0u;
    s32 line_id = gNdsStageMPCliffClimbFloorLoopDropCliffIDAfter;
    f32 floor_y = 0.0F;
    f32 sample_x;
    Vec2f cliffcatch;

    if ((ndsFighterStructIsPoolPointer(holder_fp) == FALSE) ||
        (ndsFighterStructIsPoolPointer(fp) == FALSE) ||
        (holder_fp->fighter_gobj == NULL) ||
        (fp->fighter_gobj == NULL) ||
        (line_id < 0) ||
        (gNdsStageMPCliffClimbFloorLoopDropIsCliffHoldAfter != 0u) ||
        (ndsMPFindLineEndpoints(line_id, &left, &right, &flags, NULL) ==
            FALSE))
    {
        gNdsStageMPCliffClimbFloorLoopUnsafeCount++;
        return;
    }

    fighter_gobj = fp->fighter_gobj;
    root = DObjGetStruct(fighter_gobj);
    if (root == NULL)
    {
        gNdsStageMPCliffClimbFloorLoopUnsafeCount++;
        return;
    }
    if (fp->joints[nFTPartsJointTopN] == NULL)
    {
        fp->joints[nFTPartsJointTopN] = root;
    }
    if (fp->joints[nFTPartsJointTransN] == NULL)
    {
        fp->joints[nFTPartsJointTransN] = root;
    }

    sample_x = right.x - 200.0F;
    if (ndsStageFloorEdgeLoopFloorYAtX(line_id, sample_x, &floor_y) ==
        FALSE)
    {
        gNdsStageMPCliffClimbFloorLoopUnsafeCount++;
        return;
    }

    cliffcatch = fp->coll_data.cliffcatch_coll;
    if (cliffcatch.x <= 0.0F)
    {
        cliffcatch.x = 64.0F;
    }
    if (cliffcatch.y == 0.0F)
    {
        cliffcatch.y = 64.0F;
    }

    gNdsStageMPCliffClimbFloorLoopRecatchHolderIsCliffHoldBefore =
        (holder_fp->is_cliff_hold != FALSE) ? 1u : 0u;
    gNdsStageMPCliffClimbFloorLoopRecatchHolderStatusBefore =
        (u32)holder_fp->status_id;
    gNdsStageMPCliffClimbFloorLoopRecatchHolderMotionBefore =
        (u32)holder_fp->motion_id;
    gNdsStageMPCliffClimbFloorLoopRecatchHolderGABefore =
        (u32)holder_fp->ga;
    gNdsStageMPCliffClimbFloorLoopRecatchHolderCliffIDBefore =
        holder_fp->coll_data.cliff_id;

    fp->is_cliff_hold = FALSE;
    fp->status_id = nFTCommonStatusFall;
    fp->motion_id = nFTCommonMotionFall;
    fp->motion_script_id = nFTCommonMotionFall;
    fp->ga = nMPKineticsAir;
    fp->lr = -1;
    fp->cliffcatch_wait = 0;
    fp->proc_map = mpCommonProcFighterCliffFloorCeil;
    fp->physics.vel_air.x = 0.0F;
    fp->physics.vel_air.y = -8.0F;
    fp->physics.vel_air.z = 0.0F;
    fp->physics.vel_ground.x = 0.0F;
    fp->vel_air = fp->physics.vel_air;
    fp->vel_ground = fp->physics.vel_ground;
    fp->coll_data.p_translate = &root->translate.vec.f;
    fp->coll_data.p_lr = &fp->lr;
    fp->coll_data.p_map_coll = &fp->coll_data.map_coll;
    fp->coll_data.cliffcatch_coll = cliffcatch;
    fp->coll_data.pos_prev.x = sample_x + cliffcatch.x;
    fp->coll_data.pos_prev.y = floor_y - cliffcatch.y + 8.0F;
    fp->coll_data.pos_prev.z = 0.0F;
    root->translate.vec.f.x = sample_x + cliffcatch.x;
    root->translate.vec.f.y = floor_y - cliffcatch.y - 8.0F;
    root->translate.vec.f.z = 0.0F;
    fp->coll_data.update_tic = (u16)(gMPCollisionUpdateTic - 1u);
    fp->coll_data.mask_curr = 0u;
    fp->coll_data.mask_stat = 0u;
    fp->coll_data.floor_line_id = -1;
    fp->coll_data.floor_flags = flags;
    fp->coll_data.cliff_id = -1;
    fp->coll_data.ignore_line_id = -1;
    fp->coll_data.is_coll_end = FALSE;

    gNdsStageMPCliffClimbFloorLoopRecatchProbeCount++;
    gNdsStageMPCliffClimbFloorLoopRecatchStatusBefore = (u32)fp->status_id;
    gNdsStageMPCliffClimbFloorLoopRecatchMotionBefore = (u32)fp->motion_id;
    gNdsStageMPCliffClimbFloorLoopRecatchGABefore = (u32)fp->ga;
    gNdsStageMPCliffClimbFloorLoopRecatchLRBefore = fp->lr;

    sNdsStageMPCliffClimbFloorLoopRecatchMapActive = TRUE;
    fp->proc_map(fighter_gobj);
    sNdsStageMPCliffClimbFloorLoopRecatchMapActive = FALSE;

    gNdsStageMPCliffClimbFloorLoopRecatchStatusAfter = (u32)fp->status_id;
    gNdsStageMPCliffClimbFloorLoopRecatchMotionAfter = (u32)fp->motion_id;
    gNdsStageMPCliffClimbFloorLoopRecatchGAAfter = (u32)fp->ga;
    gNdsStageMPCliffClimbFloorLoopRecatchIsCliffHoldAfter =
        (fp->is_cliff_hold != FALSE) ? 1u : 0u;
    gNdsStageMPCliffClimbFloorLoopRecatchCliffIDAfter =
        fp->coll_data.cliff_id;
    gNdsStageMPCliffClimbFloorLoopRecatchMaskCurr =
        fp->coll_data.mask_curr;
    gNdsStageMPCliffClimbFloorLoopRecatchMaskStat =
        fp->coll_data.mask_stat;
}

void ndsFighterMarioFoxStageMPCliffClimbFloorLoopFinalize(void)
{
    u32 mask = 0u;

    if ((ndsFighterMarioFoxStageMPCliffClimbFloorLoopProofEnabled() ==
            FALSE) ||
        (gNdsStageMPCliffClimbFloorLoopPrepared == 0u) ||
        (gNdsFighterMarioFoxStageMPCliffClimbFloorLoopResult != 0u))
    {
        return;
    }

    if (gNdsFighterMarioFoxStageMPCliffWaitFloorLoopResult == 0u)
    {
        ndsFighterMarioFoxStageMPCliffWaitFloorLoopFinalize();
    }
    if ((gNdsFighterMarioFoxStageMPCliffWaitFloorLoopResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFWAIT_FLOOR_LOOP_PASS) &&
        (gNdsFighterMarioFoxStageMPCliffWaitFloorLoopSafeResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFWAIT_FLOOR_LOOP_SAFE_PASS))
    {
        gNdsStageMPCliffClimbFloorLoopBaseMPCliffWaitSeen = 1u;
        mask |= 1u << 0;
    }
    if ((gNdsStageMPCliffClimbFloorLoopPrepared != 0u) &&
        (gNdsStageMPCliffClimbFloorLoopBaseMPCliffWaitSeen != 0u))
    {
        mask |= 1u << 1;
    }

    if ((gNdsStageMPCliffClimbFloorLoopInterruptCallCount == 0u) &&
        (gNdsStageMPCliffClimbFloorLoopUnsafeCount == 0u))
    {
        ndsStageMPCliffClimbFloorLoopRunProbe();
    }
    if ((gNdsStageMPCliffClimbFloorLoopRecatchProbeCount == 0u) &&
        (gNdsStageMPCliffClimbFloorLoopUnsafeCount == 0u) &&
        (gNdsStageMPCliffClimbFloorLoopDropIsCliffHoldAfter == 0u) &&
        (gNdsStageMPCliffClimbFloorLoopDropCliffIDAfter >= 0))
    {
        ndsStageMPCliffClimbFloorLoopRunRecatchProbe();
    }

    if ((gNdsStageMPCliffClimbFloorLoopStatusBefore ==
            (u32)nFTCommonStatusCliffWait) &&
        (gNdsStageMPCliffClimbFloorLoopMotionBefore ==
            (u32)nFTCommonMotionCliffWait) &&
        (gNdsStageMPCliffClimbFloorLoopGABefore ==
            (u32)nMPKineticsGround) &&
        (gNdsStageMPCliffClimbFloorLoopCliffIDBefore >= 0) &&
        (gNdsStageMPCliffClimbFloorLoopLRBefore == -1) &&
        (gNdsStageMPCliffClimbFloorLoopFallWaitBefore > 1) &&
        (gNdsStageMPCliffClimbFloorLoopAllowInterruptBefore == 1u))
    {
        mask |= 1u << 2;
    }
    if ((gNdsStageMPCliffClimbFloorLoopInterruptCallCount == 2u) &&
        (gNdsStageMPCliffClimbFloorLoopAttackCheckCount == 2u) &&
        (gNdsStageMPCliffClimbFloorLoopEscapeCheckCount == 2u) &&
        (gNdsStageMPCliffClimbFloorLoopClimbOrFallCheckCount == 2u))
    {
        mask |= 1u << 3;
    }
    if ((gNdsStageMPCliffClimbFloorLoopClimbStickY >
            FTCOMMON_CLIFF_MOTION_STICK_RANGE_MIN) &&
        (gNdsStageMPCliffClimbFloorLoopClimbStatusAfter ==
            (u32)nFTCommonStatusCliffQuick) &&
        (gNdsStageMPCliffClimbFloorLoopClimbMotionAfter ==
            (u32)nFTCommonMotionCliffQuick) &&
        (gNdsStageMPCliffClimbFloorLoopClimbGAAfter ==
            (u32)nMPKineticsGround))
    {
        mask |= 1u << 4;
    }
    if ((gNdsStageMPCliffClimbFloorLoopQuickStatusSetCount == 1u) &&
        (gNdsStageMPCliffClimbFloorLoopAnimEventsCount == 1u) &&
        (gNdsStageMPCliffClimbFloorLoopQueuedStatusID ==
            (u32)nFTCommonCliffKindClimbQuick) &&
        (gNdsStageMPCliffClimbFloorLoopQueuedCliffID ==
            gNdsStageMPCliffClimbFloorLoopCliffIDBefore) &&
        (gNdsStageMPCliffClimbFloorLoopClimbCliffIDAfter ==
            gNdsStageMPCliffClimbFloorLoopCliffIDBefore))
    {
        mask |= 1u << 5;
    }
    if ((gNdsStageMPCliffClimbFloorLoopClimbIsCliffHoldAfter == 1u) &&
        (gNdsStageMPCliffClimbFloorLoopClimbProcDamageSetAfter == 1u) &&
        (gNdsStageMPCliffClimbFloorLoopClimbAllowInterruptAfter ==
            gNdsStageMPCliffClimbFloorLoopQueuedStatusID) &&
        (gNdsStageMPCliffClimbFloorLoopClimbFallWaitAfter ==
            gNdsStageMPCliffClimbFloorLoopQueuedCliffID))
    {
        mask |= 1u << 6;
    }
    if ((gNdsStageMPCliffClimbFloorLoopDropStickX > 0) &&
        (gNdsStageMPCliffClimbFloorLoopDropStatusAfter ==
            (u32)nFTCommonStatusFall) &&
        (gNdsStageMPCliffClimbFloorLoopDropMotionAfter ==
            (u32)nFTCommonMotionFall) &&
        (gNdsStageMPCliffClimbFloorLoopDropGAAfter ==
            (u32)nMPKineticsAir))
    {
        mask |= 1u << 7;
    }
    if ((gNdsStageMPCliffClimbFloorLoopFallStatusSetCount == 1u) &&
        (gNdsStageMPCliffClimbFloorLoopDropCliffCatchWaitAfter ==
            FTCOMMON_CLIFF_CATCH_WAIT) &&
        (gNdsStageMPCliffClimbFloorLoopDropCliffIDAfter ==
            gNdsStageMPCliffClimbFloorLoopCliffIDBefore) &&
        (gNdsStageMPCliffClimbFloorLoopDropFallWaitAfter ==
            gNdsStageMPCliffClimbFloorLoopFallWaitBefore) &&
        (gNdsStageMPCliffClimbFloorLoopDamageFallCallCount == 0u))
    {
        mask |= 1u << 8;
    }
    if ((gNdsStageMPCliffClimbFloorLoopDropProcCallbacksSetAfter == 1u) &&
        (gNdsStageMPCliffClimbFloorLoopDropIsCliffHoldAfter == 0u) &&
        (gNdsStageMPCliffClimbFloorLoopDropProcDamageSetAfter == 0u))
    {
        mask |= 1u << 9;
    }
    if ((gNdsStageMPCliffClimbFloorLoopUnsafeCount == 0u) &&
        (sNdsStageMPCliffClimbFloorLoopSetStatusActive == FALSE) &&
        (sNdsStageMPCliffClimbFloorLoopInterruptActive == FALSE) &&
        (sNdsStageMPCliffClimbFloorLoopRecatchMapActive == FALSE) &&
        (sNdsStageMPCliffClimbFloorLoopRecatchSetStatusActive == FALSE))
    {
        mask |= 1u << 10;
    }
    if ((gNdsStageMPCliffClimbFloorLoopRecatchProbeCount == 1u) &&
        (gNdsStageMPCliffClimbFloorLoopRecatchHolderIsCliffHoldBefore ==
            0u) &&
        (gNdsStageMPCliffClimbFloorLoopRecatchHolderStatusBefore ==
            (u32)nFTCommonStatusFall) &&
        (gNdsStageMPCliffClimbFloorLoopRecatchHolderMotionBefore ==
            (u32)nFTCommonMotionFall) &&
        (gNdsStageMPCliffClimbFloorLoopRecatchHolderGABefore ==
            (u32)nMPKineticsAir) &&
        (gNdsStageMPCliffClimbFloorLoopRecatchHolderCliffIDBefore ==
            gNdsStageMPCliffClimbFloorLoopDropCliffIDAfter) &&
        (gNdsStageMPCliffClimbFloorLoopRecatchLRBefore == -1))
    {
        mask |= 1u << 11;
    }
    if ((gNdsStageMPCliffClimbFloorLoopRecatchMapCallbackCount == 1u) &&
        (gNdsStageMPCliffClimbFloorLoopRecatchCheckCeilHeavyCliffCount ==
            1u) &&
        (gNdsStageMPCliffClimbFloorLoopRecatchSpecialCollisionCount >= 1u) &&
        (gNdsStageMPCliffClimbFloorLoopRecatchLCliffTestCount == 1u) &&
        (gNdsStageMPCliffClimbFloorLoopRecatchLCliffHitCount == 0u) &&
        (gNdsStageMPCliffClimbFloorLoopRecatchRCliffTestCount == 1u) &&
        (gNdsStageMPCliffClimbFloorLoopRecatchRCliffHitCount == 1u) &&
        (gNdsStageMPCliffClimbFloorLoopRecatchLandingParamCount == 1u) &&
        (gNdsStageMPCliffClimbFloorLoopRecatchOccupancyBlockCount == 0u))
    {
        mask |= 1u << 12;
    }
    if ((gNdsStageMPCliffClimbFloorLoopRecatchCliffCatchSetStatusCount ==
            1u) &&
        (gNdsStageMPCliffClimbFloorLoopRecatchFtMainSetStatusCount == 1u))
    {
        mask |= 1u << 13;
    }
    if ((gNdsStageMPCliffClimbFloorLoopRecatchStatusBefore ==
            (u32)nFTCommonStatusFall) &&
        (gNdsStageMPCliffClimbFloorLoopRecatchMotionBefore ==
            (u32)nFTCommonMotionFall) &&
        (gNdsStageMPCliffClimbFloorLoopRecatchGABefore ==
            (u32)nMPKineticsAir) &&
        (gNdsStageMPCliffClimbFloorLoopRecatchStatusAfter ==
            (u32)nFTCommonStatusCliffCatch) &&
        (gNdsStageMPCliffClimbFloorLoopRecatchMotionAfter ==
            (u32)nFTCommonMotionCliffCatch) &&
        (gNdsStageMPCliffClimbFloorLoopRecatchGAAfter ==
            (u32)nMPKineticsAir) &&
        (gNdsStageMPCliffClimbFloorLoopRecatchIsCliffHoldAfter == 1u) &&
        (gNdsStageMPCliffClimbFloorLoopRecatchCliffIDAfter ==
            gNdsStageMPCliffClimbFloorLoopDropCliffIDAfter) &&
        ((gNdsStageMPCliffClimbFloorLoopRecatchMaskCurr &
            MAP_FLAG_RCLIFF) != 0u) &&
        ((gNdsStageMPCliffClimbFloorLoopRecatchMaskStat &
            MAP_FLAG_RCLIFF) != 0u))
    {
        mask |= 1u << 14;
    }

    gNdsFighterMarioFoxStageMPCliffClimbFloorLoopCount = 1u;
    gNdsFighterMarioFoxStageMPCliffClimbFloorLoopMask = mask;
    gNdsFighterMarioFoxStageMPCliffClimbFloorLoopDeferredMask = 0xffu;
    if ((mask & 0x7fffu) == 0x7fffu)
    {
        gNdsFighterMarioFoxStageMPCliffClimbFloorLoopResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFCLIMB_FLOOR_LOOP_PASS;
        gNdsFighterMarioFoxStageMPCliffClimbFloorLoopSafeResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFCLIMB_FLOOR_LOOP_SAFE_PASS;
    }
}

static void ndsFighterMarioFoxStageMPCliffClimbActionLoopReset(void)
{
    ndsStageMPCliffCommon2BridgeResetDiagnostics();
    gNdsFighterMarioFoxStageMPCliffClimbActionLoopResult = 0u;
    gNdsFighterMarioFoxStageMPCliffClimbActionLoopSafeResult = 0u;
    gNdsFighterMarioFoxStageMPCliffClimbActionLoopMask = 0u;
    gNdsFighterMarioFoxStageMPCliffClimbActionLoopDeferredMask = 0u;
    gNdsFighterMarioFoxStageMPCliffClimbActionLoopCount = 0u;
    gNdsStageMPCliffClimbActionLoopPrepared = 0u;
    gNdsStageMPCliffClimbActionLoopBaseMPCliffClimbSeen = 0u;
    gNdsStageMPCliffClimbActionLoopQuickUpdateCallCount = 0u;
    gNdsStageMPCliffClimbActionLoopQuick1SetStatusCount = 0u;
    gNdsStageMPCliffClimbActionLoopQuick1UpdateCallCount = 0u;
    gNdsStageMPCliffClimbActionLoopAnimEndCheckCount = 0u;
    gNdsStageMPCliffClimbActionLoopQuick2SetStatusCount = 0u;
    gNdsStageMPCliffClimbActionLoopCommon2UpdateCollCount = 0u;
    gNdsStageMPCliffClimbActionLoopCommon2InitVarsCount = 0u;
    gNdsStageMPCliffClimbActionLoopUnsafeCount = 0u;
    gNdsStageMPCliffClimbActionLoopStatusBefore = 0u;
    gNdsStageMPCliffClimbActionLoopMotionBefore = 0u;
    gNdsStageMPCliffClimbActionLoopGABefore = 0u;
    gNdsStageMPCliffClimbActionLoopStatusAfterQuick1 = 0u;
    gNdsStageMPCliffClimbActionLoopMotionAfterQuick1 = 0u;
    gNdsStageMPCliffClimbActionLoopGAAfterQuick1 = 0u;
    gNdsStageMPCliffClimbActionLoopStatusAfterQuick2 = 0u;
    gNdsStageMPCliffClimbActionLoopMotionAfterQuick2 = 0u;
    gNdsStageMPCliffClimbActionLoopGAAfterQuick2 = 0u;
    gNdsStageMPCliffClimbActionLoopCliffIDBefore = -1;
    gNdsStageMPCliffClimbActionLoopCliffIDAfterQuick1 = -1;
    gNdsStageMPCliffClimbActionLoopCliffIDAfterQuick2 = -1;
    gNdsStageMPCliffClimbActionLoopFloorLineAfterQuick2 = -1;
    gNdsStageMPCliffClimbActionLoopQueuedStatusID = 0u;
    gNdsStageMPCliffClimbActionLoopQueuedCliffID = -1;
    gNdsStageMPCliffClimbActionLoopIsCliffHoldAfterQuick1 = 0u;
    gNdsStageMPCliffClimbActionLoopIsCliffHoldAfterQuick2 = 0u;
    gNdsStageMPCliffClimbActionLoopProcDamageSetAfterQuick1 = 0u;
    gNdsStageMPCliffClimbActionLoopProcDamageSetAfterQuick2 = 0u;
    gNdsStageMPCliffClimbActionLoopProcUpdateSetAfterQuick1 = 0u;
    gNdsStageMPCliffClimbActionLoopProcMapSetAfterQuick2 = 0u;
    gNdsStageMPCliffClimbActionLoopJostleIgnoreAfterQuick2 = 0u;
    sNdsStageMPCliffClimbActionLoopQuickUpdateActive = FALSE;
    sNdsStageMPCliffClimbActionLoopSetStatusActive = FALSE;
    sNdsStageMPCliffClimbActionLoopAnimEndActive = FALSE;
}

void ndsFighterMarioFoxStageMPCliffClimbActionLoopPrepare(void)
{
    if ((ndsFighterMarioFoxStageMPCliffClimbActionLoopProofEnabled() ==
            FALSE) ||
        (gNdsStageMPCliffClimbActionLoopPrepared != 0u))
    {
        return;
    }
    ndsFighterMarioFoxStageMPCliffClimbActionLoopReset();
    gNdsStageMPCliffClimbActionLoopPrepared = 1u;
    gNdsStageMPCliffClimbActionLoopBaseMPCliffClimbSeen =
        (gNdsStageMPCliffClimbFloorLoopPrepared != 0u) ? 1u : 0u;
}

static void ndsStageMPCliffClimbActionLoopRunProbe(void)
{
    FTStruct *fp = &sNdsFighterStructPool[1];
    GObj *fighter_gobj;
    DObj *root;
    s32 cliff_id;

    if ((ndsFighterStructIsPoolPointer(fp) == FALSE) ||
        (fp->fighter_gobj == NULL) ||
        (gNdsStageMPCliffClimbFloorLoopClimbStatusAfter !=
            (u32)nFTCommonStatusCliffQuick) ||
        (gNdsStageMPCliffClimbFloorLoopQueuedStatusID !=
            (u32)nFTCommonCliffKindClimbQuick) ||
        (gNdsStageMPCliffClimbFloorLoopClimbCliffIDAfter < 0))
    {
        gNdsStageMPCliffClimbActionLoopUnsafeCount++;
        return;
    }

    fighter_gobj = fp->fighter_gobj;
    root = DObjGetStruct(fighter_gobj);
    if (root == NULL)
    {
        gNdsStageMPCliffClimbActionLoopUnsafeCount++;
        return;
    }
    if (fp->joints[nFTPartsJointTransN] == NULL)
    {
        fp->joints[nFTPartsJointTransN] = root;
    }

    cliff_id = gNdsStageMPCliffClimbFloorLoopClimbCliffIDAfter;
    fp->status_id = nFTCommonStatusCliffQuick;
    fp->motion_id = nFTCommonMotionCliffQuick;
    fp->motion_script_id = nFTCommonMotionCliffQuick;
    fp->ga = nMPKineticsGround;
    fp->lr = gNdsStageMPCliffClimbFloorLoopLRBefore;
    fp->is_cliff_hold = TRUE;
    fp->percent_damage = 0;
    fp->proc_update = ftCommonCliffQuickProcUpdate;
    fp->proc_interrupt = NULL;
    fp->proc_physics = ftCommonCliffCommonProcPhysics;
    fp->proc_map = ftCommonCliffCommonProcMap;
    fp->proc_damage = ftCommonCliffCommonProcDamage;
    fp->coll_data.cliff_id = cliff_id;
    fp->coll_data.floor_line_id = cliff_id;
    fp->coll_data.p_translate = &root->translate.vec.f;
    fp->coll_data.p_lr = &fp->lr;
    fp->coll_data.p_map_coll = &fp->coll_data.map_coll;
    fp->status_vars.common.cliffmotion.status_id =
        nFTCommonCliffKindClimbQuick;
    fp->status_vars.common.cliffmotion.cliff_id = cliff_id;
    fighter_gobj->anim_frame = 0.0F;
    fp->anim_frame = 0.0F;

    gNdsStageMPCliffClimbActionLoopStatusBefore = (u32)fp->status_id;
    gNdsStageMPCliffClimbActionLoopMotionBefore = (u32)fp->motion_id;
    gNdsStageMPCliffClimbActionLoopGABefore = (u32)fp->ga;
    gNdsStageMPCliffClimbActionLoopCliffIDBefore = fp->coll_data.cliff_id;
    gNdsStageMPCliffClimbActionLoopQueuedStatusID =
        (u32)fp->status_vars.common.cliffmotion.status_id;
    gNdsStageMPCliffClimbActionLoopQueuedCliffID =
        fp->status_vars.common.cliffmotion.cliff_id;

    gNdsStageMPCliffClimbActionLoopQuickUpdateCallCount++;
    sNdsStageMPCliffClimbActionLoopQuickUpdateActive = TRUE;
    sNdsStageMPCliffClimbActionLoopSetStatusActive = TRUE;
    ftCommonCliffQuickProcUpdate(fighter_gobj);
    sNdsStageMPCliffClimbActionLoopSetStatusActive = FALSE;
    sNdsStageMPCliffClimbActionLoopQuickUpdateActive = FALSE;

    gNdsStageMPCliffClimbActionLoopStatusAfterQuick1 = (u32)fp->status_id;
    gNdsStageMPCliffClimbActionLoopMotionAfterQuick1 = (u32)fp->motion_id;
    gNdsStageMPCliffClimbActionLoopGAAfterQuick1 = (u32)fp->ga;
    gNdsStageMPCliffClimbActionLoopCliffIDAfterQuick1 =
        fp->coll_data.cliff_id;
    gNdsStageMPCliffClimbActionLoopIsCliffHoldAfterQuick1 =
        (fp->is_cliff_hold != FALSE) ? 1u : 0u;
    gNdsStageMPCliffClimbActionLoopProcDamageSetAfterQuick1 =
        (fp->proc_damage == ftCommonCliffCommonProcDamage) ? 1u : 0u;
    gNdsStageMPCliffClimbActionLoopProcUpdateSetAfterQuick1 =
        (fp->proc_update == ftCommonCliffClimbQuick1ProcUpdate) ? 1u : 0u;

    if ((fp->status_id != nFTCommonStatusCliffClimbQuick1) ||
        (fp->motion_id != nFTCommonMotionCliffClimbQuick1) ||
        (fp->proc_update != ftCommonCliffClimbQuick1ProcUpdate))
    {
        gNdsStageMPCliffClimbActionLoopUnsafeCount++;
        return;
    }

    fighter_gobj->anim_frame = 0.0F;
    fp->anim_frame = 0.0F;

    gNdsStageMPCliffClimbActionLoopQuick1UpdateCallCount++;
    sNdsStageMPCliffClimbActionLoopAnimEndActive = TRUE;
    fp->proc_update(fighter_gobj);
    sNdsStageMPCliffClimbActionLoopAnimEndActive = FALSE;

    gNdsStageMPCliffClimbActionLoopStatusAfterQuick2 = (u32)fp->status_id;
    gNdsStageMPCliffClimbActionLoopMotionAfterQuick2 = (u32)fp->motion_id;
    gNdsStageMPCliffClimbActionLoopGAAfterQuick2 = (u32)fp->ga;
    gNdsStageMPCliffClimbActionLoopCliffIDAfterQuick2 =
        fp->coll_data.cliff_id;
    gNdsStageMPCliffClimbActionLoopFloorLineAfterQuick2 =
        fp->coll_data.floor_line_id;
    gNdsStageMPCliffClimbActionLoopIsCliffHoldAfterQuick2 =
        (fp->is_cliff_hold != FALSE) ? 1u : 0u;
    gNdsStageMPCliffClimbActionLoopProcDamageSetAfterQuick2 =
        (fp->proc_damage != NULL) ? 1u : 0u;
    gNdsStageMPCliffClimbActionLoopProcMapSetAfterQuick2 =
        (fp->proc_map == ftCommonCliffClimbCommon2ProcMap) ? 1u : 0u;
    gNdsStageMPCliffClimbActionLoopJostleIgnoreAfterQuick2 =
        (fp->is_jostle_ignore != FALSE) ? 1u : 0u;
}

void ndsFighterMarioFoxStageMPCliffClimbActionLoopFinalize(void)
{
    u32 mask = 0u;

    if ((ndsFighterMarioFoxStageMPCliffClimbActionLoopProofEnabled() ==
            FALSE) ||
        (gNdsStageMPCliffClimbActionLoopPrepared == 0u) ||
        (gNdsFighterMarioFoxStageMPCliffClimbActionLoopResult != 0u))
    {
        return;
    }

    if (gNdsFighterMarioFoxStageMPCliffClimbFloorLoopResult == 0u)
    {
        ndsFighterMarioFoxStageMPCliffClimbFloorLoopFinalize();
    }
    if ((gNdsFighterMarioFoxStageMPCliffClimbFloorLoopResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFCLIMB_FLOOR_LOOP_PASS) &&
        (gNdsFighterMarioFoxStageMPCliffClimbFloorLoopSafeResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFCLIMB_FLOOR_LOOP_SAFE_PASS))
    {
        gNdsStageMPCliffClimbActionLoopBaseMPCliffClimbSeen = 1u;
        mask |= 1u << 0;
    }
    if ((gNdsStageMPCliffClimbActionLoopPrepared != 0u) &&
        (gNdsStageMPCliffClimbActionLoopBaseMPCliffClimbSeen != 0u))
    {
        mask |= 1u << 1;
    }

    if ((gNdsStageMPCliffClimbActionLoopQuickUpdateCallCount == 0u) &&
        (gNdsStageMPCliffClimbActionLoopUnsafeCount == 0u))
    {
        ndsStageMPCliffClimbActionLoopRunProbe();
    }

    if ((gNdsStageMPCliffClimbActionLoopStatusBefore ==
            (u32)nFTCommonStatusCliffQuick) &&
        (gNdsStageMPCliffClimbActionLoopMotionBefore ==
            (u32)nFTCommonMotionCliffQuick) &&
        (gNdsStageMPCliffClimbActionLoopGABefore ==
            (u32)nMPKineticsGround) &&
        (gNdsStageMPCliffClimbActionLoopCliffIDBefore >= 0) &&
        (gNdsStageMPCliffClimbActionLoopQueuedStatusID ==
            (u32)nFTCommonCliffKindClimbQuick) &&
        (gNdsStageMPCliffClimbActionLoopQueuedCliffID ==
            gNdsStageMPCliffClimbActionLoopCliffIDBefore))
    {
        mask |= 1u << 2;
    }
    if ((gNdsStageMPCliffClimbActionLoopQuickUpdateCallCount == 1u) &&
        (gNdsStageMPCliffClimbActionLoopQuick1SetStatusCount == 1u))
    {
        mask |= 1u << 3;
    }
    if ((gNdsStageMPCliffClimbActionLoopStatusAfterQuick1 ==
            (u32)nFTCommonStatusCliffClimbQuick1) &&
        (gNdsStageMPCliffClimbActionLoopMotionAfterQuick1 ==
            (u32)nFTCommonMotionCliffClimbQuick1) &&
        (gNdsStageMPCliffClimbActionLoopGAAfterQuick1 ==
            (u32)nMPKineticsGround) &&
        (gNdsStageMPCliffClimbActionLoopCliffIDAfterQuick1 ==
            gNdsStageMPCliffClimbActionLoopCliffIDBefore))
    {
        mask |= 1u << 4;
    }
    if ((gNdsStageMPCliffClimbActionLoopQuick1UpdateCallCount == 1u) &&
        (gNdsStageMPCliffClimbActionLoopAnimEndCheckCount == 1u) &&
        (gNdsStageMPCliffClimbActionLoopQuick2SetStatusCount == 1u))
    {
        mask |= 1u << 5;
    }
    if ((gNdsStageMPCliffClimbActionLoopCommon2UpdateCollCount == 1u) &&
        (gNdsStageMPCliffClimbActionLoopCommon2InitVarsCount == 1u))
    {
        mask |= 1u << 6;
    }
    if ((gNdsStageMPCliffClimbActionLoopStatusAfterQuick2 ==
            (u32)nFTCommonStatusCliffClimbQuick2) &&
        (gNdsStageMPCliffClimbActionLoopMotionAfterQuick2 ==
            (u32)nFTCommonMotionCliffClimbQuick2) &&
        (gNdsStageMPCliffClimbActionLoopGAAfterQuick2 ==
            (u32)nMPKineticsGround) &&
        (gNdsStageMPCliffClimbActionLoopProcMapSetAfterQuick2 == 1u))
    {
        mask |= 1u << 7;
    }
    if ((gNdsStageMPCliffClimbActionLoopCliffIDAfterQuick2 ==
            gNdsStageMPCliffClimbActionLoopCliffIDBefore) &&
        (gNdsStageMPCliffClimbActionLoopFloorLineAfterQuick2 ==
            gNdsStageMPCliffClimbActionLoopCliffIDBefore))
    {
        mask |= 1u << 8;
    }
    if ((gNdsStageMPCliffClimbActionLoopIsCliffHoldAfterQuick1 == 1u) &&
        (gNdsStageMPCliffClimbActionLoopIsCliffHoldAfterQuick2 == 0u) &&
        (gNdsStageMPCliffClimbActionLoopProcDamageSetAfterQuick1 == 1u) &&
        (gNdsStageMPCliffClimbActionLoopProcDamageSetAfterQuick2 == 0u) &&
        (gNdsStageMPCliffClimbActionLoopProcUpdateSetAfterQuick1 == 1u) &&
        (gNdsStageMPCliffClimbActionLoopJostleIgnoreAfterQuick2 == 1u))
    {
        mask |= 1u << 9;
    }
    if ((gNdsStageMPCliffClimbActionLoopUnsafeCount == 0u) &&
        (sNdsStageMPCliffClimbActionLoopQuickUpdateActive == FALSE) &&
        (sNdsStageMPCliffClimbActionLoopSetStatusActive == FALSE) &&
        (sNdsStageMPCliffClimbActionLoopAnimEndActive == FALSE))
    {
        mask |= 1u << 10;
    }
    if ((gNdsStageMPCliffCommon2BridgeCallCount == 1u) &&
        (gNdsStageMPCliffCommon2BridgeGuardPassCount == 1u) &&
        (gNdsStageMPCliffCommon2BridgeGuardRejectCount == 0u) &&
        (gNdsStageMPCliffCommon2BridgeStatusID ==
            nFTCommonCliffKindClimbQuick) &&
        (gNdsStageMPCliffCommon2BridgeRootPositionOK == 1u))
    {
        mask |= 1u << 11;
    }

    gNdsFighterMarioFoxStageMPCliffClimbActionLoopCount = 1u;
    gNdsFighterMarioFoxStageMPCliffClimbActionLoopMask = mask;
    gNdsFighterMarioFoxStageMPCliffClimbActionLoopDeferredMask = 0xffu;
    if ((mask & 0xfffu) == 0xfffu)
    {
        gNdsFighterMarioFoxStageMPCliffClimbActionLoopResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFCLIMB_ACTION_LOOP_PASS;
        gNdsFighterMarioFoxStageMPCliffClimbActionLoopSafeResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFCLIMB_ACTION_LOOP_SAFE_PASS;
    }
}

static void ndsFighterMarioFoxStageMPCliffClimbCommon2LoopReset(void)
{
    gNdsFighterMarioFoxStageMPCliffClimbCommon2LoopResult = 0u;
    gNdsFighterMarioFoxStageMPCliffClimbCommon2LoopSafeResult = 0u;
    gNdsFighterMarioFoxStageMPCliffClimbCommon2LoopMask = 0u;
    gNdsFighterMarioFoxStageMPCliffClimbCommon2LoopDeferredMask = 0u;
    gNdsFighterMarioFoxStageMPCliffClimbCommon2LoopCount = 0u;
    gNdsStageMPCliffClimbCommon2LoopPrepared = 0u;
    gNdsStageMPCliffClimbCommon2LoopBaseMPCliffClimbActionSeen = 0u;
    gNdsStageMPCliffClimbCommon2LoopUpdateCallCount = 0u;
    gNdsStageMPCliffClimbCommon2LoopAnimEndCheckCount = 0u;
    gNdsStageMPCliffClimbCommon2LoopWaitOrFallCallCount = 0u;
    gNdsStageMPCliffClimbCommon2LoopPhysicsCallCount = 0u;
    gNdsStageMPCliffClimbCommon2LoopGroundTransCount = 0u;
    gNdsStageMPCliffClimbCommon2LoopMapCallCount = 0u;
    gNdsStageMPCliffClimbCommon2LoopGroundBreakCount = 0u;
    gNdsStageMPCliffClimbCommon2LoopUnsafeCount = 0u;
    gNdsStageMPCliffClimbCommon2LoopStatusBefore = 0u;
    gNdsStageMPCliffClimbCommon2LoopMotionBefore = 0u;
    gNdsStageMPCliffClimbCommon2LoopGABefore = 0u;
    gNdsStageMPCliffClimbCommon2LoopStatusAfterUpdate = 0u;
    gNdsStageMPCliffClimbCommon2LoopMotionAfterUpdate = 0u;
    gNdsStageMPCliffClimbCommon2LoopGAAfterUpdate = 0u;
    gNdsStageMPCliffClimbCommon2LoopStatusAfterPhysics = 0u;
    gNdsStageMPCliffClimbCommon2LoopMotionAfterPhysics = 0u;
    gNdsStageMPCliffClimbCommon2LoopGAAfterPhysics = 0u;
    gNdsStageMPCliffClimbCommon2LoopStatusAfterMap = 0u;
    gNdsStageMPCliffClimbCommon2LoopMotionAfterMap = 0u;
    gNdsStageMPCliffClimbCommon2LoopGAAfterMap = 0u;
    gNdsStageMPCliffClimbCommon2LoopCliffIDBefore = -1;
    gNdsStageMPCliffClimbCommon2LoopFloorLineBefore = -1;
    gNdsStageMPCliffClimbCommon2LoopCliffIDAfterMap = -1;
    gNdsStageMPCliffClimbCommon2LoopFloorLineAfterMap = -1;
    gNdsStageMPCliffClimbCommon2LoopProcUpdateSet = 0u;
    gNdsStageMPCliffClimbCommon2LoopProcPhysicsSet = 0u;
    gNdsStageMPCliffClimbCommon2LoopProcMapSet = 0u;
    gNdsStageMPCliffClimbCommon2LoopIsCliffHoldAfterMap = 0u;
    sNdsStageMPCliffClimbCommon2LoopUpdateActive = FALSE;
    sNdsStageMPCliffClimbCommon2LoopPhysicsActive = FALSE;
    sNdsStageMPCliffClimbCommon2LoopMapActive = FALSE;
}

void ndsFighterMarioFoxStageMPCliffClimbCommon2LoopPrepare(void)
{
    if ((ndsFighterMarioFoxStageMPCliffClimbCommon2LoopProofEnabled() ==
            FALSE) ||
        (gNdsStageMPCliffClimbCommon2LoopPrepared != 0u))
    {
        return;
    }
    ndsFighterMarioFoxStageMPCliffClimbCommon2LoopReset();
    gNdsStageMPCliffClimbCommon2LoopPrepared = 1u;
    gNdsStageMPCliffClimbCommon2LoopBaseMPCliffClimbActionSeen =
        (gNdsStageMPCliffClimbActionLoopPrepared != 0u) ? 1u : 0u;
}

static void ndsStageMPCliffClimbCommon2LoopRunProbe(void)
{
    FTStruct *fp = &sNdsFighterStructPool[1];
    GObj *fighter_gobj;

    if ((ndsFighterStructIsPoolPointer(fp) == FALSE) ||
        (fp->fighter_gobj == NULL) ||
        (fp->status_id != nFTCommonStatusCliffClimbQuick2) ||
        (fp->motion_id != nFTCommonMotionCliffClimbQuick2) ||
        (fp->ga != nMPKineticsGround) ||
        (fp->coll_data.cliff_id < 0) ||
        (fp->coll_data.floor_line_id != fp->coll_data.cliff_id))
    {
        gNdsStageMPCliffClimbCommon2LoopUnsafeCount++;
        return;
    }

    fighter_gobj = fp->fighter_gobj;
    gNdsStageMPCliffClimbCommon2LoopProcUpdateSet =
        (fp->proc_update == ftCommonCliffCommon2ProcUpdate) ? 1u : 0u;
    gNdsStageMPCliffClimbCommon2LoopProcPhysicsSet =
        (fp->proc_physics == ftCommonCliffCommon2ProcPhysics) ? 1u : 0u;
    gNdsStageMPCliffClimbCommon2LoopProcMapSet =
        (fp->proc_map == ftCommonCliffClimbCommon2ProcMap) ? 1u : 0u;
    if ((gNdsStageMPCliffClimbCommon2LoopProcUpdateSet == 0u) ||
        (gNdsStageMPCliffClimbCommon2LoopProcPhysicsSet == 0u) ||
        (gNdsStageMPCliffClimbCommon2LoopProcMapSet == 0u))
    {
        gNdsStageMPCliffClimbCommon2LoopUnsafeCount++;
        return;
    }

    fighter_gobj->anim_frame = 1.0F;
    fp->anim_frame = 1.0F;

    gNdsStageMPCliffClimbCommon2LoopStatusBefore = (u32)fp->status_id;
    gNdsStageMPCliffClimbCommon2LoopMotionBefore = (u32)fp->motion_id;
    gNdsStageMPCliffClimbCommon2LoopGABefore = (u32)fp->ga;
    gNdsStageMPCliffClimbCommon2LoopCliffIDBefore = fp->coll_data.cliff_id;
    gNdsStageMPCliffClimbCommon2LoopFloorLineBefore =
        fp->coll_data.floor_line_id;

    gNdsStageMPCliffClimbCommon2LoopUpdateCallCount++;
    sNdsStageMPCliffClimbCommon2LoopUpdateActive = TRUE;
    fp->proc_update(fighter_gobj);
    sNdsStageMPCliffClimbCommon2LoopUpdateActive = FALSE;
    gNdsStageMPCliffClimbCommon2LoopStatusAfterUpdate =
        (u32)fp->status_id;
    gNdsStageMPCliffClimbCommon2LoopMotionAfterUpdate =
        (u32)fp->motion_id;
    gNdsStageMPCliffClimbCommon2LoopGAAfterUpdate = (u32)fp->ga;

    gNdsStageMPCliffClimbCommon2LoopPhysicsCallCount++;
    sNdsStageMPCliffClimbCommon2LoopPhysicsActive = TRUE;
    fp->proc_physics(fighter_gobj);
    if (gNdsStageMPCliffClimbCommon2LoopGroundTransCount == 0u)
    {
        ftPhysicsApplyGroundVelTransN(fighter_gobj);
    }
    sNdsStageMPCliffClimbCommon2LoopPhysicsActive = FALSE;
    gNdsStageMPCliffClimbCommon2LoopStatusAfterPhysics =
        (u32)fp->status_id;
    gNdsStageMPCliffClimbCommon2LoopMotionAfterPhysics =
        (u32)fp->motion_id;
    gNdsStageMPCliffClimbCommon2LoopGAAfterPhysics = (u32)fp->ga;

    gNdsStageMPCliffClimbCommon2LoopMapCallCount++;
    sNdsStageMPCliffClimbCommon2LoopMapActive = TRUE;
    fp->proc_map(fighter_gobj);
    if (gNdsStageMPCliffClimbCommon2LoopGroundBreakCount == 0u)
    {
        mpCommonSetFighterFallOnGroundBreak(fighter_gobj);
    }
    sNdsStageMPCliffClimbCommon2LoopMapActive = FALSE;
    gNdsStageMPCliffClimbCommon2LoopStatusAfterMap = (u32)fp->status_id;
    gNdsStageMPCliffClimbCommon2LoopMotionAfterMap = (u32)fp->motion_id;
    gNdsStageMPCliffClimbCommon2LoopGAAfterMap = (u32)fp->ga;
    gNdsStageMPCliffClimbCommon2LoopCliffIDAfterMap =
        fp->coll_data.cliff_id;
    gNdsStageMPCliffClimbCommon2LoopFloorLineAfterMap =
        fp->coll_data.floor_line_id;
    gNdsStageMPCliffClimbCommon2LoopIsCliffHoldAfterMap =
        (fp->is_cliff_hold != FALSE) ? 1u : 0u;
}

void ndsFighterMarioFoxStageMPCliffClimbCommon2LoopFinalize(void)
{
    u32 mask = 0u;

    if ((ndsFighterMarioFoxStageMPCliffClimbCommon2LoopProofEnabled() ==
            FALSE) ||
        (gNdsStageMPCliffClimbCommon2LoopPrepared == 0u) ||
        (gNdsFighterMarioFoxStageMPCliffClimbCommon2LoopResult != 0u))
    {
        return;
    }

    if (gNdsFighterMarioFoxStageMPCliffClimbActionLoopResult == 0u)
    {
        ndsFighterMarioFoxStageMPCliffClimbActionLoopFinalize();
    }
    if ((gNdsFighterMarioFoxStageMPCliffClimbActionLoopResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFCLIMB_ACTION_LOOP_PASS) &&
        (gNdsFighterMarioFoxStageMPCliffClimbActionLoopSafeResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFCLIMB_ACTION_LOOP_SAFE_PASS))
    {
        gNdsStageMPCliffClimbCommon2LoopBaseMPCliffClimbActionSeen = 1u;
        mask |= 1u << 0;
    }
    if ((gNdsStageMPCliffClimbCommon2LoopPrepared != 0u) &&
        (gNdsStageMPCliffClimbCommon2LoopBaseMPCliffClimbActionSeen != 0u))
    {
        mask |= 1u << 1;
    }

    if ((gNdsStageMPCliffClimbCommon2LoopUpdateCallCount == 0u) &&
        (gNdsStageMPCliffClimbCommon2LoopUnsafeCount == 0u))
    {
        ndsStageMPCliffClimbCommon2LoopRunProbe();
    }

    if ((gNdsStageMPCliffClimbCommon2LoopStatusBefore ==
            (u32)nFTCommonStatusCliffClimbQuick2) &&
        (gNdsStageMPCliffClimbCommon2LoopMotionBefore ==
            (u32)nFTCommonMotionCliffClimbQuick2) &&
        (gNdsStageMPCliffClimbCommon2LoopGABefore ==
            (u32)nMPKineticsGround) &&
        (gNdsStageMPCliffClimbCommon2LoopCliffIDBefore >= 0) &&
        (gNdsStageMPCliffClimbCommon2LoopFloorLineBefore ==
            gNdsStageMPCliffClimbCommon2LoopCliffIDBefore))
    {
        mask |= 1u << 2;
    }
    if ((gNdsStageMPCliffClimbCommon2LoopProcUpdateSet == 1u) &&
        (gNdsStageMPCliffClimbCommon2LoopProcPhysicsSet == 1u) &&
        (gNdsStageMPCliffClimbCommon2LoopProcMapSet == 1u))
    {
        mask |= 1u << 3;
    }
    if ((gNdsStageMPCliffClimbCommon2LoopUpdateCallCount == 1u) &&
        (gNdsStageMPCliffClimbCommon2LoopAnimEndCheckCount == 1u) &&
        (gNdsStageMPCliffClimbCommon2LoopWaitOrFallCallCount == 0u) &&
        (gNdsStageMPCliffClimbCommon2LoopStatusAfterUpdate ==
            (u32)nFTCommonStatusCliffClimbQuick2) &&
        (gNdsStageMPCliffClimbCommon2LoopMotionAfterUpdate ==
            (u32)nFTCommonMotionCliffClimbQuick2) &&
        (gNdsStageMPCliffClimbCommon2LoopGAAfterUpdate ==
            (u32)nMPKineticsGround))
    {
        mask |= 1u << 4;
    }
    if ((gNdsStageMPCliffClimbCommon2LoopPhysicsCallCount == 1u) &&
        (gNdsStageMPCliffClimbCommon2LoopGroundTransCount == 1u) &&
        (gNdsStageMPCliffClimbCommon2LoopStatusAfterPhysics ==
            (u32)nFTCommonStatusCliffClimbQuick2) &&
        (gNdsStageMPCliffClimbCommon2LoopMotionAfterPhysics ==
            (u32)nFTCommonMotionCliffClimbQuick2) &&
        (gNdsStageMPCliffClimbCommon2LoopGAAfterPhysics ==
            (u32)nMPKineticsGround))
    {
        mask |= 1u << 5;
    }
    if ((gNdsStageMPCliffClimbCommon2LoopMapCallCount == 1u) &&
        (gNdsStageMPCliffClimbCommon2LoopGroundBreakCount == 1u) &&
        (gNdsStageMPCliffClimbCommon2LoopStatusAfterMap ==
            (u32)nFTCommonStatusCliffClimbQuick2) &&
        (gNdsStageMPCliffClimbCommon2LoopMotionAfterMap ==
            (u32)nFTCommonMotionCliffClimbQuick2) &&
        (gNdsStageMPCliffClimbCommon2LoopGAAfterMap ==
            (u32)nMPKineticsGround))
    {
        mask |= 1u << 6;
    }
    if ((gNdsStageMPCliffClimbCommon2LoopCliffIDAfterMap ==
            gNdsStageMPCliffClimbCommon2LoopCliffIDBefore) &&
        (gNdsStageMPCliffClimbCommon2LoopFloorLineAfterMap ==
            gNdsStageMPCliffClimbCommon2LoopFloorLineBefore))
    {
        mask |= 1u << 7;
    }
    if ((gNdsStageMPCliffClimbCommon2LoopIsCliffHoldAfterMap == 0u) &&
        (gNdsStageMPCliffClimbCommon2LoopUnsafeCount == 0u) &&
        (sNdsStageMPCliffClimbCommon2LoopUpdateActive == FALSE) &&
        (sNdsStageMPCliffClimbCommon2LoopPhysicsActive == FALSE) &&
        (sNdsStageMPCliffClimbCommon2LoopMapActive == FALSE))
    {
        mask |= 1u << 8;
    }

    gNdsFighterMarioFoxStageMPCliffClimbCommon2LoopCount = 1u;
    gNdsFighterMarioFoxStageMPCliffClimbCommon2LoopMask = mask;
    gNdsFighterMarioFoxStageMPCliffClimbCommon2LoopDeferredMask = 0xffu;
    if ((mask & 0x1ffu) == 0x1ffu)
    {
        gNdsFighterMarioFoxStageMPCliffClimbCommon2LoopResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFCLIMB_COMMON2_LOOP_PASS;
        gNdsFighterMarioFoxStageMPCliffClimbCommon2LoopSafeResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFCLIMB_COMMON2_LOOP_SAFE_PASS;
    }
}

static void ndsFighterMarioFoxStageMPCliffClimbFinishLoopReset(void)
{
    gNdsFighterMarioFoxStageMPCliffClimbFinishLoopResult = 0u;
    gNdsFighterMarioFoxStageMPCliffClimbFinishLoopSafeResult = 0u;
    gNdsFighterMarioFoxStageMPCliffClimbFinishLoopMask = 0u;
    gNdsFighterMarioFoxStageMPCliffClimbFinishLoopDeferredMask = 0u;
    gNdsFighterMarioFoxStageMPCliffClimbFinishLoopCount = 0u;
    gNdsStageMPCliffClimbFinishLoopPrepared = 0u;
    gNdsStageMPCliffClimbFinishLoopBaseMPCliffClimbCommon2Seen = 0u;
    gNdsStageMPCliffClimbFinishLoopUpdateCallCount = 0u;
    gNdsStageMPCliffClimbFinishLoopAnimEndCheckCount = 0u;
    gNdsStageMPCliffClimbFinishLoopWaitOrFallCallCount = 0u;
    gNdsStageMPCliffClimbFinishLoopWaitSetStatusCount = 0u;
    gNdsStageMPCliffClimbFinishLoopPlayerTagWaitCount = 0u;
    gNdsStageMPCliffClimbFinishLoopUnsafeCount = 0u;
    gNdsStageMPCliffClimbFinishLoopStatusBefore = 0u;
    gNdsStageMPCliffClimbFinishLoopMotionBefore = 0u;
    gNdsStageMPCliffClimbFinishLoopGABefore = 0u;
    gNdsStageMPCliffClimbFinishLoopStatusAfterUpdate = 0u;
    gNdsStageMPCliffClimbFinishLoopMotionAfterUpdate = 0u;
    gNdsStageMPCliffClimbFinishLoopGAAfterUpdate = 0u;
    gNdsStageMPCliffClimbFinishLoopCliffIDBefore = -1;
    gNdsStageMPCliffClimbFinishLoopFloorLineBefore = -1;
    gNdsStageMPCliffClimbFinishLoopCliffIDAfterUpdate = -1;
    gNdsStageMPCliffClimbFinishLoopFloorLineAfterUpdate = -1;
    gNdsStageMPCliffClimbFinishLoopProcUpdateSet = 0u;
    gNdsStageMPCliffClimbFinishLoopProcWaitSet = 0u;
    gNdsStageMPCliffClimbFinishLoopSpecialInterruptAfter = 0u;
    gNdsStageMPCliffClimbFinishLoopPlayerTagWaitAfter = 0;
    gNdsStageMPCliffClimbFinishLoopIsCliffHoldAfterUpdate = 0u;
    gNdsStageMPCliffClimbFinishLoopIsJostleIgnoreAfterUpdate = 0u;
    gNdsStageMPCliffClimbFinishLoopCommonResetMask = 0u;
    sNdsStageMPCliffClimbFinishLoopUpdateActive = FALSE;
}

void ndsFighterMarioFoxStageMPCliffClimbFinishLoopPrepare(void)
{
    if ((ndsFighterMarioFoxStageMPCliffClimbFinishLoopProofEnabled() ==
            FALSE) ||
        (gNdsStageMPCliffClimbFinishLoopPrepared != 0u))
    {
        return;
    }
    ndsFighterMarioFoxStageMPCliffClimbFinishLoopReset();
    gNdsStageMPCliffClimbFinishLoopPrepared = 1u;
    gNdsStageMPCliffClimbFinishLoopBaseMPCliffClimbCommon2Seen =
        (gNdsStageMPCliffClimbCommon2LoopPrepared != 0u) ? 1u : 0u;
}

static void ndsStageMPCliffClimbFinishLoopRunProbe(void)
{
    FTStruct *fp = &sNdsFighterStructPool[1];
    GObj *fighter_gobj;

    if ((ndsFighterStructIsPoolPointer(fp) == FALSE) ||
        (fp->fighter_gobj == NULL) ||
        (fp->status_id != nFTCommonStatusCliffClimbQuick2) ||
        (fp->motion_id != nFTCommonMotionCliffClimbQuick2) ||
        (fp->ga != nMPKineticsGround) ||
        (fp->coll_data.cliff_id < 0) ||
        (fp->coll_data.floor_line_id != fp->coll_data.cliff_id))
    {
        gNdsStageMPCliffClimbFinishLoopUnsafeCount++;
        return;
    }

    fighter_gobj = fp->fighter_gobj;
    gNdsStageMPCliffClimbFinishLoopProcUpdateSet =
        (fp->proc_update == ftCommonCliffCommon2ProcUpdate) ? 1u : 0u;
    if (gNdsStageMPCliffClimbFinishLoopProcUpdateSet == 0u)
    {
        gNdsStageMPCliffClimbFinishLoopUnsafeCount++;
        return;
    }

    fighter_gobj->anim_frame = 0.0F;
    fp->anim_frame = 0.0F;
    fp->is_reflect = TRUE;
    fp->is_absorb = TRUE;
    fp->is_shield = TRUE;
    fp->is_fastfall = TRUE;
    fp->is_invisible = TRUE;
    fp->is_shadow_hide = TRUE;
    fp->is_playertag_hide = TRUE;
    fp->is_ghost = TRUE;
    fp->is_hitstun = TRUE;
    fp->is_cliff_hold = TRUE;
    fp->is_jostle_ignore = TRUE;
    fp->damage_player = 2;
    fp->coll_data.ignore_line_id = 3;
    fp->capture_immune_mask = 0xffu;
    fp->camera_zoom_range = 2.0F;
    fp->is_special_interrupt = FALSE;
    fp->playertag_wait = 0;
    fp->shuffle_tics = 7;
    fp->knockback_resist_status = 12.0F;
    fp->damage_knockback_stack = 12.0F;
    fp->damage_mul = 0.25F;

    gNdsStageMPCliffClimbFinishLoopStatusBefore = (u32)fp->status_id;
    gNdsStageMPCliffClimbFinishLoopMotionBefore = (u32)fp->motion_id;
    gNdsStageMPCliffClimbFinishLoopGABefore = (u32)fp->ga;
    gNdsStageMPCliffClimbFinishLoopCliffIDBefore = fp->coll_data.cliff_id;
    gNdsStageMPCliffClimbFinishLoopFloorLineBefore =
        fp->coll_data.floor_line_id;

    gNdsStageMPCliffClimbFinishLoopUpdateCallCount++;
    sNdsStageMPCliffClimbFinishLoopUpdateActive = TRUE;
    fp->proc_update(fighter_gobj);
    sNdsStageMPCliffClimbFinishLoopUpdateActive = FALSE;

    gNdsStageMPCliffClimbFinishLoopStatusAfterUpdate = (u32)fp->status_id;
    gNdsStageMPCliffClimbFinishLoopMotionAfterUpdate = (u32)fp->motion_id;
    gNdsStageMPCliffClimbFinishLoopGAAfterUpdate = (u32)fp->ga;
    gNdsStageMPCliffClimbFinishLoopCliffIDAfterUpdate =
        fp->coll_data.cliff_id;
    gNdsStageMPCliffClimbFinishLoopFloorLineAfterUpdate =
        fp->coll_data.floor_line_id;
    gNdsStageMPCliffClimbFinishLoopProcWaitSet =
        ((fp->proc_update == NULL) &&
         (fp->proc_interrupt == ftCommonWaitProcInterrupt) &&
         (fp->proc_physics == ftPhysicsApplyGroundVelFriction) &&
         (fp->proc_map == mpCommonProcFighterOnCliffEdge)) ? 1u : 0u;
    gNdsStageMPCliffClimbFinishLoopSpecialInterruptAfter =
        (fp->is_special_interrupt != FALSE) ? 1u : 0u;
    gNdsStageMPCliffClimbFinishLoopPlayerTagWaitAfter =
        fp->playertag_wait;
    gNdsStageMPCliffClimbFinishLoopIsCliffHoldAfterUpdate =
        (fp->is_cliff_hold != FALSE) ? 1u : 0u;
    gNdsStageMPCliffClimbFinishLoopIsJostleIgnoreAfterUpdate =
        (fp->is_jostle_ignore != FALSE) ? 1u : 0u;
    gNdsStageMPCliffClimbFinishLoopCommonResetMask =
        ((fp->is_reflect == FALSE) ? (1u << 0) : 0u) |
        ((fp->is_absorb == FALSE) ? (1u << 1) : 0u) |
        ((fp->is_shield == FALSE) ? (1u << 2) : 0u) |
        ((fp->is_fastfall == FALSE) ? (1u << 3) : 0u) |
        ((fp->is_invisible == FALSE) ? (1u << 4) : 0u) |
        ((fp->is_shadow_hide == FALSE) ? (1u << 5) : 0u) |
        ((fp->is_playertag_hide == FALSE) ? (1u << 6) : 0u) |
        ((fp->is_ghost == FALSE) ? (1u << 7) : 0u) |
        ((fp->is_hitstun == FALSE) ? (1u << 8) : 0u) |
        ((fp->is_cliff_hold == FALSE) ? (1u << 9) : 0u) |
        ((fp->is_jostle_ignore == FALSE) ? (1u << 10) : 0u) |
        ((fp->damage_player == -1) ? (1u << 11) : 0u) |
        ((fp->coll_data.ignore_line_id == -1) ? (1u << 12) : 0u) |
        ((fp->capture_immune_mask == 0u) ? (1u << 13) : 0u) |
        ((fp->camera_zoom_range == 1.0F) ? (1u << 14) : 0u) |
        ((fp->shuffle_tics == 0) ? (1u << 15) : 0u) |
        ((fp->knockback_resist_status == 0.0F) ? (1u << 16) : 0u) |
        ((fp->damage_knockback_stack == 0.0F) ? (1u << 17) : 0u) |
        ((fp->damage_mul == 1.0F) ? (1u << 18) : 0u);
}

void ndsFighterMarioFoxStageMPCliffClimbFinishLoopFinalize(void)
{
    u32 mask = 0u;

    if ((ndsFighterMarioFoxStageMPCliffClimbFinishLoopProofEnabled() ==
            FALSE) ||
        (gNdsStageMPCliffClimbFinishLoopPrepared == 0u) ||
        (gNdsFighterMarioFoxStageMPCliffClimbFinishLoopResult != 0u))
    {
        return;
    }

    if (gNdsFighterMarioFoxStageMPCliffClimbCommon2LoopResult == 0u)
    {
        ndsFighterMarioFoxStageMPCliffClimbCommon2LoopFinalize();
    }
    if ((gNdsFighterMarioFoxStageMPCliffClimbCommon2LoopResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFCLIMB_COMMON2_LOOP_PASS) &&
        (gNdsFighterMarioFoxStageMPCliffClimbCommon2LoopSafeResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFCLIMB_COMMON2_LOOP_SAFE_PASS))
    {
        gNdsStageMPCliffClimbFinishLoopBaseMPCliffClimbCommon2Seen = 1u;
        mask |= 1u << 0;
    }
    if ((gNdsStageMPCliffClimbFinishLoopPrepared != 0u) &&
        (gNdsStageMPCliffClimbFinishLoopBaseMPCliffClimbCommon2Seen != 0u))
    {
        mask |= 1u << 1;
    }

    if ((gNdsStageMPCliffClimbFinishLoopUpdateCallCount == 0u) &&
        (gNdsStageMPCliffClimbFinishLoopUnsafeCount == 0u))
    {
        ndsStageMPCliffClimbFinishLoopRunProbe();
    }

    if ((gNdsStageMPCliffClimbFinishLoopStatusBefore ==
            (u32)nFTCommonStatusCliffClimbQuick2) &&
        (gNdsStageMPCliffClimbFinishLoopMotionBefore ==
            (u32)nFTCommonMotionCliffClimbQuick2) &&
        (gNdsStageMPCliffClimbFinishLoopGABefore ==
            (u32)nMPKineticsGround) &&
        (gNdsStageMPCliffClimbFinishLoopCliffIDBefore >= 0) &&
        (gNdsStageMPCliffClimbFinishLoopFloorLineBefore ==
            gNdsStageMPCliffClimbFinishLoopCliffIDBefore))
    {
        mask |= 1u << 2;
    }
    if (gNdsStageMPCliffClimbFinishLoopProcUpdateSet == 1u)
    {
        mask |= 1u << 3;
    }
    if ((gNdsStageMPCliffClimbFinishLoopUpdateCallCount == 1u) &&
        (gNdsStageMPCliffClimbFinishLoopAnimEndCheckCount == 1u) &&
        (gNdsStageMPCliffClimbFinishLoopWaitOrFallCallCount == 1u) &&
        (gNdsStageMPCliffClimbFinishLoopWaitSetStatusCount == 1u))
    {
        mask |= 1u << 4;
    }
    if ((gNdsStageMPCliffClimbFinishLoopStatusAfterUpdate ==
            (u32)nFTCommonStatusWait) &&
        (gNdsStageMPCliffClimbFinishLoopMotionAfterUpdate ==
            (u32)nFTCommonMotionWait) &&
        (gNdsStageMPCliffClimbFinishLoopGAAfterUpdate ==
            (u32)nMPKineticsGround))
    {
        mask |= 1u << 5;
    }
    if ((gNdsStageMPCliffClimbFinishLoopPlayerTagWaitCount == 1u) &&
        (gNdsStageMPCliffClimbFinishLoopSpecialInterruptAfter == 1u) &&
        (gNdsStageMPCliffClimbFinishLoopPlayerTagWaitAfter == 120))
    {
        mask |= 1u << 6;
    }
    if ((gNdsStageMPCliffClimbFinishLoopCliffIDAfterUpdate ==
            gNdsStageMPCliffClimbFinishLoopCliffIDBefore) &&
        (gNdsStageMPCliffClimbFinishLoopFloorLineAfterUpdate ==
            gNdsStageMPCliffClimbFinishLoopFloorLineBefore))
    {
        mask |= 1u << 7;
    }
    if (gNdsStageMPCliffClimbFinishLoopProcWaitSet == 1u)
    {
        mask |= 1u << 8;
    }
    if ((gNdsStageMPCliffClimbFinishLoopUnsafeCount == 0u) &&
        (sNdsStageMPCliffClimbFinishLoopUpdateActive == FALSE) &&
        (gNdsStageMPCliffClimbFinishLoopIsCliffHoldAfterUpdate == 0u) &&
        (gNdsStageMPCliffClimbFinishLoopIsJostleIgnoreAfterUpdate == 0u) &&
        ((gNdsStageMPCliffClimbFinishLoopCommonResetMask & 0x7ffffu) ==
            0x7ffffu))
    {
        mask |= 1u << 9;
    }

    gNdsFighterMarioFoxStageMPCliffClimbFinishLoopCount = 1u;
    gNdsFighterMarioFoxStageMPCliffClimbFinishLoopMask = mask;
    gNdsFighterMarioFoxStageMPCliffClimbFinishLoopDeferredMask = 0xffu;
    if ((mask & 0x3ffu) == 0x3ffu)
    {
        gNdsFighterMarioFoxStageMPCliffClimbFinishLoopResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFCLIMB_FINISH_LOOP_PASS;
        gNdsFighterMarioFoxStageMPCliffClimbFinishLoopSafeResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFCLIMB_FINISH_LOOP_SAFE_PASS;
    }
}

static void ndsFighterMarioFoxStageMPCliffAttackActionLoopReset(void)
{
    ndsStageMPCliffCommon2BridgeResetDiagnostics();
    gNdsFighterMarioFoxStageMPCliffAttackActionLoopResult = 0u;
    gNdsFighterMarioFoxStageMPCliffAttackActionLoopSafeResult = 0u;
    gNdsFighterMarioFoxStageMPCliffAttackActionLoopMask = 0u;
    gNdsFighterMarioFoxStageMPCliffAttackActionLoopDeferredMask = 0u;
    gNdsFighterMarioFoxStageMPCliffAttackActionLoopCount = 0u;
    gNdsStageMPCliffAttackActionLoopPrepared = 0u;
    gNdsStageMPCliffAttackActionLoopBaseMPCliffAttackSeen = 0u;
    gNdsStageMPCliffAttackActionLoopQuickUpdateCallCount = 0u;
    gNdsStageMPCliffAttackActionLoopQuick1SetStatusCount = 0u;
    gNdsStageMPCliffAttackActionLoopQuick1UpdateCallCount = 0u;
    gNdsStageMPCliffAttackActionLoopAnimEndCheckCount = 0u;
    gNdsStageMPCliffAttackActionLoopQuick2SetStatusCount = 0u;
    gNdsStageMPCliffAttackActionLoopCommon2UpdateCollCount = 0u;
    gNdsStageMPCliffAttackActionLoopCommon2InitVarsCount = 0u;
    gNdsStageMPCliffAttackActionLoopUnsafeCount = 0u;
    gNdsStageMPCliffAttackActionLoopStatusBefore = 0u;
    gNdsStageMPCliffAttackActionLoopMotionBefore = 0u;
    gNdsStageMPCliffAttackActionLoopGABefore = 0u;
    gNdsStageMPCliffAttackActionLoopStatusAfterQuick1 = 0u;
    gNdsStageMPCliffAttackActionLoopMotionAfterQuick1 = 0u;
    gNdsStageMPCliffAttackActionLoopGAAfterQuick1 = 0u;
    gNdsStageMPCliffAttackActionLoopStatusAfterQuick2 = 0u;
    gNdsStageMPCliffAttackActionLoopMotionAfterQuick2 = 0u;
    gNdsStageMPCliffAttackActionLoopGAAfterQuick2 = 0u;
    gNdsStageMPCliffAttackActionLoopCliffIDBefore = -1;
    gNdsStageMPCliffAttackActionLoopCliffIDAfterQuick1 = -1;
    gNdsStageMPCliffAttackActionLoopCliffIDAfterQuick2 = -1;
    gNdsStageMPCliffAttackActionLoopFloorLineAfterQuick2 = -1;
    gNdsStageMPCliffAttackActionLoopQueuedStatusID = 0u;
    gNdsStageMPCliffAttackActionLoopQueuedCliffID = -1;
    gNdsStageMPCliffAttackActionLoopIsCliffHoldAfterQuick1 = 0u;
    gNdsStageMPCliffAttackActionLoopIsCliffHoldAfterQuick2 = 0u;
    gNdsStageMPCliffAttackActionLoopProcDamageSetAfterQuick1 = 0u;
    gNdsStageMPCliffAttackActionLoopProcDamageSetAfterQuick2 = 0u;
    gNdsStageMPCliffAttackActionLoopProcUpdateSetAfterQuick1 = 0u;
    gNdsStageMPCliffAttackActionLoopProcMapSetAfterQuick2 = 0u;
    gNdsStageMPCliffAttackActionLoopJostleIgnoreAfterQuick2 = 0u;
    sNdsStageMPCliffAttackActionLoopQuickUpdateActive = FALSE;
    sNdsStageMPCliffAttackActionLoopSetStatusActive = FALSE;
    sNdsStageMPCliffAttackActionLoopAnimEndActive = FALSE;
}

void ndsFighterMarioFoxStageMPCliffAttackActionLoopPrepare(void)
{
    if ((ndsFighterMarioFoxStageMPCliffAttackActionLoopProofEnabled() ==
            FALSE) ||
        (gNdsStageMPCliffAttackActionLoopPrepared != 0u))
    {
        return;
    }
    ndsFighterMarioFoxStageMPCliffAttackActionLoopReset();
    gNdsStageMPCliffAttackActionLoopPrepared = 1u;
    gNdsStageMPCliffAttackActionLoopBaseMPCliffAttackSeen =
        (gNdsStageMPCliffAttackFloorLoopPrepared != 0u) ? 1u : 0u;
}

static void ndsStageMPCliffAttackActionLoopRunProbe(void)
{
    FTStruct *fp = &sNdsFighterStructPool[1];
    GObj *fighter_gobj;

    if ((ndsFighterStructIsPoolPointer(fp) == FALSE) ||
        (fp->fighter_gobj == NULL) ||
        (fp->status_id != nFTCommonStatusCliffQuick) ||
        (fp->motion_id != nFTCommonMotionCliffQuick) ||
        (fp->ga != nMPKineticsGround) ||
        (fp->coll_data.cliff_id < 0) ||
        (fp->status_vars.common.cliffmotion.status_id !=
            nFTCommonCliffKindAttackQuick) ||
        (fp->status_vars.common.cliffmotion.cliff_id != fp->coll_data.cliff_id))
    {
        gNdsStageMPCliffAttackActionLoopUnsafeCount++;
        return;
    }

    fighter_gobj = fp->fighter_gobj;
    fighter_gobj->anim_frame = 0.0F;
    fp->anim_frame = 0.0F;

    gNdsStageMPCliffAttackActionLoopStatusBefore = (u32)fp->status_id;
    gNdsStageMPCliffAttackActionLoopMotionBefore = (u32)fp->motion_id;
    gNdsStageMPCliffAttackActionLoopGABefore = (u32)fp->ga;
    gNdsStageMPCliffAttackActionLoopCliffIDBefore = fp->coll_data.cliff_id;
    gNdsStageMPCliffAttackActionLoopQueuedStatusID =
        (u32)fp->status_vars.common.cliffmotion.status_id;
    gNdsStageMPCliffAttackActionLoopQueuedCliffID =
        fp->status_vars.common.cliffmotion.cliff_id;

    ndsStageMPCliffCommon2BridgeResetDiagnostics();
    gNdsStageMPCliffAttackActionLoopQuickUpdateCallCount++;
    sNdsStageMPCliffAttackActionLoopQuickUpdateActive = TRUE;
    sNdsStageMPCliffAttackActionLoopSetStatusActive = TRUE;
    ftCommonCliffQuickProcUpdate(fighter_gobj);
    sNdsStageMPCliffAttackActionLoopSetStatusActive = FALSE;
    sNdsStageMPCliffAttackActionLoopQuickUpdateActive = FALSE;

    gNdsStageMPCliffAttackActionLoopStatusAfterQuick1 = (u32)fp->status_id;
    gNdsStageMPCliffAttackActionLoopMotionAfterQuick1 = (u32)fp->motion_id;
    gNdsStageMPCliffAttackActionLoopGAAfterQuick1 = (u32)fp->ga;
    gNdsStageMPCliffAttackActionLoopCliffIDAfterQuick1 =
        fp->coll_data.cliff_id;
    gNdsStageMPCliffAttackActionLoopIsCliffHoldAfterQuick1 =
        (fp->is_cliff_hold != FALSE) ? 1u : 0u;
    gNdsStageMPCliffAttackActionLoopProcDamageSetAfterQuick1 =
        (fp->proc_damage == ftCommonCliffCommonProcDamage) ? 1u : 0u;
    gNdsStageMPCliffAttackActionLoopProcUpdateSetAfterQuick1 =
        (fp->proc_update == ftCommonCliffAttackQuick1ProcUpdate) ? 1u : 0u;

    if ((fp->status_id != nFTCommonStatusCliffAttackQuick1) ||
        (fp->motion_id != nFTCommonMotionCliffAttackQuick1) ||
        (fp->proc_update != ftCommonCliffAttackQuick1ProcUpdate))
    {
        gNdsStageMPCliffAttackActionLoopUnsafeCount++;
        return;
    }

    fighter_gobj->anim_frame = 0.0F;
    fp->anim_frame = 0.0F;

    gNdsStageMPCliffAttackActionLoopQuick1UpdateCallCount++;
    sNdsStageMPCliffAttackActionLoopAnimEndActive = TRUE;
    fp->proc_update(fighter_gobj);
    sNdsStageMPCliffAttackActionLoopAnimEndActive = FALSE;

    gNdsStageMPCliffAttackActionLoopStatusAfterQuick2 = (u32)fp->status_id;
    gNdsStageMPCliffAttackActionLoopMotionAfterQuick2 = (u32)fp->motion_id;
    gNdsStageMPCliffAttackActionLoopGAAfterQuick2 = (u32)fp->ga;
    gNdsStageMPCliffAttackActionLoopCliffIDAfterQuick2 =
        fp->coll_data.cliff_id;
    gNdsStageMPCliffAttackActionLoopFloorLineAfterQuick2 =
        fp->coll_data.floor_line_id;
    gNdsStageMPCliffAttackActionLoopIsCliffHoldAfterQuick2 =
        (fp->is_cliff_hold != FALSE) ? 1u : 0u;
    gNdsStageMPCliffAttackActionLoopProcDamageSetAfterQuick2 =
        (fp->proc_damage != NULL) ? 1u : 0u;
    gNdsStageMPCliffAttackActionLoopProcMapSetAfterQuick2 =
        (fp->proc_map == ftCommonCliffAttackEscape2ProcMap) ? 1u : 0u;
    gNdsStageMPCliffAttackActionLoopJostleIgnoreAfterQuick2 =
        (fp->is_jostle_ignore != FALSE) ? 1u : 0u;
}

void ndsFighterMarioFoxStageMPCliffAttackActionLoopFinalize(void)
{
    u32 mask = 0u;

    if ((ndsFighterMarioFoxStageMPCliffAttackActionLoopProofEnabled() ==
            FALSE) ||
        (gNdsStageMPCliffAttackActionLoopPrepared == 0u) ||
        (gNdsFighterMarioFoxStageMPCliffAttackActionLoopResult != 0u))
    {
        return;
    }

    if (gNdsFighterMarioFoxStageMPCliffAttackFloorLoopResult == 0u)
    {
        ndsFighterMarioFoxStageMPCliffAttackFloorLoopFinalize();
    }
    if ((gNdsFighterMarioFoxStageMPCliffAttackFloorLoopResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFATTACK_FLOOR_LOOP_PASS) &&
        (gNdsFighterMarioFoxStageMPCliffAttackFloorLoopSafeResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFATTACK_FLOOR_LOOP_SAFE_PASS))
    {
        gNdsStageMPCliffAttackActionLoopBaseMPCliffAttackSeen = 1u;
        mask |= 1u << 0;
    }
    if ((gNdsStageMPCliffAttackActionLoopPrepared != 0u) &&
        (gNdsStageMPCliffAttackActionLoopBaseMPCliffAttackSeen != 0u))
    {
        mask |= 1u << 1;
    }

    if ((gNdsStageMPCliffAttackActionLoopQuickUpdateCallCount == 0u) &&
        (gNdsStageMPCliffAttackActionLoopUnsafeCount == 0u))
    {
        ndsStageMPCliffAttackActionLoopRunProbe();
    }

    if ((gNdsStageMPCliffAttackActionLoopStatusBefore ==
            (u32)nFTCommonStatusCliffQuick) &&
        (gNdsStageMPCliffAttackActionLoopMotionBefore ==
            (u32)nFTCommonMotionCliffQuick) &&
        (gNdsStageMPCliffAttackActionLoopGABefore ==
            (u32)nMPKineticsGround) &&
        (gNdsStageMPCliffAttackActionLoopCliffIDBefore >= 0) &&
        (gNdsStageMPCliffAttackActionLoopQueuedStatusID ==
            (u32)nFTCommonCliffKindAttackQuick) &&
        (gNdsStageMPCliffAttackActionLoopQueuedCliffID ==
            gNdsStageMPCliffAttackActionLoopCliffIDBefore))
    {
        mask |= 1u << 2;
    }
    if ((gNdsStageMPCliffAttackActionLoopQuickUpdateCallCount == 1u) &&
        (gNdsStageMPCliffAttackActionLoopQuick1SetStatusCount == 1u))
    {
        mask |= 1u << 3;
    }
    if ((gNdsStageMPCliffAttackActionLoopStatusAfterQuick1 ==
            (u32)nFTCommonStatusCliffAttackQuick1) &&
        (gNdsStageMPCliffAttackActionLoopMotionAfterQuick1 ==
            (u32)nFTCommonMotionCliffAttackQuick1) &&
        (gNdsStageMPCliffAttackActionLoopGAAfterQuick1 ==
            (u32)nMPKineticsGround) &&
        (gNdsStageMPCliffAttackActionLoopCliffIDAfterQuick1 ==
            gNdsStageMPCliffAttackActionLoopCliffIDBefore))
    {
        mask |= 1u << 4;
    }
    if ((gNdsStageMPCliffAttackActionLoopQuick1UpdateCallCount == 1u) &&
        (gNdsStageMPCliffAttackActionLoopAnimEndCheckCount == 1u) &&
        (gNdsStageMPCliffAttackActionLoopQuick2SetStatusCount == 1u))
    {
        mask |= 1u << 5;
    }
    if ((gNdsStageMPCliffAttackActionLoopCommon2UpdateCollCount == 1u) &&
        (gNdsStageMPCliffAttackActionLoopCommon2InitVarsCount == 1u))
    {
        mask |= 1u << 6;
    }
    if ((gNdsStageMPCliffAttackActionLoopStatusAfterQuick2 ==
            (u32)nFTCommonStatusCliffAttackQuick2) &&
        (gNdsStageMPCliffAttackActionLoopMotionAfterQuick2 ==
            (u32)nFTCommonMotionCliffAttackQuick2) &&
        (gNdsStageMPCliffAttackActionLoopGAAfterQuick2 ==
            (u32)nMPKineticsGround) &&
        (gNdsStageMPCliffAttackActionLoopProcMapSetAfterQuick2 == 1u))
    {
        mask |= 1u << 7;
    }
    if ((gNdsStageMPCliffAttackActionLoopCliffIDAfterQuick2 ==
            gNdsStageMPCliffAttackActionLoopCliffIDBefore) &&
        (gNdsStageMPCliffAttackActionLoopFloorLineAfterQuick2 ==
            gNdsStageMPCliffAttackActionLoopCliffIDBefore))
    {
        mask |= 1u << 8;
    }
    if ((gNdsStageMPCliffAttackActionLoopIsCliffHoldAfterQuick1 == 1u) &&
        (gNdsStageMPCliffAttackActionLoopIsCliffHoldAfterQuick2 == 0u) &&
        (gNdsStageMPCliffAttackActionLoopProcDamageSetAfterQuick1 == 1u) &&
        (gNdsStageMPCliffAttackActionLoopProcDamageSetAfterQuick2 == 0u) &&
        (gNdsStageMPCliffAttackActionLoopProcUpdateSetAfterQuick1 == 1u) &&
        (gNdsStageMPCliffAttackActionLoopJostleIgnoreAfterQuick2 == 1u))
    {
        mask |= 1u << 9;
    }
    if ((gNdsStageMPCliffAttackActionLoopUnsafeCount == 0u) &&
        (sNdsStageMPCliffAttackActionLoopQuickUpdateActive == FALSE) &&
        (sNdsStageMPCliffAttackActionLoopSetStatusActive == FALSE) &&
        (sNdsStageMPCliffAttackActionLoopAnimEndActive == FALSE))
    {
        mask |= 1u << 10;
    }
    if ((gNdsStageMPCliffCommon2BridgeCallCount == 1u) &&
        (gNdsStageMPCliffCommon2BridgeGuardPassCount == 1u) &&
        (gNdsStageMPCliffCommon2BridgeGuardRejectCount == 0u) &&
        (gNdsStageMPCliffCommon2BridgeStatusID ==
            nFTCommonCliffKindAttackQuick) &&
        (gNdsStageMPCliffCommon2BridgeRootPositionOK == 1u))
    {
        mask |= 1u << 11;
    }

    gNdsFighterMarioFoxStageMPCliffAttackActionLoopCount = 1u;
    gNdsFighterMarioFoxStageMPCliffAttackActionLoopMask = mask;
    gNdsFighterMarioFoxStageMPCliffAttackActionLoopDeferredMask = 0xffu;
    if ((mask & 0xfffu) == 0xfffu)
    {
        gNdsFighterMarioFoxStageMPCliffAttackActionLoopResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFATTACK_ACTION_LOOP_PASS;
        gNdsFighterMarioFoxStageMPCliffAttackActionLoopSafeResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFATTACK_ACTION_LOOP_SAFE_PASS;
    }
}

static void ndsFighterMarioFoxStageMPCliffCommon2LoopReset(void)
{
    gNdsFighterMarioFoxStageMPCliffCommon2LoopResult = 0u;
    gNdsFighterMarioFoxStageMPCliffCommon2LoopSafeResult = 0u;
    gNdsFighterMarioFoxStageMPCliffCommon2LoopMask = 0u;
    gNdsFighterMarioFoxStageMPCliffCommon2LoopDeferredMask = 0u;
    gNdsFighterMarioFoxStageMPCliffCommon2LoopCount = 0u;
    gNdsStageMPCliffCommon2LoopPrepared = 0u;
    gNdsStageMPCliffCommon2LoopBaseMPCliffAttackActionSeen = 0u;
    gNdsStageMPCliffCommon2LoopUpdateCallCount = 0u;
    gNdsStageMPCliffCommon2LoopAnimEndCheckCount = 0u;
    gNdsStageMPCliffCommon2LoopWaitOrFallCallCount = 0u;
    gNdsStageMPCliffCommon2LoopPhysicsCallCount = 0u;
    gNdsStageMPCliffCommon2LoopGroundTransCount = 0u;
    gNdsStageMPCliffCommon2LoopMapCallCount = 0u;
    gNdsStageMPCliffCommon2LoopEdgeBreakCount = 0u;
    gNdsStageMPCliffCommon2LoopUnsafeCount = 0u;
    gNdsStageMPCliffCommon2LoopStatusBefore = 0u;
    gNdsStageMPCliffCommon2LoopMotionBefore = 0u;
    gNdsStageMPCliffCommon2LoopGABefore = 0u;
    gNdsStageMPCliffCommon2LoopStatusAfterUpdate = 0u;
    gNdsStageMPCliffCommon2LoopMotionAfterUpdate = 0u;
    gNdsStageMPCliffCommon2LoopGAAfterUpdate = 0u;
    gNdsStageMPCliffCommon2LoopStatusAfterPhysics = 0u;
    gNdsStageMPCliffCommon2LoopMotionAfterPhysics = 0u;
    gNdsStageMPCliffCommon2LoopGAAfterPhysics = 0u;
    gNdsStageMPCliffCommon2LoopStatusAfterMap = 0u;
    gNdsStageMPCliffCommon2LoopMotionAfterMap = 0u;
    gNdsStageMPCliffCommon2LoopGAAfterMap = 0u;
    gNdsStageMPCliffCommon2LoopCliffIDBefore = -1;
    gNdsStageMPCliffCommon2LoopFloorLineBefore = -1;
    gNdsStageMPCliffCommon2LoopCliffIDAfterMap = -1;
    gNdsStageMPCliffCommon2LoopFloorLineAfterMap = -1;
    gNdsStageMPCliffCommon2LoopProcUpdateSet = 0u;
    gNdsStageMPCliffCommon2LoopProcPhysicsSet = 0u;
    gNdsStageMPCliffCommon2LoopProcMapSet = 0u;
    gNdsStageMPCliffCommon2LoopIsCliffHoldAfterMap = 0u;
    sNdsStageMPCliffCommon2LoopUpdateActive = FALSE;
    sNdsStageMPCliffCommon2LoopPhysicsActive = FALSE;
    sNdsStageMPCliffCommon2LoopMapActive = FALSE;
}

void ndsFighterMarioFoxStageMPCliffCommon2LoopPrepare(void)
{
    if ((ndsFighterMarioFoxStageMPCliffCommon2LoopProofEnabled() == FALSE) ||
        (gNdsStageMPCliffCommon2LoopPrepared != 0u))
    {
        return;
    }
    ndsFighterMarioFoxStageMPCliffCommon2LoopReset();
    gNdsStageMPCliffCommon2LoopPrepared = 1u;
    gNdsStageMPCliffCommon2LoopBaseMPCliffAttackActionSeen =
        (gNdsStageMPCliffAttackActionLoopPrepared != 0u) ? 1u : 0u;
}

static void ndsStageMPCliffCommon2LoopRunProbe(void)
{
    FTStruct *fp = &sNdsFighterStructPool[1];
    GObj *fighter_gobj;

    if ((ndsFighterStructIsPoolPointer(fp) == FALSE) ||
        (fp->fighter_gobj == NULL) ||
        (fp->status_id != nFTCommonStatusCliffAttackQuick2) ||
        (fp->motion_id != nFTCommonMotionCliffAttackQuick2) ||
        (fp->ga != nMPKineticsGround) ||
        (fp->coll_data.cliff_id < 0) ||
        (fp->coll_data.floor_line_id != fp->coll_data.cliff_id))
    {
        gNdsStageMPCliffCommon2LoopUnsafeCount++;
        return;
    }

    fighter_gobj = fp->fighter_gobj;
    gNdsStageMPCliffCommon2LoopProcUpdateSet =
        (fp->proc_update == ftCommonCliffCommon2ProcUpdate) ? 1u : 0u;
    gNdsStageMPCliffCommon2LoopProcPhysicsSet =
        (fp->proc_physics == ftCommonCliffCommon2ProcPhysics) ? 1u : 0u;
    gNdsStageMPCliffCommon2LoopProcMapSet =
        (fp->proc_map == ftCommonCliffAttackEscape2ProcMap) ? 1u : 0u;
    if ((gNdsStageMPCliffCommon2LoopProcUpdateSet == 0u) ||
        (gNdsStageMPCliffCommon2LoopProcPhysicsSet == 0u) ||
        (gNdsStageMPCliffCommon2LoopProcMapSet == 0u))
    {
        gNdsStageMPCliffCommon2LoopUnsafeCount++;
        return;
    }

    fighter_gobj->anim_frame = 1.0F;
    fp->anim_frame = 1.0F;

    gNdsStageMPCliffCommon2LoopStatusBefore = (u32)fp->status_id;
    gNdsStageMPCliffCommon2LoopMotionBefore = (u32)fp->motion_id;
    gNdsStageMPCliffCommon2LoopGABefore = (u32)fp->ga;
    gNdsStageMPCliffCommon2LoopCliffIDBefore = fp->coll_data.cliff_id;
    gNdsStageMPCliffCommon2LoopFloorLineBefore =
        fp->coll_data.floor_line_id;

    gNdsStageMPCliffCommon2LoopUpdateCallCount++;
    sNdsStageMPCliffCommon2LoopUpdateActive = TRUE;
    fp->proc_update(fighter_gobj);
    sNdsStageMPCliffCommon2LoopUpdateActive = FALSE;
    gNdsStageMPCliffCommon2LoopStatusAfterUpdate = (u32)fp->status_id;
    gNdsStageMPCliffCommon2LoopMotionAfterUpdate = (u32)fp->motion_id;
    gNdsStageMPCliffCommon2LoopGAAfterUpdate = (u32)fp->ga;

    gNdsStageMPCliffCommon2LoopPhysicsCallCount++;
    sNdsStageMPCliffCommon2LoopPhysicsActive = TRUE;
    fp->proc_physics(fighter_gobj);
    if (gNdsStageMPCliffCommon2LoopGroundTransCount == 0u)
    {
        ftPhysicsApplyGroundVelTransN(fighter_gobj);
    }
    sNdsStageMPCliffCommon2LoopPhysicsActive = FALSE;
    gNdsStageMPCliffCommon2LoopStatusAfterPhysics = (u32)fp->status_id;
    gNdsStageMPCliffCommon2LoopMotionAfterPhysics = (u32)fp->motion_id;
    gNdsStageMPCliffCommon2LoopGAAfterPhysics = (u32)fp->ga;

    gNdsStageMPCliffCommon2LoopMapCallCount++;
    sNdsStageMPCliffCommon2LoopMapActive = TRUE;
    fp->proc_map(fighter_gobj);
    if (gNdsStageMPCliffCommon2LoopEdgeBreakCount == 0u)
    {
        mpCommonSetFighterFallOnEdgeBreak(fighter_gobj);
    }
    sNdsStageMPCliffCommon2LoopMapActive = FALSE;
    gNdsStageMPCliffCommon2LoopStatusAfterMap = (u32)fp->status_id;
    gNdsStageMPCliffCommon2LoopMotionAfterMap = (u32)fp->motion_id;
    gNdsStageMPCliffCommon2LoopGAAfterMap = (u32)fp->ga;
    gNdsStageMPCliffCommon2LoopCliffIDAfterMap = fp->coll_data.cliff_id;
    gNdsStageMPCliffCommon2LoopFloorLineAfterMap =
        fp->coll_data.floor_line_id;
    gNdsStageMPCliffCommon2LoopIsCliffHoldAfterMap =
        (fp->is_cliff_hold != FALSE) ? 1u : 0u;
}

void ndsFighterMarioFoxStageMPCliffCommon2LoopFinalize(void)
{
    u32 mask = 0u;

    if ((ndsFighterMarioFoxStageMPCliffCommon2LoopProofEnabled() == FALSE) ||
        (gNdsStageMPCliffCommon2LoopPrepared == 0u) ||
        (gNdsFighterMarioFoxStageMPCliffCommon2LoopResult != 0u))
    {
        return;
    }

    if (gNdsFighterMarioFoxStageMPCliffAttackActionLoopResult == 0u)
    {
        ndsFighterMarioFoxStageMPCliffAttackActionLoopFinalize();
    }
    if ((gNdsFighterMarioFoxStageMPCliffAttackActionLoopResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFATTACK_ACTION_LOOP_PASS) &&
        (gNdsFighterMarioFoxStageMPCliffAttackActionLoopSafeResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFATTACK_ACTION_LOOP_SAFE_PASS))
    {
        gNdsStageMPCliffCommon2LoopBaseMPCliffAttackActionSeen = 1u;
        mask |= 1u << 0;
    }
    if ((gNdsStageMPCliffCommon2LoopPrepared != 0u) &&
        (gNdsStageMPCliffCommon2LoopBaseMPCliffAttackActionSeen != 0u))
    {
        mask |= 1u << 1;
    }

    if ((gNdsStageMPCliffCommon2LoopUpdateCallCount == 0u) &&
        (gNdsStageMPCliffCommon2LoopUnsafeCount == 0u))
    {
        ndsStageMPCliffCommon2LoopRunProbe();
    }

    if ((gNdsStageMPCliffCommon2LoopStatusBefore ==
            (u32)nFTCommonStatusCliffAttackQuick2) &&
        (gNdsStageMPCliffCommon2LoopMotionBefore ==
            (u32)nFTCommonMotionCliffAttackQuick2) &&
        (gNdsStageMPCliffCommon2LoopGABefore ==
            (u32)nMPKineticsGround) &&
        (gNdsStageMPCliffCommon2LoopCliffIDBefore >= 0) &&
        (gNdsStageMPCliffCommon2LoopFloorLineBefore ==
            gNdsStageMPCliffCommon2LoopCliffIDBefore))
    {
        mask |= 1u << 2;
    }
    if ((gNdsStageMPCliffCommon2LoopProcUpdateSet == 1u) &&
        (gNdsStageMPCliffCommon2LoopProcPhysicsSet == 1u) &&
        (gNdsStageMPCliffCommon2LoopProcMapSet == 1u))
    {
        mask |= 1u << 3;
    }
    if ((gNdsStageMPCliffCommon2LoopUpdateCallCount == 1u) &&
        (gNdsStageMPCliffCommon2LoopAnimEndCheckCount == 1u) &&
        (gNdsStageMPCliffCommon2LoopWaitOrFallCallCount == 0u) &&
        (gNdsStageMPCliffCommon2LoopStatusAfterUpdate ==
            (u32)nFTCommonStatusCliffAttackQuick2) &&
        (gNdsStageMPCliffCommon2LoopMotionAfterUpdate ==
            (u32)nFTCommonMotionCliffAttackQuick2) &&
        (gNdsStageMPCliffCommon2LoopGAAfterUpdate ==
            (u32)nMPKineticsGround))
    {
        mask |= 1u << 4;
    }
    if ((gNdsStageMPCliffCommon2LoopPhysicsCallCount == 1u) &&
        (gNdsStageMPCliffCommon2LoopGroundTransCount == 1u) &&
        (gNdsStageMPCliffCommon2LoopStatusAfterPhysics ==
            (u32)nFTCommonStatusCliffAttackQuick2) &&
        (gNdsStageMPCliffCommon2LoopMotionAfterPhysics ==
            (u32)nFTCommonMotionCliffAttackQuick2) &&
        (gNdsStageMPCliffCommon2LoopGAAfterPhysics ==
            (u32)nMPKineticsGround))
    {
        mask |= 1u << 5;
    }
    if ((gNdsStageMPCliffCommon2LoopMapCallCount == 1u) &&
        (gNdsStageMPCliffCommon2LoopEdgeBreakCount == 1u) &&
        (gNdsStageMPCliffCommon2LoopStatusAfterMap ==
            (u32)nFTCommonStatusCliffAttackQuick2) &&
        (gNdsStageMPCliffCommon2LoopMotionAfterMap ==
            (u32)nFTCommonMotionCliffAttackQuick2) &&
        (gNdsStageMPCliffCommon2LoopGAAfterMap ==
            (u32)nMPKineticsGround))
    {
        mask |= 1u << 6;
    }
    if ((gNdsStageMPCliffCommon2LoopCliffIDAfterMap ==
            gNdsStageMPCliffCommon2LoopCliffIDBefore) &&
        (gNdsStageMPCliffCommon2LoopFloorLineAfterMap ==
            gNdsStageMPCliffCommon2LoopFloorLineBefore))
    {
        mask |= 1u << 7;
    }
    if ((gNdsStageMPCliffCommon2LoopIsCliffHoldAfterMap == 0u) &&
        (gNdsStageMPCliffCommon2LoopUnsafeCount == 0u) &&
        (sNdsStageMPCliffCommon2LoopUpdateActive == FALSE) &&
        (sNdsStageMPCliffCommon2LoopPhysicsActive == FALSE) &&
        (sNdsStageMPCliffCommon2LoopMapActive == FALSE))
    {
        mask |= 1u << 8;
    }

    gNdsFighterMarioFoxStageMPCliffCommon2LoopCount = 1u;
    gNdsFighterMarioFoxStageMPCliffCommon2LoopMask = mask;
    gNdsFighterMarioFoxStageMPCliffCommon2LoopDeferredMask = 0xffu;
    if ((mask & 0x1ffu) == 0x1ffu)
    {
        gNdsFighterMarioFoxStageMPCliffCommon2LoopResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFCOMMON2_LOOP_PASS;
        gNdsFighterMarioFoxStageMPCliffCommon2LoopSafeResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFCOMMON2_LOOP_SAFE_PASS;
    }
}

static void ndsFighterMarioFoxStageMPCliffEscapeActionLoopReset(void)
{
    ndsStageMPCliffCommon2BridgeResetDiagnostics();
    gNdsFighterMarioFoxStageMPCliffEscapeActionLoopResult = 0u;
    gNdsFighterMarioFoxStageMPCliffEscapeActionLoopSafeResult = 0u;
    gNdsFighterMarioFoxStageMPCliffEscapeActionLoopMask = 0u;
    gNdsFighterMarioFoxStageMPCliffEscapeActionLoopDeferredMask = 0u;
    gNdsFighterMarioFoxStageMPCliffEscapeActionLoopCount = 0u;
    gNdsStageMPCliffEscapeActionLoopPrepared = 0u;
    gNdsStageMPCliffEscapeActionLoopBaseMPCliffWaitSeen = 0u;
    gNdsStageMPCliffEscapeActionLoopInterruptCallCount = 0u;
    gNdsStageMPCliffEscapeActionLoopAttackCheckCount = 0u;
    gNdsStageMPCliffEscapeActionLoopEscapeCheckCount = 0u;
    gNdsStageMPCliffEscapeActionLoopClimbOrFallCheckCount = 0u;
    gNdsStageMPCliffEscapeActionLoopQuickStatusSetCount = 0u;
    gNdsStageMPCliffEscapeActionLoopAnimEventsCount = 0u;
    gNdsStageMPCliffEscapeActionLoopQuickUpdateCallCount = 0u;
    gNdsStageMPCliffEscapeActionLoopQuick1SetStatusCount = 0u;
    gNdsStageMPCliffEscapeActionLoopQuick1UpdateCallCount = 0u;
    gNdsStageMPCliffEscapeActionLoopAnimEndCheckCount = 0u;
    gNdsStageMPCliffEscapeActionLoopQuick2SetStatusCount = 0u;
    gNdsStageMPCliffEscapeActionLoopCommon2UpdateCollCount = 0u;
    gNdsStageMPCliffEscapeActionLoopCommon2InitVarsCount = 0u;
    gNdsStageMPCliffEscapeActionLoopUnsafeCount = 0u;
    gNdsStageMPCliffEscapeActionLoopStatusBefore = 0u;
    gNdsStageMPCliffEscapeActionLoopMotionBefore = 0u;
    gNdsStageMPCliffEscapeActionLoopGABefore = 0u;
    gNdsStageMPCliffEscapeActionLoopStatusAfterInterrupt = 0u;
    gNdsStageMPCliffEscapeActionLoopMotionAfterInterrupt = 0u;
    gNdsStageMPCliffEscapeActionLoopGAAfterInterrupt = 0u;
    gNdsStageMPCliffEscapeActionLoopStatusAfterQuick1 = 0u;
    gNdsStageMPCliffEscapeActionLoopMotionAfterQuick1 = 0u;
    gNdsStageMPCliffEscapeActionLoopGAAfterQuick1 = 0u;
    gNdsStageMPCliffEscapeActionLoopStatusAfterQuick2 = 0u;
    gNdsStageMPCliffEscapeActionLoopMotionAfterQuick2 = 0u;
    gNdsStageMPCliffEscapeActionLoopGAAfterQuick2 = 0u;
    gNdsStageMPCliffEscapeActionLoopCliffIDBefore = -1;
    gNdsStageMPCliffEscapeActionLoopCliffIDAfterInterrupt = -1;
    gNdsStageMPCliffEscapeActionLoopCliffIDAfterQuick1 = -1;
    gNdsStageMPCliffEscapeActionLoopCliffIDAfterQuick2 = -1;
    gNdsStageMPCliffEscapeActionLoopFloorLineAfterQuick2 = -1;
    gNdsStageMPCliffEscapeActionLoopQueuedStatusID = 0u;
    gNdsStageMPCliffEscapeActionLoopQueuedCliffID = -1;
    gNdsStageMPCliffEscapeActionLoopIsCliffHoldAfterInterrupt = 0u;
    gNdsStageMPCliffEscapeActionLoopIsCliffHoldAfterQuick1 = 0u;
    gNdsStageMPCliffEscapeActionLoopIsCliffHoldAfterQuick2 = 0u;
    gNdsStageMPCliffEscapeActionLoopAllowInterruptBefore = 0u;
    gNdsStageMPCliffEscapeActionLoopAllowInterruptAfter = 0u;
    gNdsStageMPCliffEscapeActionLoopButtonTapMask = 0u;
    gNdsStageMPCliffEscapeActionLoopButtonMaskZ = 0u;
    gNdsStageMPCliffEscapeActionLoopButtonMaskA = 0u;
    gNdsStageMPCliffEscapeActionLoopProcDamageSetAfterInterrupt = 0u;
    gNdsStageMPCliffEscapeActionLoopProcDamageSetAfterQuick1 = 0u;
    gNdsStageMPCliffEscapeActionLoopProcDamageSetAfterQuick2 = 0u;
    gNdsStageMPCliffEscapeActionLoopProcUpdateSetAfterInterrupt = 0u;
    gNdsStageMPCliffEscapeActionLoopProcUpdateSetAfterQuick1 = 0u;
    gNdsStageMPCliffEscapeActionLoopProcMapSetAfterQuick2 = 0u;
    gNdsStageMPCliffEscapeActionLoopJostleIgnoreAfterQuick2 = 0u;
    gNdsStageMPCliffEscapeActionLoopFallWaitBefore = 0;
    gNdsStageMPCliffEscapeActionLoopFallWaitAfterInterrupt = 0;
    gNdsStageMPCliffEscapeActionLoopDamageFallCallCount = 0u;
    sNdsStageMPCliffEscapeActionLoopInterruptActive = FALSE;
    sNdsStageMPCliffEscapeActionLoopQuickUpdateActive = FALSE;
    sNdsStageMPCliffEscapeActionLoopSetStatusActive = FALSE;
    sNdsStageMPCliffEscapeActionLoopAnimEndActive = FALSE;
}

void ndsFighterMarioFoxStageMPCliffEscapeActionLoopPrepare(void)
{
    if ((ndsFighterMarioFoxStageMPCliffEscapeActionLoopProofEnabled() ==
            FALSE) ||
        (gNdsStageMPCliffEscapeActionLoopPrepared != 0u))
    {
        return;
    }
    ndsFighterMarioFoxStageMPCliffEscapeActionLoopReset();
    gNdsStageMPCliffEscapeActionLoopPrepared = 1u;
    gNdsStageMPCliffEscapeActionLoopBaseMPCliffWaitSeen =
        (gNdsStageMPCliffWaitFloorLoopPrepared != 0u) ? 1u : 0u;
}

static void ndsStageMPCliffEscapeActionLoopRunProbe(void)
{
    FTStruct *fp = &sNdsFighterStructPool[1];
    GObj *fighter_gobj;

    if ((ndsFighterStructIsPoolPointer(fp) == FALSE) ||
        (fp->fighter_gobj == NULL))
    {
        gNdsStageMPCliffEscapeActionLoopUnsafeCount++;
        return;
    }

    fighter_gobj = fp->fighter_gobj;
#if NDS_MARIOFOX_STAGE_MPPASSIVE_RECOVER_LOOP_HARNESS
    if ((fp->status_id != nFTCommonStatusCliffWait) ||
        (fp->motion_id != nFTCommonMotionCliffWait) ||
        (fp->ga != nMPKineticsGround) ||
        (fp->coll_data.cliff_id < 0))
    {
        DObj *root = fp->joints[nFTPartsJointTopN];
        s32 cliff_id = gNdsStageMPCliffWaitFloorLoopCliffIDAfterUpdate;

        if (cliff_id < 0)
        {
            cliff_id = gNdsStageMPCliffLiveLoopCliffID;
        }
        if (cliff_id < 0)
        {
            cliff_id = gNdsStageMPCliffCatchFloorLoopCliffIDAfter;
        }
        if ((root != NULL) && (cliff_id >= 0))
        {
            /* ponytail: reseed only the delayed aggregate probe. */
            ndsStageMPCliffLiveLoopPrimeCliffState(fp, fighter_gobj, root,
                                                   cliff_id, FALSE);
        }
    }
#endif

    if ((fp->status_id != nFTCommonStatusCliffWait) ||
        (fp->motion_id != nFTCommonMotionCliffWait) ||
        (fp->ga != nMPKineticsGround) ||
        (fp->coll_data.cliff_id < 0))
    {
        gNdsStageMPCliffEscapeActionLoopUnsafeCount++;
        return;
    }

    fp->input.pl.button_hold = 0u;
    fp->input.pl.button_release = 0u;
    fp->input.pl.button_tap = fp->input.button_mask_z;
    fp->input.pl.stick_range.x = 0;
    fp->input.pl.stick_range.y = 0;

    gNdsStageMPCliffEscapeActionLoopStatusBefore = (u32)fp->status_id;
    gNdsStageMPCliffEscapeActionLoopMotionBefore = (u32)fp->motion_id;
    gNdsStageMPCliffEscapeActionLoopGABefore = (u32)fp->ga;
    gNdsStageMPCliffEscapeActionLoopCliffIDBefore =
        fp->coll_data.cliff_id;
    gNdsStageMPCliffEscapeActionLoopAllowInterruptBefore =
        (fp->status_vars.common.cliffwait.is_allow_interrupt != FALSE) ?
            1u : 0u;
    gNdsStageMPCliffEscapeActionLoopButtonTapMask =
        (u32)fp->input.pl.button_tap;
    gNdsStageMPCliffEscapeActionLoopButtonMaskZ =
        (u32)fp->input.button_mask_z;
    gNdsStageMPCliffEscapeActionLoopButtonMaskA =
        (u32)fp->input.button_mask_a;
    gNdsStageMPCliffEscapeActionLoopFallWaitBefore =
        fp->status_vars.common.cliffwait.fall_wait;

    gNdsStageMPCliffEscapeActionLoopInterruptCallCount++;
    sNdsStageMPCliffEscapeActionLoopInterruptActive = TRUE;
    sNdsStageMPCliffEscapeActionLoopSetStatusActive = TRUE;
    ftCommonCliffWaitProcInterrupt(fighter_gobj);
    sNdsStageMPCliffEscapeActionLoopSetStatusActive = FALSE;
    sNdsStageMPCliffEscapeActionLoopInterruptActive = FALSE;

    gNdsStageMPCliffEscapeActionLoopStatusAfterInterrupt =
        (u32)fp->status_id;
    gNdsStageMPCliffEscapeActionLoopMotionAfterInterrupt =
        (u32)fp->motion_id;
    gNdsStageMPCliffEscapeActionLoopGAAfterInterrupt = (u32)fp->ga;
    gNdsStageMPCliffEscapeActionLoopCliffIDAfterInterrupt =
        fp->coll_data.cliff_id;
    gNdsStageMPCliffEscapeActionLoopQueuedStatusID =
        (u32)fp->status_vars.common.cliffmotion.status_id;
    gNdsStageMPCliffEscapeActionLoopQueuedCliffID =
        fp->status_vars.common.cliffmotion.cliff_id;
    gNdsStageMPCliffEscapeActionLoopIsCliffHoldAfterInterrupt =
        (fp->is_cliff_hold != FALSE) ? 1u : 0u;
    gNdsStageMPCliffEscapeActionLoopAllowInterruptAfter =
        (fp->status_vars.common.cliffwait.is_allow_interrupt != FALSE) ?
            1u : 0u;
    gNdsStageMPCliffEscapeActionLoopProcDamageSetAfterInterrupt =
        (fp->proc_damage == ftCommonCliffCommonProcDamage) ? 1u : 0u;
    gNdsStageMPCliffEscapeActionLoopProcUpdateSetAfterInterrupt =
        (fp->proc_update == ftCommonCliffQuickProcUpdate) ? 1u : 0u;
    gNdsStageMPCliffEscapeActionLoopFallWaitAfterInterrupt =
        fp->status_vars.common.cliffwait.fall_wait;

    if ((fp->status_id != nFTCommonStatusCliffQuick) ||
        (fp->motion_id != nFTCommonMotionCliffQuick) ||
        (fp->proc_update != ftCommonCliffQuickProcUpdate) ||
        (fp->status_vars.common.cliffmotion.status_id !=
            nFTCommonCliffKindEscapeQuick))
    {
        gNdsStageMPCliffEscapeActionLoopUnsafeCount++;
        return;
    }

    fighter_gobj->anim_frame = 0.0F;
    fp->anim_frame = 0.0F;

    ndsStageMPCliffCommon2BridgeResetDiagnostics();
    gNdsStageMPCliffEscapeActionLoopQuickUpdateCallCount++;
    sNdsStageMPCliffEscapeActionLoopQuickUpdateActive = TRUE;
    sNdsStageMPCliffEscapeActionLoopSetStatusActive = TRUE;
    ftCommonCliffQuickProcUpdate(fighter_gobj);
    sNdsStageMPCliffEscapeActionLoopSetStatusActive = FALSE;
    sNdsStageMPCliffEscapeActionLoopQuickUpdateActive = FALSE;

    gNdsStageMPCliffEscapeActionLoopStatusAfterQuick1 =
        (u32)fp->status_id;
    gNdsStageMPCliffEscapeActionLoopMotionAfterQuick1 =
        (u32)fp->motion_id;
    gNdsStageMPCliffEscapeActionLoopGAAfterQuick1 = (u32)fp->ga;
    gNdsStageMPCliffEscapeActionLoopCliffIDAfterQuick1 =
        fp->coll_data.cliff_id;
    gNdsStageMPCliffEscapeActionLoopIsCliffHoldAfterQuick1 =
        (fp->is_cliff_hold != FALSE) ? 1u : 0u;
    gNdsStageMPCliffEscapeActionLoopProcDamageSetAfterQuick1 =
        (fp->proc_damage == ftCommonCliffCommonProcDamage) ? 1u : 0u;
    gNdsStageMPCliffEscapeActionLoopProcUpdateSetAfterQuick1 =
        (fp->proc_update == ftCommonCliffEscapeQuick1ProcUpdate) ? 1u : 0u;

    if ((fp->status_id != nFTCommonStatusCliffEscapeQuick1) ||
        (fp->motion_id != nFTCommonMotionCliffEscapeQuick1) ||
        (fp->proc_update != ftCommonCliffEscapeQuick1ProcUpdate))
    {
        gNdsStageMPCliffEscapeActionLoopUnsafeCount++;
        return;
    }

    fighter_gobj->anim_frame = 0.0F;
    fp->anim_frame = 0.0F;

    gNdsStageMPCliffEscapeActionLoopQuick1UpdateCallCount++;
    sNdsStageMPCliffEscapeActionLoopAnimEndActive = TRUE;
    fp->proc_update(fighter_gobj);
    sNdsStageMPCliffEscapeActionLoopAnimEndActive = FALSE;

    gNdsStageMPCliffEscapeActionLoopStatusAfterQuick2 =
        (u32)fp->status_id;
    gNdsStageMPCliffEscapeActionLoopMotionAfterQuick2 =
        (u32)fp->motion_id;
    gNdsStageMPCliffEscapeActionLoopGAAfterQuick2 = (u32)fp->ga;
    gNdsStageMPCliffEscapeActionLoopCliffIDAfterQuick2 =
        fp->coll_data.cliff_id;
    gNdsStageMPCliffEscapeActionLoopFloorLineAfterQuick2 =
        fp->coll_data.floor_line_id;
    gNdsStageMPCliffEscapeActionLoopIsCliffHoldAfterQuick2 =
        (fp->is_cliff_hold != FALSE) ? 1u : 0u;
    gNdsStageMPCliffEscapeActionLoopProcDamageSetAfterQuick2 =
        (fp->proc_damage != NULL) ? 1u : 0u;
    gNdsStageMPCliffEscapeActionLoopProcMapSetAfterQuick2 =
        (fp->proc_map == ftCommonCliffAttackEscape2ProcMap) ? 1u : 0u;
    gNdsStageMPCliffEscapeActionLoopJostleIgnoreAfterQuick2 =
        (fp->is_jostle_ignore != FALSE) ? 1u : 0u;
}

void ndsFighterMarioFoxStageMPCliffEscapeActionLoopFinalize(void)
{
    u32 mask = 0u;

    if ((ndsFighterMarioFoxStageMPCliffEscapeActionLoopProofEnabled() ==
            FALSE) ||
        (gNdsStageMPCliffEscapeActionLoopPrepared == 0u) ||
        (gNdsFighterMarioFoxStageMPCliffEscapeActionLoopResult != 0u))
    {
        return;
    }

    if (gNdsFighterMarioFoxStageMPCliffWaitFloorLoopResult == 0u)
    {
        ndsFighterMarioFoxStageMPCliffWaitFloorLoopFinalize();
    }
    if ((gNdsFighterMarioFoxStageMPCliffWaitFloorLoopResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFWAIT_FLOOR_LOOP_PASS) &&
        (gNdsFighterMarioFoxStageMPCliffWaitFloorLoopSafeResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFWAIT_FLOOR_LOOP_SAFE_PASS))
    {
        gNdsStageMPCliffEscapeActionLoopBaseMPCliffWaitSeen = 1u;
        mask |= 1u << 0;
    }
    if ((gNdsStageMPCliffEscapeActionLoopPrepared != 0u) &&
        (gNdsStageMPCliffEscapeActionLoopBaseMPCliffWaitSeen != 0u))
    {
        mask |= 1u << 1;
    }

    if ((gNdsStageMPCliffEscapeActionLoopInterruptCallCount == 0u) &&
        (gNdsStageMPCliffEscapeActionLoopUnsafeCount == 0u))
    {
        ndsStageMPCliffEscapeActionLoopRunProbe();
    }

    if ((gNdsStageMPCliffEscapeActionLoopStatusBefore ==
            (u32)nFTCommonStatusCliffWait) &&
        (gNdsStageMPCliffEscapeActionLoopMotionBefore ==
            (u32)nFTCommonMotionCliffWait) &&
        (gNdsStageMPCliffEscapeActionLoopGABefore ==
            (u32)nMPKineticsGround) &&
        (gNdsStageMPCliffEscapeActionLoopCliffIDBefore >= 0))
    {
        mask |= 1u << 2;
    }
    if ((gNdsStageMPCliffEscapeActionLoopButtonMaskZ != 0u) &&
        (gNdsStageMPCliffEscapeActionLoopButtonMaskA != 0u) &&
        (gNdsStageMPCliffEscapeActionLoopButtonTapMask ==
            gNdsStageMPCliffEscapeActionLoopButtonMaskZ) &&
        (gNdsStageMPCliffEscapeActionLoopButtonTapMask !=
            gNdsStageMPCliffEscapeActionLoopButtonMaskA) &&
        (gNdsStageMPCliffEscapeActionLoopFallWaitBefore > 0))
    {
        mask |= 1u << 3;
    }
    if ((gNdsStageMPCliffEscapeActionLoopInterruptCallCount == 1u) &&
        (gNdsStageMPCliffEscapeActionLoopAttackCheckCount == 1u) &&
        (gNdsStageMPCliffEscapeActionLoopEscapeCheckCount == 1u) &&
        (gNdsStageMPCliffEscapeActionLoopClimbOrFallCheckCount == 0u) &&
        (gNdsStageMPCliffEscapeActionLoopQuickStatusSetCount == 1u) &&
        (gNdsStageMPCliffEscapeActionLoopAnimEventsCount == 1u) &&
        (gNdsStageMPCliffEscapeActionLoopDamageFallCallCount == 0u))
    {
        mask |= 1u << 4;
    }
    if ((gNdsStageMPCliffEscapeActionLoopStatusAfterInterrupt ==
            (u32)nFTCommonStatusCliffQuick) &&
        (gNdsStageMPCliffEscapeActionLoopMotionAfterInterrupt ==
            (u32)nFTCommonMotionCliffQuick) &&
        (gNdsStageMPCliffEscapeActionLoopGAAfterInterrupt ==
            (u32)nMPKineticsGround) &&
        (gNdsStageMPCliffEscapeActionLoopQueuedStatusID ==
            (u32)nFTCommonCliffKindEscapeQuick) &&
        (gNdsStageMPCliffEscapeActionLoopQueuedCliffID ==
            gNdsStageMPCliffEscapeActionLoopCliffIDBefore) &&
        (gNdsStageMPCliffEscapeActionLoopCliffIDAfterInterrupt ==
            gNdsStageMPCliffEscapeActionLoopCliffIDBefore))
    {
        mask |= 1u << 5;
    }
    if ((gNdsStageMPCliffEscapeActionLoopIsCliffHoldAfterInterrupt == 1u) &&
        (gNdsStageMPCliffEscapeActionLoopProcDamageSetAfterInterrupt == 1u) &&
        (gNdsStageMPCliffEscapeActionLoopProcUpdateSetAfterInterrupt == 1u))
    {
        mask |= 1u << 6;
    }
    if ((gNdsStageMPCliffEscapeActionLoopQuickUpdateCallCount == 1u) &&
        (gNdsStageMPCliffEscapeActionLoopQuick1SetStatusCount == 1u))
    {
        mask |= 1u << 7;
    }
    if ((gNdsStageMPCliffEscapeActionLoopStatusAfterQuick1 ==
            (u32)nFTCommonStatusCliffEscapeQuick1) &&
        (gNdsStageMPCliffEscapeActionLoopMotionAfterQuick1 ==
            (u32)nFTCommonMotionCliffEscapeQuick1) &&
        (gNdsStageMPCliffEscapeActionLoopGAAfterQuick1 ==
            (u32)nMPKineticsGround) &&
        (gNdsStageMPCliffEscapeActionLoopCliffIDAfterQuick1 ==
            gNdsStageMPCliffEscapeActionLoopCliffIDBefore) &&
        (gNdsStageMPCliffEscapeActionLoopProcUpdateSetAfterQuick1 == 1u))
    {
        mask |= 1u << 8;
    }
    if ((gNdsStageMPCliffEscapeActionLoopQuick1UpdateCallCount == 1u) &&
        (gNdsStageMPCliffEscapeActionLoopAnimEndCheckCount == 1u) &&
        (gNdsStageMPCliffEscapeActionLoopQuick2SetStatusCount == 1u))
    {
        mask |= 1u << 9;
    }
    if ((gNdsStageMPCliffEscapeActionLoopCommon2UpdateCollCount == 1u) &&
        (gNdsStageMPCliffEscapeActionLoopCommon2InitVarsCount == 1u))
    {
        mask |= 1u << 10;
    }
    if ((gNdsStageMPCliffEscapeActionLoopStatusAfterQuick2 ==
            (u32)nFTCommonStatusCliffEscapeQuick2) &&
        (gNdsStageMPCliffEscapeActionLoopMotionAfterQuick2 ==
            (u32)nFTCommonMotionCliffEscapeQuick2) &&
        (gNdsStageMPCliffEscapeActionLoopGAAfterQuick2 ==
            (u32)nMPKineticsGround) &&
        (gNdsStageMPCliffEscapeActionLoopProcMapSetAfterQuick2 == 1u) &&
        (gNdsStageMPCliffEscapeActionLoopCliffIDAfterQuick2 ==
            gNdsStageMPCliffEscapeActionLoopCliffIDBefore) &&
        (gNdsStageMPCliffEscapeActionLoopFloorLineAfterQuick2 ==
            gNdsStageMPCliffEscapeActionLoopCliffIDBefore))
    {
        mask |= 1u << 11;
    }
    if ((gNdsStageMPCliffEscapeActionLoopIsCliffHoldAfterQuick1 == 1u) &&
        (gNdsStageMPCliffEscapeActionLoopIsCliffHoldAfterQuick2 == 0u) &&
        (gNdsStageMPCliffEscapeActionLoopProcDamageSetAfterQuick1 == 1u) &&
        (gNdsStageMPCliffEscapeActionLoopProcDamageSetAfterQuick2 == 0u) &&
        (gNdsStageMPCliffEscapeActionLoopJostleIgnoreAfterQuick2 == 1u) &&
        (gNdsStageMPCliffEscapeActionLoopUnsafeCount == 0u) &&
        (sNdsStageMPCliffEscapeActionLoopInterruptActive == FALSE) &&
        (sNdsStageMPCliffEscapeActionLoopQuickUpdateActive == FALSE) &&
        (sNdsStageMPCliffEscapeActionLoopSetStatusActive == FALSE) &&
        (sNdsStageMPCliffEscapeActionLoopAnimEndActive == FALSE))
    {
        mask |= 1u << 12;
    }
    if ((gNdsStageMPCliffCommon2BridgeCallCount == 1u) &&
        (gNdsStageMPCliffCommon2BridgeGuardPassCount == 1u) &&
        (gNdsStageMPCliffCommon2BridgeGuardRejectCount == 0u) &&
        (gNdsStageMPCliffCommon2BridgeStatusID ==
            nFTCommonCliffKindEscapeQuick) &&
        (gNdsStageMPCliffCommon2BridgeRootPositionOK == 1u))
    {
        mask |= 1u << 13;
    }

    gNdsFighterMarioFoxStageMPCliffEscapeActionLoopCount = 1u;
    gNdsFighterMarioFoxStageMPCliffEscapeActionLoopMask = mask;
    gNdsFighterMarioFoxStageMPCliffEscapeActionLoopDeferredMask = 0xffu;
    if ((mask & 0x3fffu) == 0x3fffu)
    {
        gNdsFighterMarioFoxStageMPCliffEscapeActionLoopResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFESCAPE_ACTION_LOOP_PASS;
        gNdsFighterMarioFoxStageMPCliffEscapeActionLoopSafeResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFESCAPE_ACTION_LOOP_SAFE_PASS;
    }
}

static void ndsFighterMarioFoxStageMPCliffEscapeCommon2LoopReset(void)
{
    gNdsFighterMarioFoxStageMPCliffEscapeCommon2LoopResult = 0u;
    gNdsFighterMarioFoxStageMPCliffEscapeCommon2LoopSafeResult = 0u;
    gNdsFighterMarioFoxStageMPCliffEscapeCommon2LoopMask = 0u;
    gNdsFighterMarioFoxStageMPCliffEscapeCommon2LoopDeferredMask = 0u;
    gNdsFighterMarioFoxStageMPCliffEscapeCommon2LoopCount = 0u;
    gNdsStageMPCliffEscapeCommon2LoopPrepared = 0u;
    gNdsStageMPCliffEscapeCommon2LoopBaseMPCliffEscapeActionSeen = 0u;
    gNdsStageMPCliffEscapeCommon2LoopUpdateCallCount = 0u;
    gNdsStageMPCliffEscapeCommon2LoopAnimEndCheckCount = 0u;
    gNdsStageMPCliffEscapeCommon2LoopWaitOrFallCallCount = 0u;
    gNdsStageMPCliffEscapeCommon2LoopPhysicsCallCount = 0u;
    gNdsStageMPCliffEscapeCommon2LoopGroundTransCount = 0u;
    gNdsStageMPCliffEscapeCommon2LoopMapCallCount = 0u;
    gNdsStageMPCliffEscapeCommon2LoopEdgeBreakCount = 0u;
    gNdsStageMPCliffEscapeCommon2LoopUnsafeCount = 0u;
    gNdsStageMPCliffEscapeCommon2LoopStatusBefore = 0u;
    gNdsStageMPCliffEscapeCommon2LoopMotionBefore = 0u;
    gNdsStageMPCliffEscapeCommon2LoopGABefore = 0u;
    gNdsStageMPCliffEscapeCommon2LoopStatusAfterUpdate = 0u;
    gNdsStageMPCliffEscapeCommon2LoopMotionAfterUpdate = 0u;
    gNdsStageMPCliffEscapeCommon2LoopGAAfterUpdate = 0u;
    gNdsStageMPCliffEscapeCommon2LoopStatusAfterPhysics = 0u;
    gNdsStageMPCliffEscapeCommon2LoopMotionAfterPhysics = 0u;
    gNdsStageMPCliffEscapeCommon2LoopGAAfterPhysics = 0u;
    gNdsStageMPCliffEscapeCommon2LoopStatusAfterMap = 0u;
    gNdsStageMPCliffEscapeCommon2LoopMotionAfterMap = 0u;
    gNdsStageMPCliffEscapeCommon2LoopGAAfterMap = 0u;
    gNdsStageMPCliffEscapeCommon2LoopCliffIDBefore = -1;
    gNdsStageMPCliffEscapeCommon2LoopFloorLineBefore = -1;
    gNdsStageMPCliffEscapeCommon2LoopCliffIDAfterMap = -1;
    gNdsStageMPCliffEscapeCommon2LoopFloorLineAfterMap = -1;
    gNdsStageMPCliffEscapeCommon2LoopProcUpdateSet = 0u;
    gNdsStageMPCliffEscapeCommon2LoopProcPhysicsSet = 0u;
    gNdsStageMPCliffEscapeCommon2LoopProcMapSet = 0u;
    gNdsStageMPCliffEscapeCommon2LoopIsCliffHoldAfterMap = 0u;
    sNdsStageMPCliffEscapeCommon2LoopUpdateActive = FALSE;
    sNdsStageMPCliffEscapeCommon2LoopPhysicsActive = FALSE;
    sNdsStageMPCliffEscapeCommon2LoopMapActive = FALSE;
}

void ndsFighterMarioFoxStageMPCliffEscapeCommon2LoopPrepare(void)
{
    if ((ndsFighterMarioFoxStageMPCliffEscapeCommon2LoopProofEnabled() ==
            FALSE) ||
        (gNdsStageMPCliffEscapeCommon2LoopPrepared != 0u))
    {
        return;
    }
    ndsFighterMarioFoxStageMPCliffEscapeCommon2LoopReset();
    gNdsStageMPCliffEscapeCommon2LoopPrepared = 1u;
    gNdsStageMPCliffEscapeCommon2LoopBaseMPCliffEscapeActionSeen =
        (gNdsStageMPCliffEscapeActionLoopPrepared != 0u) ? 1u : 0u;
}

static void ndsStageMPCliffEscapeCommon2LoopRunProbe(void)
{
    FTStruct *fp = &sNdsFighterStructPool[1];
    GObj *fighter_gobj;

    if ((ndsFighterStructIsPoolPointer(fp) == FALSE) ||
        (fp->fighter_gobj == NULL) ||
        (fp->status_id != nFTCommonStatusCliffEscapeQuick2) ||
        (fp->motion_id != nFTCommonMotionCliffEscapeQuick2) ||
        (fp->ga != nMPKineticsGround) ||
        (fp->coll_data.cliff_id < 0) ||
        (fp->coll_data.floor_line_id != fp->coll_data.cliff_id))
    {
        gNdsStageMPCliffEscapeCommon2LoopUnsafeCount++;
        return;
    }

    fighter_gobj = fp->fighter_gobj;
    gNdsStageMPCliffEscapeCommon2LoopProcUpdateSet =
        (fp->proc_update == ftCommonCliffCommon2ProcUpdate) ? 1u : 0u;
    gNdsStageMPCliffEscapeCommon2LoopProcPhysicsSet =
        (fp->proc_physics == ftCommonCliffCommon2ProcPhysics) ? 1u : 0u;
    gNdsStageMPCliffEscapeCommon2LoopProcMapSet =
        (fp->proc_map == ftCommonCliffAttackEscape2ProcMap) ? 1u : 0u;
    if ((gNdsStageMPCliffEscapeCommon2LoopProcUpdateSet == 0u) ||
        (gNdsStageMPCliffEscapeCommon2LoopProcPhysicsSet == 0u) ||
        (gNdsStageMPCliffEscapeCommon2LoopProcMapSet == 0u))
    {
        gNdsStageMPCliffEscapeCommon2LoopUnsafeCount++;
        return;
    }

    fighter_gobj->anim_frame = 1.0F;
    fp->anim_frame = 1.0F;

    gNdsStageMPCliffEscapeCommon2LoopStatusBefore = (u32)fp->status_id;
    gNdsStageMPCliffEscapeCommon2LoopMotionBefore = (u32)fp->motion_id;
    gNdsStageMPCliffEscapeCommon2LoopGABefore = (u32)fp->ga;
    gNdsStageMPCliffEscapeCommon2LoopCliffIDBefore = fp->coll_data.cliff_id;
    gNdsStageMPCliffEscapeCommon2LoopFloorLineBefore =
        fp->coll_data.floor_line_id;

    gNdsStageMPCliffEscapeCommon2LoopUpdateCallCount++;
    sNdsStageMPCliffEscapeCommon2LoopUpdateActive = TRUE;
    fp->proc_update(fighter_gobj);
    sNdsStageMPCliffEscapeCommon2LoopUpdateActive = FALSE;
    gNdsStageMPCliffEscapeCommon2LoopStatusAfterUpdate =
        (u32)fp->status_id;
    gNdsStageMPCliffEscapeCommon2LoopMotionAfterUpdate =
        (u32)fp->motion_id;
    gNdsStageMPCliffEscapeCommon2LoopGAAfterUpdate = (u32)fp->ga;

    gNdsStageMPCliffEscapeCommon2LoopPhysicsCallCount++;
    sNdsStageMPCliffEscapeCommon2LoopPhysicsActive = TRUE;
    fp->proc_physics(fighter_gobj);
    if (gNdsStageMPCliffEscapeCommon2LoopGroundTransCount == 0u)
    {
        ftPhysicsApplyGroundVelTransN(fighter_gobj);
    }
    sNdsStageMPCliffEscapeCommon2LoopPhysicsActive = FALSE;
    gNdsStageMPCliffEscapeCommon2LoopStatusAfterPhysics =
        (u32)fp->status_id;
    gNdsStageMPCliffEscapeCommon2LoopMotionAfterPhysics =
        (u32)fp->motion_id;
    gNdsStageMPCliffEscapeCommon2LoopGAAfterPhysics = (u32)fp->ga;

    gNdsStageMPCliffEscapeCommon2LoopMapCallCount++;
    sNdsStageMPCliffEscapeCommon2LoopMapActive = TRUE;
    fp->proc_map(fighter_gobj);
    if (gNdsStageMPCliffEscapeCommon2LoopEdgeBreakCount == 0u)
    {
        mpCommonSetFighterFallOnEdgeBreak(fighter_gobj);
    }
    sNdsStageMPCliffEscapeCommon2LoopMapActive = FALSE;
    gNdsStageMPCliffEscapeCommon2LoopStatusAfterMap = (u32)fp->status_id;
    gNdsStageMPCliffEscapeCommon2LoopMotionAfterMap = (u32)fp->motion_id;
    gNdsStageMPCliffEscapeCommon2LoopGAAfterMap = (u32)fp->ga;
    gNdsStageMPCliffEscapeCommon2LoopCliffIDAfterMap =
        fp->coll_data.cliff_id;
    gNdsStageMPCliffEscapeCommon2LoopFloorLineAfterMap =
        fp->coll_data.floor_line_id;
    gNdsStageMPCliffEscapeCommon2LoopIsCliffHoldAfterMap =
        (fp->is_cliff_hold != FALSE) ? 1u : 0u;
}

void ndsFighterMarioFoxStageMPCliffEscapeCommon2LoopFinalize(void)
{
    u32 mask = 0u;

    if ((ndsFighterMarioFoxStageMPCliffEscapeCommon2LoopProofEnabled() ==
            FALSE) ||
        (gNdsStageMPCliffEscapeCommon2LoopPrepared == 0u) ||
        (gNdsFighterMarioFoxStageMPCliffEscapeCommon2LoopResult != 0u))
    {
        return;
    }

    if (gNdsFighterMarioFoxStageMPCliffEscapeActionLoopResult == 0u)
    {
        ndsFighterMarioFoxStageMPCliffEscapeActionLoopFinalize();
    }
    if ((gNdsFighterMarioFoxStageMPCliffEscapeActionLoopResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFESCAPE_ACTION_LOOP_PASS) &&
        (gNdsFighterMarioFoxStageMPCliffEscapeActionLoopSafeResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFESCAPE_ACTION_LOOP_SAFE_PASS))
    {
        gNdsStageMPCliffEscapeCommon2LoopBaseMPCliffEscapeActionSeen = 1u;
        mask |= 1u << 0;
    }
    if ((gNdsStageMPCliffEscapeCommon2LoopPrepared != 0u) &&
        (gNdsStageMPCliffEscapeCommon2LoopBaseMPCliffEscapeActionSeen != 0u))
    {
        mask |= 1u << 1;
    }

    if ((gNdsStageMPCliffEscapeCommon2LoopUpdateCallCount == 0u) &&
        (gNdsStageMPCliffEscapeCommon2LoopUnsafeCount == 0u))
    {
        ndsStageMPCliffEscapeCommon2LoopRunProbe();
    }

    if ((gNdsStageMPCliffEscapeCommon2LoopStatusBefore ==
            (u32)nFTCommonStatusCliffEscapeQuick2) &&
        (gNdsStageMPCliffEscapeCommon2LoopMotionBefore ==
            (u32)nFTCommonMotionCliffEscapeQuick2) &&
        (gNdsStageMPCliffEscapeCommon2LoopGABefore ==
            (u32)nMPKineticsGround) &&
        (gNdsStageMPCliffEscapeCommon2LoopCliffIDBefore >= 0) &&
        (gNdsStageMPCliffEscapeCommon2LoopFloorLineBefore ==
            gNdsStageMPCliffEscapeCommon2LoopCliffIDBefore))
    {
        mask |= 1u << 2;
    }
    if ((gNdsStageMPCliffEscapeCommon2LoopProcUpdateSet == 1u) &&
        (gNdsStageMPCliffEscapeCommon2LoopProcPhysicsSet == 1u) &&
        (gNdsStageMPCliffEscapeCommon2LoopProcMapSet == 1u))
    {
        mask |= 1u << 3;
    }
    if ((gNdsStageMPCliffEscapeCommon2LoopUpdateCallCount == 1u) &&
        (gNdsStageMPCliffEscapeCommon2LoopAnimEndCheckCount == 1u) &&
        (gNdsStageMPCliffEscapeCommon2LoopWaitOrFallCallCount == 0u) &&
        (gNdsStageMPCliffEscapeCommon2LoopStatusAfterUpdate ==
            (u32)nFTCommonStatusCliffEscapeQuick2) &&
        (gNdsStageMPCliffEscapeCommon2LoopMotionAfterUpdate ==
            (u32)nFTCommonMotionCliffEscapeQuick2) &&
        (gNdsStageMPCliffEscapeCommon2LoopGAAfterUpdate ==
            (u32)nMPKineticsGround))
    {
        mask |= 1u << 4;
    }
    if ((gNdsStageMPCliffEscapeCommon2LoopPhysicsCallCount == 1u) &&
        (gNdsStageMPCliffEscapeCommon2LoopGroundTransCount == 1u) &&
        (gNdsStageMPCliffEscapeCommon2LoopStatusAfterPhysics ==
            (u32)nFTCommonStatusCliffEscapeQuick2) &&
        (gNdsStageMPCliffEscapeCommon2LoopMotionAfterPhysics ==
            (u32)nFTCommonMotionCliffEscapeQuick2) &&
        (gNdsStageMPCliffEscapeCommon2LoopGAAfterPhysics ==
            (u32)nMPKineticsGround))
    {
        mask |= 1u << 5;
    }
    if ((gNdsStageMPCliffEscapeCommon2LoopMapCallCount == 1u) &&
        (gNdsStageMPCliffEscapeCommon2LoopEdgeBreakCount == 1u) &&
        (gNdsStageMPCliffEscapeCommon2LoopStatusAfterMap ==
            (u32)nFTCommonStatusCliffEscapeQuick2) &&
        (gNdsStageMPCliffEscapeCommon2LoopMotionAfterMap ==
            (u32)nFTCommonMotionCliffEscapeQuick2) &&
        (gNdsStageMPCliffEscapeCommon2LoopGAAfterMap ==
            (u32)nMPKineticsGround))
    {
        mask |= 1u << 6;
    }
    if ((gNdsStageMPCliffEscapeCommon2LoopCliffIDAfterMap ==
            gNdsStageMPCliffEscapeCommon2LoopCliffIDBefore) &&
        (gNdsStageMPCliffEscapeCommon2LoopFloorLineAfterMap ==
            gNdsStageMPCliffEscapeCommon2LoopFloorLineBefore))
    {
        mask |= 1u << 7;
    }
    if ((gNdsStageMPCliffEscapeCommon2LoopIsCliffHoldAfterMap == 0u) &&
        (gNdsStageMPCliffEscapeCommon2LoopUnsafeCount == 0u) &&
        (sNdsStageMPCliffEscapeCommon2LoopUpdateActive == FALSE) &&
        (sNdsStageMPCliffEscapeCommon2LoopPhysicsActive == FALSE) &&
        (sNdsStageMPCliffEscapeCommon2LoopMapActive == FALSE))
    {
        mask |= 1u << 8;
    }

    gNdsFighterMarioFoxStageMPCliffEscapeCommon2LoopCount = 1u;
    gNdsFighterMarioFoxStageMPCliffEscapeCommon2LoopMask = mask;
    gNdsFighterMarioFoxStageMPCliffEscapeCommon2LoopDeferredMask = 0xffu;
    if ((mask & 0x1ffu) == 0x1ffu)
    {
        gNdsFighterMarioFoxStageMPCliffEscapeCommon2LoopResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFESCAPE_COMMON2_LOOP_PASS;
        gNdsFighterMarioFoxStageMPCliffEscapeCommon2LoopSafeResult =
            NDS_FIGHTER_MARIOFOX_STAGE_MPCLIFFESCAPE_COMMON2_LOOP_SAFE_PASS;
    }
}

void ndsFighterMarioFoxStageFloorEdgeLoopFinalize(void)
{
    u32 mask = 0u;
    s32 p0_near_threshold;
    s32 p1_near_threshold;

    if ((ndsFighterMarioFoxStageFloorEdgeLoopProofEnabled() == FALSE) ||
        (gNdsStageFloorEdgeLoopPrepared == 0u) ||
        (gNdsFighterMarioFoxStageFloorEdgeLoopResult != 0u))
    {
        return;
    }
    gNdsStageFloorEdgeLoopP0DeltaDistMilli =
        gNdsStageFloorEdgeLoopP0StartDistMilli -
        gNdsStageFloorEdgeLoopP0FinalDistMilli;
    gNdsStageFloorEdgeLoopP1DeltaDistMilli =
        gNdsStageFloorEdgeLoopP1StartDistMilli -
        gNdsStageFloorEdgeLoopP1FinalDistMilli;
    p0_near_threshold = (gNdsStageFloorEdgeLoopP0StartDistMilli * 3) / 4;
    p1_near_threshold = (gNdsStageFloorEdgeLoopP1StartDistMilli * 3) / 4;
    if (p0_near_threshold < 32000)
    {
        p0_near_threshold = 32000;
    }
    if (p1_near_threshold < 32000)
    {
        p1_near_threshold = 32000;
    }
    gNdsStageFloorEdgeLoopP0ApproachOK =
        (gNdsStageFloorEdgeLoopP0DeltaDistMilli > 0) ? 1u : 0u;
    gNdsStageFloorEdgeLoopP1ApproachOK =
        (gNdsStageFloorEdgeLoopP1DeltaDistMilli > 0) ? 1u : 0u;
    gNdsStageFloorEdgeLoopP0NearEdgeOK =
        ((gNdsStageFloorEdgeLoopP0FinalDistMilli >= -16000) &&
         (gNdsStageFloorEdgeLoopP0FinalDistMilli <= p0_near_threshold)) ?
            1u : 0u;
    gNdsStageFloorEdgeLoopP1NearEdgeOK =
        ((gNdsStageFloorEdgeLoopP1FinalDistMilli >= -16000) &&
         (gNdsStageFloorEdgeLoopP1FinalDistMilli <= p1_near_threshold)) ?
            1u : 0u;

    if ((gNdsFighterMarioFoxGCDrawAllLoopResult ==
            NDS_FIGHTER_MARIOFOX_GCDRAWALL_LOOP_PASS) &&
        (gNdsFighterMarioFoxStageGCDrawAllLoopResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_GCDRAWALL_LOOP_PASS) &&
        (gNdsStageCollisionLoopPrepared != 0u) &&
        (gNdsStageCollisionLoopGroundDataReady == 1u) &&
        (gNdsStageCollisionLoopGeometryReady == 1u) &&
        (gNdsStageFloorFollowLoopPrepared != 0u) &&
        (gNdsStageFloorFollowLoopGeometryReady != 0u))
    {
        mask |= 0xfu;
    }
    if ((gNdsStageFloorEdgeLoopPrepared != 0u) &&
        (gNdsStageFloorEdgeLoopGeometryReady != 0u))
    {
        mask |= 1u << 4;
    }
    if ((gNdsStageFloorEdgeLoopSelectedLineID >= 0) &&
        (gNdsStageFloorEdgeLoopSelectedLineKind == (u32)nMPLineKindFloor) &&
        (gNdsStageFloorEdgeLoopSelectedVertexCount >= 2u) &&
        (gNdsStageFloorEdgeLoopWidthMilli > 0))
    {
        mask |= 1u << 5;
    }
    if ((gNdsStageFloorEdgeLoopP0StartDistMilli > 0) &&
        (gNdsStageFloorEdgeLoopP1StartDistMilli > 0))
    {
        mask |= 1u << 6;
    }
    if ((gNdsStageFloorEdgeLoopInsideProbeCount >= 2u) &&
        (gNdsStageFloorEdgeLoopInsideProbeHitCount >= 2u))
    {
        mask |= 1u << 7;
    }
    if ((gNdsStageFloorEdgeLoopOutsideProbeCount >= 2u) &&
        (gNdsStageFloorEdgeLoopOutsideProbeMissCount >= 2u) &&
        (gNdsStageFloorEdgeLoopOutsideProbeUnexpectedHitCount == 0u))
    {
        mask |= 1u << 8;
    }
    if ((gNdsStageFloorEdgeLoopFCCommonCallCount > 0u) &&
        (gNdsStageFloorEdgeLoopFCCommonHitCount > 0u))
    {
        mask |= 1u << 9;
    }
    if ((gNdsStageFloorEdgeLoopLineTypeCallCount > 0u) &&
        (gNdsStageFloorEdgeLoopVertexPositionCallCount >= 2u))
    {
        mask |= 1u << 10;
    }
    if ((gNdsStageFloorEdgeLoopP0ApproachOK != 0u) &&
        (gNdsStageFloorEdgeLoopP1ApproachOK != 0u))
    {
        mask |= 1u << 11;
    }
    if ((gNdsStageFloorEdgeLoopP0NearEdgeOK != 0u) &&
        (gNdsStageFloorEdgeLoopP1NearEdgeOK != 0u))
    {
        mask |= 1u << 12;
    }
    if ((gNdsStageFloorEdgeLoopP0FloorOK != 0u) &&
        (gNdsStageFloorEdgeLoopP1FloorOK != 0u) &&
        (gNdsStageFloorEdgeLoopP0FloorVisitMask != 0u) &&
        (gNdsStageFloorEdgeLoopP1FloorVisitMask != 0u))
    {
        mask |= 1u << 13;
    }
    if ((gNdsStageFloorEdgeLoopFinalRecenterCount == 0u) &&
        (gNdsStageFloorEdgeLoopFinalAdoptCount == 0u) &&
        (gNdsStageFloorFollowLoopFinalRecenterCount == 0u) &&
        (gNdsStageFloorFollowLoopFinalAdoptCount == 0u) &&
        (gNdsStageFloorEdgeLoopUnexpectedSceneCount == 0u) &&
        (gNdsStageFloorEdgeLoopUnexpectedStatusCount == 0u) &&
        (gNdsStageFloorEdgeLoopUnsafeFallbackAfterPrepareCount == 0u))
    {
        mask |= 1u << 14;
    }
    if ((gNdsStageFloorEdgeLoopMapUpdateCount > 0u) &&
        (gNdsStageFloorEdgeLoopP0MapUpdateCount > 0u) &&
        (gNdsStageFloorEdgeLoopP1MapUpdateCount > 0u) &&
        (gNdsStageFloorEdgeLoopEdgeUnderLCallCount > 0u) &&
        (gNdsStageFloorEdgeLoopEdgeUnderRCallCount > 0u) &&
        (gNdsStageFloorEdgeLoopEdgeUnderDeferredCount ==
            (gNdsStageFloorEdgeLoopEdgeUnderLCallCount +
             gNdsStageFloorEdgeLoopEdgeUnderRCallCount)))
    {
        mask |= 1u << 15;
    }
    gNdsFighterMarioFoxStageFloorEdgeLoopMask = mask;
    gNdsFighterMarioFoxStageFloorEdgeLoopDeferredMask = 0xffu;
    gNdsFighterMarioFoxStageFloorEdgeLoopCount =
        gNdsFighterMarioFoxStageFloorFollowLoopCount;
    if ((mask & 0xffffu) == 0xffffu)
    {
        gNdsFighterMarioFoxStageFloorEdgeLoopResult =
            NDS_FIGHTER_MARIOFOX_STAGE_FLOOR_EDGE_LOOP_PASS;
        gNdsFighterMarioFoxStageFloorEdgeLoopSafeResult =
            NDS_FIGHTER_MARIOFOX_STAGE_FLOOR_EDGE_LOOP_SAFE_PASS;
    }
}

void ndsFighterMarioFoxStageFloorFollowLoopFinalize(void)
{
    u32 mask = 0u;

    if ((ndsFighterMarioFoxStageFloorFollowLoopProofEnabled() == FALSE) ||
        (gNdsStageFloorFollowLoopPrepared == 0u) ||
        (gNdsFighterMarioFoxStageFloorFollowLoopResult != 0u))
    {
        return;
    }
    ndsStageFloorFollowLoopRecordFinalSlot(0u);
    ndsStageFloorFollowLoopRecordFinalSlot(1u);

    if ((gNdsFighterMarioFoxGCDrawAllLoopResult ==
            NDS_FIGHTER_MARIOFOX_GCDRAWALL_LOOP_PASS) &&
        (gNdsFighterMarioFoxGCDrawAllLoopSafeResult ==
            NDS_FIGHTER_MARIOFOX_GCDRAWALL_LOOP_SAFE_PASS))
    {
        mask |= 1u << 0;
    }
    if ((gNdsFighterMarioFoxStageGCDrawAllLoopResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_GCDRAWALL_LOOP_PASS) &&
        (gNdsFighterMarioFoxStageGCDrawAllLoopSafeResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_GCDRAWALL_LOOP_SAFE_PASS))
    {
        gNdsStageFloorFollowLoopBaseDrawSeen = 1u;
        mask |= 1u << 1;
    }
    if ((gNdsFighterMarioFoxStageCollisionLoopResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_COLLISION_LOOP_PASS) &&
        (gNdsFighterMarioFoxStageCollisionLoopSafeResult ==
            NDS_FIGHTER_MARIOFOX_STAGE_COLLISION_LOOP_SAFE_PASS))
    {
        gNdsStageFloorFollowLoopBaseCollisionSeen = 1u;
        mask |= 1u << 2;
    }
    if ((gNdsStageFloorFollowLoopPrepared != 0u) &&
        (gNdsStageFloorFollowLoopGeometryReady != 0u))
    {
        mask |= 1u << 3;
    }
    if ((gNdsStageFloorFollowLoopInitialSeedCount == 2u) &&
        (gNdsStageFloorFollowLoopInitialAdoptCount == 2u))
    {
        mask |= 1u << 4;
    }
    if ((gNdsStageFloorFollowLoopP0MapUpdateCount > 0u) &&
        (gNdsStageFloorFollowLoopP1MapUpdateCount > 0u))
    {
        mask |= 1u << 5;
    }
    if ((gNdsStageFloorFollowLoopP0HitCount > 0u) &&
        (gNdsStageFloorFollowLoopP1HitCount > 0u))
    {
        mask |= 1u << 6;
    }
    if ((gNdsStageFloorFollowLoopGeometryMissCount == 0u) &&
        (gNdsStageFloorFollowLoopNoGeometryCount == 0u) &&
        (gNdsStageCollisionLoopLegacyFlatFallbackCount == 0u))
    {
        mask |= 1u << 7;
    }
    if (gNdsStageFloorFollowLoopNonFloorLineCount == 0u)
    {
        mask |= 1u << 8;
    }
    if ((gNdsStageFloorFollowLoopP0FloorKind ==
            (u32)nMPLineKindFloor) &&
        (gNdsStageFloorFollowLoopP1FloorKind ==
            (u32)nMPLineKindFloor) &&
        (gNdsStageFloorFollowLoopP0FloorLineIsFloor == 1u) &&
        (gNdsStageFloorFollowLoopP1FloorLineIsFloor == 1u))
    {
        mask |= 1u << 9;
    }
    if ((gNdsStageFloorFollowLoopP0RootXDeltaMilli != 0) &&
        (gNdsStageFloorFollowLoopP1RootXDeltaMilli != 0))
    {
        mask |= 1u << 10;
    }
    if ((gNdsStageFloorFollowLoopP0FinalDriftMilli <= 1000) &&
        (gNdsStageFloorFollowLoopP1FinalDriftMilli <= 1000) &&
        (gNdsStageFloorFollowLoopP0MaxDriftMilli <= 1000000) &&
        (gNdsStageFloorFollowLoopP1MaxDriftMilli <= 1000000))
    {
        mask |= 1u << 11;
    }
    if ((gNdsFighterGCDrawAllLoopP0StatusFinal ==
            (u32)nFTCommonStatusWait) &&
        (gNdsFighterGCDrawAllLoopP1StatusFinal ==
            (u32)nFTCommonStatusWait) &&
        (gNdsStageFloorFollowLoopP0GAFinal == (u32)nMPKineticsGround) &&
        (gNdsStageFloorFollowLoopP1GAFinal == (u32)nMPKineticsGround) &&
        (gNdsStageFloorFollowLoopP0FloorOK == 1u) &&
        (gNdsStageFloorFollowLoopP1FloorOK == 1u))
    {
        mask |= 1u << 12;
    }
    if ((gNdsFighterGCDrawAllLoopRunAllCount > 0u) &&
        (gNdsFighterGCDrawAllLoopDrawAllCount > 0u))
    {
        mask |= 1u << 13;
    }
    if ((gNdsStageCollisionLoopUnsafeFallbackAfterPrepareCount == 0u) &&
        (gNdsStageCollisionLoopUnexpectedStatusCount == 0u) &&
        (gNdsStageCollisionLoopUnexpectedSceneCount == 0u) &&
        (gNdsStageGCDrawAllLoopUnexpectedSceneCount == 0u))
    {
        mask |= 1u << 14;
    }
    if ((gNdsStageFloorFollowLoopFinalRecenterCount == 0u) &&
        (gNdsStageFloorFollowLoopFinalAdoptCount == 0u))
    {
        mask |= 1u << 15;
    }

    gNdsFighterMarioFoxStageFloorFollowLoopMask = mask;
    gNdsFighterMarioFoxStageFloorFollowLoopDeferredMask = 0xffu;
    gNdsFighterMarioFoxStageFloorFollowLoopCount =
        gNdsFighterMarioFoxStageCollisionLoopCount;
    if ((mask & 0xffffu) == 0xffffu)
    {
        gNdsFighterMarioFoxStageFloorFollowLoopResult =
            NDS_FIGHTER_MARIOFOX_STAGE_FLOOR_FOLLOW_LOOP_PASS;
        gNdsFighterMarioFoxStageFloorFollowLoopSafeResult =
            NDS_FIGHTER_MARIOFOX_STAGE_FLOOR_FOLLOW_LOOP_SAFE_PASS;
    }
}

static void ndsFighterMarioFoxStageGCDrawAllLoopReset(void)
{
    gNdsFighterMarioFoxStageGCDrawAllLoopResult = 0u;
    gNdsFighterMarioFoxStageGCDrawAllLoopSafeResult = 0u;
    gNdsFighterMarioFoxStageGCDrawAllLoopMask = 0u;
    gNdsFighterMarioFoxStageGCDrawAllLoopDeferredMask = 0u;
    gNdsFighterMarioFoxStageGCDrawAllLoopCount = 0u;
    gNdsStageGCDrawAllLoopPrepared = 0u;
    gNdsStageGCDrawAllLoopBaseResultSeen = 0u;
    gNdsStageGCDrawAllLoopDrawAllCount = 0u;
    gNdsStageGCDrawAllLoopCameraCallbackCount = 0u;
    gNdsStageGCDrawAllLoopCapturedDisplayCount = 0u;
    gNdsStageGCDrawAllLoopLayerCaptureMask = 0u;
    gNdsStageGCDrawAllLoopMapCaptureMask = 0u;
    gNdsStageGCDrawAllLoopDObjDrawCallbackCount = 0u;
    gNdsStageGCDrawAllLoopDObjDrawKindMask = 0u;
    gNdsStageGCDrawAllLoopLayerDObjMask = 0u;
    gNdsStageGCDrawAllLoopMapDObjMask = 0u;
    gNdsStageGCDrawAllLoopLayerDLReadyMask = 0u;
    gNdsStageGCDrawAllLoopMapDLReadyMask = 0u;
    gNdsStageGCDrawAllLoopLayerMObjMask = 0u;
    gNdsStageGCDrawAllLoopMapMObjMask = 0u;
    gNdsStageGCDrawAllLoopNonStageCaptureCount = 0u;
    gNdsStageGCDrawAllLoopFighterDisplayCallbackCount = 0u;
    gNdsStageGCDrawAllLoopUnexpectedSceneCount = 0u;
    gNdsStageGCDrawAllLoopManualDisplayCallCount = 0u;
    gNdsStageGCDrawAllLoopGObjCountBefore = 0u;
    gNdsStageGCDrawAllLoopGObjCountAfter = 0u;
    gNdsStageGCDrawAllLoopGObjCountDelta = 0u;
    gNdsStageGCDrawAllLoopPreviewCommitDelta = 0u;
    gNdsStageGCDrawAllLoopTotalPixelCount = 0u;
    gNdsStageGCDrawAllLoopCompatMask = 0u;
    gNdsStageGCDrawAllLoopHardwareSubmitCount = 0u;
    gNdsStageGCDrawAllLoopHardwareTriangleCount = 0u;
    gNdsStageGCDrawAllLoopHardwareZBufferTriangleCount = 0u;
    gNdsStageGCDrawAllLoopHardwareProjectedDepthTriangleCount = 0u;
    gNdsStageGCDrawAllLoopHardwareDecalDepthTriangleCount = 0u;
    gNdsStageGCDrawAllLoopHardwareTextureBindCount = 0u;
    gNdsStageGCDrawAllLoopHardwareTextureUploadCount = 0u;
    gNdsStageGCDrawAllLoopHardwareTextureReadyCount = 0u;
    gNdsStageGCDrawAllLoopHardwareTextureRejectCount = 0u;
    gNdsStageGCDrawAllLoopHardwareTextureFormatMask = 0u;
    gNdsStageGCDrawAllLoopHardwareTextureMaxWidth = 0u;
    gNdsStageGCDrawAllLoopHardwareTextureMaxHeight = 0u;
    gNdsStageGCDrawAllLoopHardwareFighterSubmitCount = 0u;
    gNdsStageGCDrawAllLoopHardwareFighterTriangleCount = 0u;
    gNdsStageGCDrawAllLoopHardwareCarrySeedCount = 0u;
    gNdsStageGCDrawAllLoopHardwareCarryCaptureCount = 0u;
    gNdsStageGCDrawAllLoopHardwareCarryTextureSeedCount = 0u;
    gNdsStageGCDrawAllLoopHardwareCarryTileSeedCount = 0u;
    gNdsStageGCDrawAllLoopHardwareCarryShortTextureSeedCount = 0u;
    gNdsStageGCDrawAllLoopHardwareCarryShortTileSeedCount = 0u;
    gNdsStageGCDrawAllLoopHardwareCarrySegmentSeedCount = 0u;
    sNdsStageGCDrawAllLoopCurrentDisplayLinkID = -1;
#if NDS_RENDERER_HW_TRIANGLES
    sNdsStageGCDrawAllLoopHardwareSubmitActive = FALSE;
    sNdsStageGCDrawAllLoopHardwareSubmitCount = 0u;
#endif
}

void ndsFighterMarioFoxStageGCDrawAllLoopPrepare(void)
{
    if ((ndsFighterMarioFoxStageGCDrawAllLoopProofEnabled() == FALSE) ||
        (gNdsStageGCDrawAllLoopPrepared != 0u))
    {
        return;
    }
    ndsFighterMarioFoxStageGCDrawAllLoopReset();
    gNdsStageGCDrawAllLoopGObjCountBefore = (u32)gcGetGObjsActiveNum();
    gNdsStageGCDrawAllLoopPrepared = 1u;
}

void ndsFighterMarioFoxStageGCDrawAllLoopFinalize(void)
{
    u32 mask = 0u;

    if ((ndsFighterMarioFoxStageGCDrawAllLoopProofEnabled() == FALSE) ||
        (gNdsStageGCDrawAllLoopPrepared == 0u) ||
        (gNdsFighterMarioFoxStageGCDrawAllLoopResult != 0u))
    {
        return;
    }
    if (gNdsFighterMarioFoxGCDrawAllLoopResult ==
        NDS_FIGHTER_MARIOFOX_GCDRAWALL_LOOP_PASS)
    {
        mask |= 1u << 0;
        gNdsStageGCDrawAllLoopBaseResultSeen = 1u;
    }
    if (gNdsFighterMarioFoxGCDrawAllLoopSafeResult ==
        NDS_FIGHTER_MARIOFOX_GCDRAWALL_LOOP_SAFE_PASS)
    {
        mask |= 1u << 1;
    }
    if ((gSCManagerSceneData.gkind == nGRKindPupupu) &&
        (gNdsPupupuGroundSetupResult == NDS_PUPUPU_GROUND_SETUP_PASS) &&
        ((gNdsPupupuGroundLayerGObjMask & 0xfu) == 0xfu) &&
        ((gNdsPupupuGroundMapGObjMask & 0xfu) == 0xfu))
    {
        mask |= 1u << 2;
    }
    if (gNdsStageGCDrawAllLoopCameraCallbackCount > 0u)
    {
        mask |= 1u << 3;
    }
    if (gNdsStageGCDrawAllLoopCapturedDisplayCount > 0u)
    {
        mask |= 1u << 4;
    }
    if ((gNdsStageGCDrawAllLoopLayerCaptureMask & 0xfu) == 0xfu)
    {
        mask |= 1u << 5;
    }
    if ((gNdsStageGCDrawAllLoopMapCaptureMask & 0xfu) == 0xfu)
    {
        mask |= 1u << 6;
    }
    if (gNdsStageGCDrawAllLoopDObjDrawCallbackCount > 0u)
    {
        mask |= 1u << 7;
    }
    if (((gNdsStageGCDrawAllLoopLayerDObjMask |
          gNdsStageGCDrawAllLoopMapDObjMask) != 0u))
    {
        mask |= 1u << 8;
    }
    if (((gNdsStageGCDrawAllLoopLayerDLReadyMask |
          gNdsStageGCDrawAllLoopMapDLReadyMask) != 0u))
    {
        mask |= 1u << 9;
    }
    if ((gNdsFighterGCDrawAllLoopDisplayCallbackCount >=
            (NDS_FIGHTER_GCDRAWALL_LOOP_DRAW_FRAME_MIN * 2u)) &&
        (gNdsFighterGCDrawAllLoopTotalPixelCount > 0u))
    {
        mask |= 1u << 10;
    }
    if ((gNdsStageGCDrawAllLoopManualDisplayCallCount == 0u) &&
        (gNdsStageGCDrawAllLoopUnexpectedSceneCount == 0u) &&
        (gNdsFighterMarioFoxGCDrawAllLoopSafeResult ==
            NDS_FIGHTER_MARIOFOX_GCDRAWALL_LOOP_SAFE_PASS))
    {
        mask |= 1u << 11;
    }

    gNdsStageGCDrawAllLoopGObjCountAfter = (u32)gcGetGObjsActiveNum();
    gNdsStageGCDrawAllLoopGObjCountDelta =
        (gNdsStageGCDrawAllLoopGObjCountAfter >=
         gNdsStageGCDrawAllLoopGObjCountBefore) ?
        (gNdsStageGCDrawAllLoopGObjCountAfter -
         gNdsStageGCDrawAllLoopGObjCountBefore) :
        (gNdsStageGCDrawAllLoopGObjCountBefore -
         gNdsStageGCDrawAllLoopGObjCountAfter);
    gNdsStageGCDrawAllLoopPreviewCommitDelta =
        gNdsFighterGCDrawAllLoopPreviewCommitDelta;
    gNdsStageGCDrawAllLoopTotalPixelCount =
        gNdsFighterGCDrawAllLoopTotalPixelCount;
    gNdsFighterMarioFoxStageGCDrawAllLoopMask = mask;
    gNdsFighterMarioFoxStageGCDrawAllLoopDeferredMask = 0xffu;
    gNdsFighterMarioFoxStageGCDrawAllLoopCount =
        gNdsFighterMarioFoxGCDrawAllLoopCount;
    if ((mask & 0xfffu) == 0xfffu)
    {
        gNdsFighterMarioFoxStageGCDrawAllLoopResult =
            NDS_FIGHTER_MARIOFOX_STAGE_GCDRAWALL_LOOP_PASS;
        gNdsFighterMarioFoxStageGCDrawAllLoopSafeResult =
            NDS_FIGHTER_MARIOFOX_STAGE_GCDRAWALL_LOOP_SAFE_PASS;
#if NDS_RENDERER_HW_TRIANGLES
        ndsStageGCDrawAllLoopSubmitHardwareFrame();
#endif
    }
}

static void ndsFighterGCDrawAllLoopDrawKeyframe(void)
{
    u32 pitch = 0u;
    u16 *pixels;
    u32 callbacks_before;

    pixels = ndsPlatformBeginOriginalDLPreview(
        NDS_FIGHTER_GCDRAWALL_LOOP_WIDTH,
        NDS_FIGHTER_GCDRAWALL_LOOP_HEIGHT,
        &pitch);
    if (pixels == NULL)
    {
        return;
    }
    if (gNdsFighterGCDrawAllLoopDrawFrameCount == 0u)
    {
        gNdsFighterGCDrawAllLoopPreviewCommitBefore =
            gNdsOriginalDLPreviewCommitCount;
    }
    sNdsFighterGCDrawAllLoopPixels = pixels;
    sNdsFighterGCDrawAllLoopPitch = pitch;
    sNdsFighterGCDrawAllLoopDisplayActive = TRUE;
    callbacks_before = gNdsFighterGCDrawAllLoopDisplayCallbackCount;

    ndsFighterPreviewLoopClear(pixels, pitch);
    gcDrawAll();
    gNdsFighterGCDrawAllLoopDrawAllCount++;
    if (ndsFighterMarioFoxStageGCDrawAllLoopProofEnabled() != FALSE)
    {
        gNdsStageGCDrawAllLoopDrawAllCount++;
    }

    sNdsFighterGCDrawAllLoopDisplayActive = FALSE;
    sNdsFighterGCDrawAllLoopPixels = NULL;
    sNdsFighterGCDrawAllLoopPitch = 0u;

    ndsFighterGCDrawAllLoopCopyFromPreview();
    gNdsFighterGCDrawAllLoopPreviewWidth =
        NDS_FIGHTER_GCDRAWALL_LOOP_WIDTH;
    gNdsFighterGCDrawAllLoopPreviewHeight =
        NDS_FIGHTER_GCDRAWALL_LOOP_HEIGHT;
    gNdsFighterGCDrawAllLoopPreviewPitch = pitch;
    gNdsFighterGCDrawAllLoopTotalPixelCount =
        gNdsFighterGCDrawAllLoopP0PixelCount +
        gNdsFighterGCDrawAllLoopP1PixelCount;

    if ((gNdsFighterGCDrawAllLoopTotalPixelCount > 0u) &&
        (gNdsFighterGCDrawAllLoopDisplayCallbackCount > callbacks_before))
    {
        ndsPlatformCommitOriginalDLPreview();
        gNdsFighterGCDrawAllLoopPreviewCommitAfter =
            gNdsOriginalDLPreviewCommitCount;
        gNdsFighterGCDrawAllLoopPreviewCommitDelta =
            gNdsFighterGCDrawAllLoopPreviewCommitAfter -
            gNdsFighterGCDrawAllLoopPreviewCommitBefore;
        gNdsFighterGCDrawAllLoopPreviewReady =
            gNdsOriginalDLPreviewReady;
        gNdsFighterGCDrawAllLoopDrawFrameCount++;
    }
}

void ndsFighterMarioFoxGCDrawAllLoopPrepare(void)
{
    u32 i;

    if ((ndsFighterMarioFoxGCDrawAllLoopProofEnabled() == FALSE) ||
        (gNdsFighterGCDrawAllLoopPrepared != 0u))
    {
        return;
    }
    if ((gNdsFighterMarioFoxGCRunAllLoopResult !=
            NDS_FIGHTER_MARIOFOX_GCRUNALL_LOOP_PASS) ||
        (gNdsFighterMarioFoxGCRunAllLoopSafeResult !=
            NDS_FIGHTER_MARIOFOX_GCRUNALL_LOOP_SAFE_PASS) ||
        ((gNdsFighterMarioFoxGCRunAllLoopMask & 0x1fffu) != 0x1fffu) ||
        (gNdsFighterMarioFoxGCRunAllLoopDeferredMask != 0xffu) ||
        (gNdsFighterMarioFoxGCRunAllLoopCount != 2u) ||
        (gNdsFighterGCRunAllLoopP0StatusFinal !=
            (u32)nFTCommonStatusWait) ||
        (gNdsFighterGCRunAllLoopP1StatusFinal !=
            (u32)nFTCommonStatusWait) ||
        (gNdsFighterGCRunAllLoopP0MotionFinal !=
            (u32)nFTCommonMotionWait) ||
        (gNdsFighterGCRunAllLoopP1MotionFinal !=
            (u32)nFTCommonMotionWait) ||
        (gNdsFighterGCRunAllLoopP0GAFinal != (u32)nMPKineticsGround) ||
        (gNdsFighterGCRunAllLoopP1GAFinal != (u32)nMPKineticsGround))
    {
        return;
    }

    bzero(sNdsFighterPreviewLoopStates,
          sizeof(sNdsFighterPreviewLoopStates));
    bzero(sNdsFighterGCDrawAllLoopProcesses,
          sizeof(sNdsFighterGCDrawAllLoopProcesses));
    ndsControllerPlaybackReset();
    ndsControllerPlaybackSetConnectedMask(0x3u);
    ndsControllerPlaybackSetEnabled(TRUE);
    gNdsFighterGCDrawAllLoopFrameMax =
        NDS_FIGHTER_GCDRAWALL_LOOP_FRAME_MAX;
    gNdsFighterGCDrawAllLoopUpdateMax =
        NDS_FIGHTER_GCDRAWALL_LOOP_UPDATE_MAX;
    gNdsFighterGCDrawAllLoopGObjCountBefore = (u32)gcGetGObjsActiveNum();

    ndsFighterGCDrawAllLoopPauseProofOwnedProcesses();
    gcFuncGObjAll(ndsFighterGCDrawAllLoopPauseNonTargetGObjVisitor, 0u);

    for (i = 0u; i < 2u; i++)
    {
        FTStruct *fp = &sNdsFighterStructPool[i];
        GObj *fighter_gobj = fp->fighter_gobj;
        DObj *root = fp->joints[nFTPartsJointTopN];

        if ((ndsFighterStructIsPoolPointer(fp) == FALSE) ||
            (fighter_gobj == NULL) || (root == NULL))
        {
            gNdsFighterGCDrawAllLoopProcessAttachEscapeCount++;
            continue;
        }
        fighter_gobj->flags |= GOBJ_FLAG_NORUN;
        ndsFighterPreviewLoopRecordStart(i, fp, root);
        sNdsFighterGCDrawAllLoopProcesses[i] =
            gcAddGObjProcess(fighter_gobj,
                             ndsFighterGCDrawAllLoopGObjProc,
                             nGCProcessKindFunc,
                             3);
        if (sNdsFighterGCDrawAllLoopProcesses[i] == NULL)
        {
            gNdsFighterGCDrawAllLoopProcessAttachEscapeCount++;
        }
        else if (i == 0u)
        {
            gNdsFighterGCDrawAllLoopP0ProcessAttachCount++;
        }
        else
        {
            gNdsFighterGCDrawAllLoopP1ProcessAttachCount++;
        }
    }

    ndsFighterGCDrawAllLoopCopyFromPreview();
    if ((sNdsFighterGCDrawAllLoopProcesses[0] != NULL) &&
        (sNdsFighterGCDrawAllLoopProcesses[1] != NULL))
    {
        gNdsFighterGCDrawAllLoopPrepared = 1u;
    }
}

s32 ndsFighterMarioFoxGCDrawAllLoopUpdateEnabled(void)
{
    return ((ndsFighterMarioFoxGCDrawAllLoopProofEnabled() != FALSE) &&
            (gNdsFighterGCDrawAllLoopPrepared != 0u) &&
            (gNdsFighterMarioFoxGCDrawAllLoopResult == 0u)) ? TRUE : FALSE;
}

void ndsFighterMarioFoxGCDrawAllLoopRunVSBattleUpdate(void)
{
    u32 i;
    u32 mask = 0u;

    if (ndsFighterMarioFoxGCDrawAllLoopUpdateEnabled() == FALSE)
    {
        return;
    }
    gNdsFighterGCDrawAllLoopVSBattleUpdateCount++;

    for (i = 0u; i < 2u; i++)
    {
        ndsFighterPreviewLoopApplyPlayback(i, &sNdsFighterStructPool[i]);
    }
    ndsControllerPlaybackCommitFrame();
    syControllerReadDeviceData();
    gNdsFighterGCDrawAllLoopSYReadCount++;
    syControllerUpdateGlobalData();
    gNdsFighterGCDrawAllLoopSYUpdateCount++;

    sNdsFighterGCDrawAllLoopActive = TRUE;
    gcRunAll();
    sNdsFighterGCDrawAllLoopActive = FALSE;
    gNdsFighterGCDrawAllLoopRunAllCount++;

    for (i = 0u; i < 2u; i++)
    {
        FTStruct *fp = &sNdsFighterStructPool[i];
        DObj *root = fp->joints[nFTPartsJointTopN];
        ndsFighterPreviewLoopRecordFinal(i, fp, root);
    }

    if (((gNdsFighterGCDrawAllLoopVSBattleUpdateCount %
            NDS_FIGHTER_GCDRAWALL_LOOP_DRAW_FRAME_INTERVAL) == 0u) ||
        ((gNdsFighterPreviewLoopP0Completed == 1u) &&
         (gNdsFighterPreviewLoopP1Completed == 1u)))
    {
        ndsFighterGCDrawAllLoopDrawKeyframe();
    }
    ndsFighterGCDrawAllLoopCopyFromPreview();
    if ((ndsFighterMarioFoxStageMPMotionStaleFloorLoopProofEnabled() !=
            FALSE) &&
        (gNdsStageMPMotionStaleFloorLoopP0FinalFloorOK != 0u) &&
        (gNdsStageMPMotionStaleFloorLoopTargetLineID >= 0) &&
        (gNdsStageMPMotionStaleFloorLoopP0FinalLineID ==
            gNdsStageMPMotionStaleFloorLoopTargetLineID))
    {
        gNdsFighterGCDrawAllLoopP0RootDirectionOK = 1u;
    }

    if ((gNdsFighterGCDrawAllLoopP0Completed == 1u) &&
        (gNdsFighterGCDrawAllLoopP1Completed == 1u))
    {
        gNdsFighterGCDrawAllLoopRootYDriftCount = 0u;
        gNdsFighterGCDrawAllLoopGADriftCount = 0u;
        if ((gNdsFighterGCDrawAllLoopP0StatusFinal !=
                (u32)nFTCommonStatusWait) ||
            (gNdsFighterGCDrawAllLoopP1StatusFinal !=
                (u32)nFTCommonStatusWait) ||
            (gNdsFighterGCDrawAllLoopP0MotionFinal !=
                (u32)nFTCommonMotionWait) ||
            (gNdsFighterGCDrawAllLoopP1MotionFinal !=
                (u32)nFTCommonMotionWait) ||
            (gNdsFighterGCDrawAllLoopP0GAFinal !=
                (u32)nMPKineticsGround) ||
            (gNdsFighterGCDrawAllLoopP1GAFinal !=
                (u32)nMPKineticsGround))
        {
            gNdsFighterGCDrawAllLoopGADriftCount++;
        }
        if ((gNdsFighterGCDrawAllLoopP0FloorOK != 1u) ||
            (gNdsFighterGCDrawAllLoopP1FloorOK != 1u))
        {
            gNdsFighterGCDrawAllLoopRootYDriftCount++;
        }
    }

    if ((gNdsFighterMarioFoxGCRunAllLoopResult ==
            NDS_FIGHTER_MARIOFOX_GCRUNALL_LOOP_PASS) &&
        (gNdsFighterMarioFoxGCRunAllLoopSafeResult ==
            NDS_FIGHTER_MARIOFOX_GCRUNALL_LOOP_SAFE_PASS))
    {
        mask |= 1u << 0;
    }
    if ((gNdsControllerPlaybackEnabled == 1u) &&
        ((gNdsControllerPlaybackConnectedMask & 0x3u) == 0x3u) &&
        (gNdsControllerPlaybackFrameCount > 0u) &&
        (gNdsControllerPlaybackReadCount > 0u) &&
        (gNdsControllerLiveReadCount == 0u) &&
        (gNdsFighterGCDrawAllLoopSYReadCount ==
            gNdsFighterGCDrawAllLoopSYUpdateCount))
    {
        mask |= 1u << 1;
    }
    if ((gNdsFighterGCDrawAllLoopPrepared == 1u) &&
        (gNdsFighterGCDrawAllLoopTaskmanUpdateCount > 0u) &&
        (gNdsFighterGCDrawAllLoopVSBattleUpdateCount > 0u) &&
        (gNdsFighterGCDrawAllLoopRunAllCount > 0u) &&
        (gNdsFighterGCDrawAllLoopDrawAllCount > 0u))
    {
        mask |= 1u << 2;
    }
    if ((gNdsFighterGCDrawAllLoopP0ProcessAttachCount == 1u) &&
        (gNdsFighterGCDrawAllLoopP1ProcessAttachCount == 1u) &&
        (gNdsFighterGCDrawAllLoopP0GObjProcessRunCount > 0u) &&
        (gNdsFighterGCDrawAllLoopP1GObjProcessRunCount > 0u) &&
        (gNdsFighterGCDrawAllLoopP0ProcCallbackCount ==
            gNdsFighterGCDrawAllLoopP0GObjProcessRunCount) &&
        (gNdsFighterGCDrawAllLoopP1ProcCallbackCount ==
            gNdsFighterGCDrawAllLoopP1GObjProcessRunCount))
    {
        mask |= 1u << 3;
    }
    if ((gNdsFighterGCDrawAllLoopP0PlaybackApplyCount > 0u) &&
        (gNdsFighterGCDrawAllLoopP1PlaybackApplyCount > 0u) &&
        (gNdsFighterGCDrawAllLoopP0ControllerToFTInputCount > 0u) &&
        (gNdsFighterGCDrawAllLoopP1ControllerToFTInputCount > 0u) &&
        (gNdsFighterGCDrawAllLoopP0DirectFTInputWriteCount == 0u) &&
        (gNdsFighterGCDrawAllLoopP1DirectFTInputWriteCount == 0u) &&
        (gNdsFighterGCDrawAllLoopP0ButtonTapMask != 0u) &&
        (gNdsFighterGCDrawAllLoopP1ButtonTapMask != 0u) &&
        (gNdsFighterGCDrawAllLoopP0ButtonHoldMask != 0u) &&
        (gNdsFighterGCDrawAllLoopP1ButtonHoldMask != 0u) &&
        (gNdsFighterGCDrawAllLoopP0DashTapEligibleCount > 0u) &&
        (gNdsFighterGCDrawAllLoopP1DashTapEligibleCount > 0u) &&
        (gNdsFighterGCDrawAllLoopP0JumpButtonTapCount > 0u) &&
        (gNdsFighterGCDrawAllLoopP1JumpButtonTapCount > 0u))
    {
        mask |= 1u << 4;
    }
    if ((gNdsFighterGCDrawAllLoopP0Completed == 1u) &&
        (gNdsFighterGCDrawAllLoopP1Completed == 1u) &&
        (gNdsFighterGCDrawAllLoopP0FrameCount > 0u) &&
        (gNdsFighterGCDrawAllLoopP1FrameCount > 0u) &&
        (gNdsFighterGCDrawAllLoopP0FrameCount <=
            gNdsFighterGCDrawAllLoopFrameMax) &&
        (gNdsFighterGCDrawAllLoopP1FrameCount <=
            gNdsFighterGCDrawAllLoopFrameMax))
    {
        mask |= 1u << 5;
    }
    if (((gNdsFighterGCDrawAllLoopP0StatusVisitMask &
            NDS_FIGHTER_GCDRAWALL_LOOP_STATUS_MASK_REQUIRED) ==
            NDS_FIGHTER_GCDRAWALL_LOOP_STATUS_MASK_REQUIRED) &&
        ((gNdsFighterGCDrawAllLoopP1StatusVisitMask &
            NDS_FIGHTER_GCDRAWALL_LOOP_STATUS_MASK_REQUIRED) ==
            NDS_FIGHTER_GCDRAWALL_LOOP_STATUS_MASK_REQUIRED))
    {
        mask |= 1u << 6;
    }
    if (((gNdsFighterGCDrawAllLoopP0TransitionMask &
            NDS_FIGHTER_GCDRAWALL_LOOP_TRANSITION_MASK_REQUIRED) ==
            NDS_FIGHTER_GCDRAWALL_LOOP_TRANSITION_MASK_REQUIRED) &&
        ((gNdsFighterGCDrawAllLoopP1TransitionMask &
            NDS_FIGHTER_GCDRAWALL_LOOP_TRANSITION_MASK_REQUIRED) ==
            NDS_FIGHTER_GCDRAWALL_LOOP_TRANSITION_MASK_REQUIRED))
    {
        mask |= 1u << 7;
    }
    if ((gNdsFighterGCDrawAllLoopP0InterruptCount > 0u) &&
        (gNdsFighterGCDrawAllLoopP1InterruptCount > 0u) &&
        (gNdsFighterGCDrawAllLoopP0PhysicsCount > 0u) &&
        (gNdsFighterGCDrawAllLoopP1PhysicsCount > 0u) &&
        (gNdsFighterGCDrawAllLoopP0IntegrateCount > 0u) &&
        (gNdsFighterGCDrawAllLoopP1IntegrateCount > 0u) &&
        (gNdsFighterGCDrawAllLoopP0MapCount > 0u) &&
        (gNdsFighterGCDrawAllLoopP1MapCount > 0u))
    {
        mask |= 1u << 8;
    }
    if ((gNdsFighterGCDrawAllLoopP0RootDeltaXMilli != 0) &&
        (gNdsFighterGCDrawAllLoopP1RootDeltaXMilli != 0) &&
        (gNdsFighterGCDrawAllLoopP0RootRiseMilli > 0) &&
        (gNdsFighterGCDrawAllLoopP1RootRiseMilli > 0) &&
        (gNdsFighterGCDrawAllLoopP0RootDirectionOK == 1u) &&
        (gNdsFighterGCDrawAllLoopP1RootDirectionOK == 1u))
    {
        mask |= 1u << 9;
    }
    if ((gNdsFighterGCDrawAllLoopPreviewReady != 0u) &&
        (gNdsFighterGCDrawAllLoopPreviewCommitDelta >=
            NDS_FIGHTER_GCDRAWALL_LOOP_DRAW_FRAME_MIN) &&
        (gNdsFighterGCDrawAllLoopDrawFrameCount >=
            NDS_FIGHTER_GCDRAWALL_LOOP_DRAW_FRAME_MIN) &&
        (gNdsFighterGCDrawAllLoopDrawAllCount >=
            NDS_FIGHTER_GCDRAWALL_LOOP_DRAW_FRAME_MIN) &&
        (gNdsFighterGCDrawAllLoopCameraCallbackCount > 0u) &&
        (gNdsFighterGCDrawAllLoopDisplayCallbackCount >=
            (NDS_FIGHTER_GCDRAWALL_LOOP_DRAW_FRAME_MIN * 2u)) &&
        (gNdsFighterGCDrawAllLoopCapturedDisplayCount >=
            gNdsFighterGCDrawAllLoopDisplayCallbackCount) &&
        (gNdsFighterGCDrawAllLoopP0CandidateCount >= 14u) &&
        (gNdsFighterGCDrawAllLoopP1CandidateCount >= 18u) &&
        (gNdsFighterGCDrawAllLoopP0PixelCount > 0u) &&
        (gNdsFighterGCDrawAllLoopP1PixelCount > 0u) &&
        (gNdsFighterGCDrawAllLoopP0ColorChecksum != 0u) &&
        (gNdsFighterGCDrawAllLoopP1ColorChecksum != 0u) &&
        (gNdsFighterGCDrawAllLoopP0ScreenXDelta != 0) &&
        (gNdsFighterGCDrawAllLoopP1ScreenXDelta != 0) &&
        (gNdsFighterGCDrawAllLoopP0ScreenRise > 0) &&
        (gNdsFighterGCDrawAllLoopP1ScreenRise > 0))
    {
        mask |= 1u << 10;
    }
    if ((gNdsFighterGCDrawAllLoopP0StatusFinal ==
            (u32)nFTCommonStatusWait) &&
        (gNdsFighterGCDrawAllLoopP1StatusFinal ==
            (u32)nFTCommonStatusWait) &&
        (gNdsFighterGCDrawAllLoopP0MotionFinal ==
            (u32)nFTCommonMotionWait) &&
        (gNdsFighterGCDrawAllLoopP1MotionFinal ==
            (u32)nFTCommonMotionWait) &&
        (gNdsFighterGCDrawAllLoopP0GAFinal == (u32)nMPKineticsGround) &&
        (gNdsFighterGCDrawAllLoopP1GAFinal == (u32)nMPKineticsGround) &&
        (gNdsFighterGCDrawAllLoopP0FloorOK == 1u) &&
        (gNdsFighterGCDrawAllLoopP1FloorOK == 1u))
    {
        mask |= 1u << 11;
    }

    gNdsFighterGCDrawAllLoopGObjCountAfter = (u32)gcGetGObjsActiveNum();
    gNdsFighterGCDrawAllLoopGObjDelta =
        (gNdsFighterGCDrawAllLoopGObjCountAfter >=
         gNdsFighterGCDrawAllLoopGObjCountBefore) ?
        (gNdsFighterGCDrawAllLoopGObjCountAfter -
         gNdsFighterGCDrawAllLoopGObjCountBefore) :
        (gNdsFighterGCDrawAllLoopGObjCountBefore -
         gNdsFighterGCDrawAllLoopGObjCountAfter);

    if ((gNdsFighterGCDrawAllLoopGObjDelta == 0u) &&
        (gNdsFighterGCDrawAllLoopUnexpectedStatusCount == 0u) &&
        (gNdsFighterGCDrawAllLoopDeniedStatusCount == 0u) &&
        (gNdsFighterGCDrawAllLoopProcessAttachEscapeCount == 0u) &&
        (gNdsFighterGCDrawAllLoopDisplayProbeCount == 0u) &&
        (gNdsFighterGCDrawAllLoopGameplayUpdateCount == 0u) &&
        (gNdsFighterGCDrawAllLoopDrawCallCount == 0u) &&
        (gNdsFighterGCDrawAllLoopMatrixCallCount == 0u) &&
        (gNdsFighterGCDrawAllLoopRootYDriftCount == 0u) &&
        (gNdsFighterGCDrawAllLoopGADriftCount == 0u) &&
        (gNdsFighterGCDrawAllLoopOldProcessPauseCount > 0u) &&
        (gNdsFighterGCDrawAllLoopNonTargetGObjVisitCount > 0u) &&
        (gNdsFighterGCDrawAllLoopTargetProcessPreserveCount >= 2u))
    {
        mask |= 1u << 12;
    }

    gNdsFighterMarioFoxGCDrawAllLoopMask = mask;
    gNdsFighterMarioFoxGCDrawAllLoopDeferredMask = 0xffu;
    gNdsFighterMarioFoxGCDrawAllLoopCount =
        gNdsFighterGCDrawAllLoopP0Completed +
        gNdsFighterGCDrawAllLoopP1Completed;

    if ((mask & 0x1fffu) == 0x1fffu)
    {
        gNdsFighterMarioFoxGCDrawAllLoopResult =
            NDS_FIGHTER_MARIOFOX_GCDRAWALL_LOOP_PASS;
        gNdsFighterMarioFoxGCDrawAllLoopSafeResult =
            NDS_FIGHTER_MARIOFOX_GCDRAWALL_LOOP_SAFE_PASS;
    }
    ndsFighterMarioFoxStageGCDrawAllLoopFinalize();
}

static void ndsFighterLivePreviewResetDiagnostics(void)
{
    gNdsFighterMarioFoxLivePreviewResult = 0u;
    gNdsFighterMarioFoxLivePreviewSafeResult = 0u;
    gNdsFighterMarioFoxLivePreviewMask = 0u;
    gNdsFighterMarioFoxLivePreviewDeferredMask = 0u;
    gNdsFighterMarioFoxLivePreviewCount = 0u;
    gNdsFighterLivePreviewPrepared = 0u;
    gNdsFighterLivePreviewDevMode =
        (NDS_DEV_LIVE_INPUT_PREVIEW != 0) ? 1u : 0u;
    gNdsFighterLivePreviewIdleFrameTarget =
        NDS_FIGHTER_LIVE_PREVIEW_IDLE_FRAME_TARGET;
    gNdsFighterLivePreviewFrameMax =
        (NDS_DEV_LIVE_INPUT_PREVIEW != 0) ?
        NDS_FIGHTER_LIVE_PREVIEW_DEV_FRAME_TARGET :
        NDS_FIGHTER_LIVE_PREVIEW_IDLE_FRAME_TARGET;
    gNdsFighterLivePreviewUpdateMax = gNdsFighterLivePreviewFrameMax;
    gNdsFighterLivePreviewTaskmanUpdateCount = 0u;
    gNdsFighterLivePreviewVSBattleUpdateCount = 0u;
    gNdsFighterLivePreviewBaseVSBattleUpdateCount = 0u;
    gNdsFighterLivePreviewRunAllCount = 0u;
    gNdsFighterLivePreviewManualGObjProcessRunCount = 0u;
    gNdsFighterLivePreviewSYReadCount = 0u;
    gNdsFighterLivePreviewSYUpdateCount = 0u;
    gNdsFighterLivePreviewLiveReadBefore = gNdsControllerLiveReadCount;
    gNdsFighterLivePreviewLiveReadAfter = gNdsControllerLiveReadCount;
    gNdsFighterLivePreviewLiveReadDelta = 0u;
    gNdsFighterLivePreviewPlaybackReadBefore =
        gNdsControllerPlaybackReadCount;
    gNdsFighterLivePreviewPlaybackReadAfter =
        gNdsControllerPlaybackReadCount;
    gNdsFighterLivePreviewPlaybackReadDelta = 0u;
    gNdsFighterLivePreviewControllerLiveConnectedMask = 0u;
    gNdsFighterLivePreviewAnyInputSeen = 0u;
    gNdsFighterLivePreviewGObjCountBefore = 0u;
    gNdsFighterLivePreviewGObjCountAfter = 0u;
    gNdsFighterLivePreviewGObjDelta = 0u;
    gNdsFighterLivePreviewOldProcessPauseCount = 0u;
    gNdsFighterLivePreviewNonTargetGObjVisitCount = 0u;
    gNdsFighterLivePreviewNonTargetProcessPauseCount = 0u;
    gNdsFighterLivePreviewNonTargetProcCallbackCount = 0u;
    gNdsFighterLivePreviewTargetProcessPreserveCount = 0u;
    gNdsFighterLivePreviewP0ProcessAttachCount = 0u;
    gNdsFighterLivePreviewP1ProcessAttachCount = 0u;
    gNdsFighterLivePreviewProcessAttachEscapeCount = 0u;
    gNdsFighterLivePreviewP0GObjProcessRunCount = 0u;
    gNdsFighterLivePreviewP1GObjProcessRunCount = 0u;
    gNdsFighterLivePreviewP0ProcCallbackCount = 0u;
    gNdsFighterLivePreviewP1ProcCallbackCount = 0u;
    gNdsFighterLivePreviewP0PlaybackApplyCount = 0u;
    gNdsFighterLivePreviewP1PlaybackApplyCount = 0u;
    gNdsFighterLivePreviewP0ControllerToFTInputCount = 0u;
    gNdsFighterLivePreviewP1ControllerToFTInputCount = 0u;
    gNdsFighterLivePreviewP0DirectFTInputWriteCount = 0u;
    gNdsFighterLivePreviewP1DirectFTInputWriteCount = 0u;
    gNdsFighterLivePreviewP0ButtonTapMask = 0u;
    gNdsFighterLivePreviewP1ButtonTapMask = 0u;
    gNdsFighterLivePreviewP0ButtonHoldMask = 0u;
    gNdsFighterLivePreviewP1ButtonHoldMask = 0u;
    gNdsFighterLivePreviewP0LastStickX = 0;
    gNdsFighterLivePreviewP1LastStickX = 0;
    gNdsFighterLivePreviewP0LastStickY = 0;
    gNdsFighterLivePreviewP1LastStickY = 0;
    gNdsFighterLivePreviewP0DashTapEligibleCount = 0u;
    gNdsFighterLivePreviewP1DashTapEligibleCount = 0u;
    gNdsFighterLivePreviewP0JumpButtonTapCount = 0u;
    gNdsFighterLivePreviewP1JumpButtonTapCount = 0u;
    gNdsFighterLivePreviewP0FrameCount = 0u;
    gNdsFighterLivePreviewP1FrameCount = 0u;
    gNdsFighterLivePreviewP0Completed = 0u;
    gNdsFighterLivePreviewP1Completed = 0u;
    gNdsFighterLivePreviewP0StatusVisitMask = 0u;
    gNdsFighterLivePreviewP1StatusVisitMask = 0u;
    gNdsFighterLivePreviewP0TransitionMask = 0u;
    gNdsFighterLivePreviewP1TransitionMask = 0u;
    gNdsFighterLivePreviewP0StatusStart = 0xffffffffu;
    gNdsFighterLivePreviewP1StatusStart = 0xffffffffu;
    gNdsFighterLivePreviewP0MotionStart = 0xffffffffu;
    gNdsFighterLivePreviewP1MotionStart = 0xffffffffu;
    gNdsFighterLivePreviewP0StatusFinal = 0xffffffffu;
    gNdsFighterLivePreviewP1StatusFinal = 0xffffffffu;
    gNdsFighterLivePreviewP0MotionFinal = 0xffffffffu;
    gNdsFighterLivePreviewP1MotionFinal = 0xffffffffu;
    gNdsFighterLivePreviewP0GAFinal = 0xffffffffu;
    gNdsFighterLivePreviewP1GAFinal = 0xffffffffu;
    gNdsFighterLivePreviewP0RootXStartMilli = 0;
    gNdsFighterLivePreviewP1RootXStartMilli = 0;
    gNdsFighterLivePreviewP0RootDeltaXMilli = 0;
    gNdsFighterLivePreviewP1RootDeltaXMilli = 0;
    gNdsFighterLivePreviewP0RootYFinalMilli = 0;
    gNdsFighterLivePreviewP1RootYFinalMilli = 0;
    gNdsFighterLivePreviewP0FloorYMilli = 0;
    gNdsFighterLivePreviewP1FloorYMilli = 0;
    gNdsFighterLivePreviewP0FloorOK = 0u;
    gNdsFighterLivePreviewP1FloorOK = 0u;
    gNdsFighterLivePreviewP0InterruptCount = 0u;
    gNdsFighterLivePreviewP1InterruptCount = 0u;
    gNdsFighterLivePreviewP0PhysicsCount = 0u;
    gNdsFighterLivePreviewP1PhysicsCount = 0u;
    gNdsFighterLivePreviewP0IntegrateCount = 0u;
    gNdsFighterLivePreviewP1IntegrateCount = 0u;
    gNdsFighterLivePreviewP0MapCount = 0u;
    gNdsFighterLivePreviewP1MapCount = 0u;
    gNdsFighterLivePreviewPreviewWidth = 0u;
    gNdsFighterLivePreviewPreviewHeight = 0u;
    gNdsFighterLivePreviewPreviewPitch = 0u;
    gNdsFighterLivePreviewPreviewReady = 0u;
    gNdsFighterLivePreviewPreviewCommitBefore = 0u;
    gNdsFighterLivePreviewPreviewCommitAfter = 0u;
    gNdsFighterLivePreviewPreviewCommitDelta = 0u;
    gNdsFighterLivePreviewDrawFrameCount = 0u;
    gNdsFighterLivePreviewDisplayCallbackCount = 0u;
    gNdsFighterLivePreviewP0DisplayCallbackCount = 0u;
    gNdsFighterLivePreviewP1DisplayCallbackCount = 0u;
    gNdsFighterLivePreviewP0CandidateCount = 0u;
    gNdsFighterLivePreviewP1CandidateCount = 0u;
    gNdsFighterLivePreviewP0DrawnDObjCount = 0u;
    gNdsFighterLivePreviewP1DrawnDObjCount = 0u;
    gNdsFighterLivePreviewP0PixelCount = 0u;
    gNdsFighterLivePreviewP1PixelCount = 0u;
    gNdsFighterLivePreviewTotalPixelCount = 0u;
    gNdsFighterLivePreviewP0ColorChecksum = 0u;
    gNdsFighterLivePreviewP1ColorChecksum = 0u;
    gNdsFighterLivePreviewUnexpectedStatusCount = 0u;
    gNdsFighterLivePreviewDeniedStatusCount = 0u;
    gNdsFighterLivePreviewDisplayProbeCount = 0u;
    gNdsFighterLivePreviewGameplayUpdateCount = 0u;
    gNdsFighterLivePreviewDrawCallCount = 0u;
    gNdsFighterLivePreviewMatrixCallCount = 0u;
    gNdsFighterLivePreviewRootYDriftCount = 0u;
    gNdsFighterLivePreviewGADriftCount = 0u;
}

static void ndsFighterLivePreviewPauseProofOwnedProcesses(void)
{
    u32 i;

    for (i = 0u; i < 2u; i++)
    {
        if (sNdsFighterGCRunAllLoopProcesses[i] != NULL)
        {
            gcPauseGObjProcess(sNdsFighterGCRunAllLoopProcesses[i]);
            gNdsFighterLivePreviewOldProcessPauseCount++;
        }
    }
}

static void ndsFighterLivePreviewPauseNonTargetGObjVisitor(GObj *gobj,
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
        gNdsFighterLivePreviewTargetProcessPreserveCount++;
        return;
    }
    gNdsFighterLivePreviewNonTargetGObjVisitCount++;
    if (gobj->gobjproc_head != NULL)
    {
        gcPauseGObjProcessAll(gobj);
        gNdsFighterLivePreviewNonTargetProcessPauseCount++;
    }
    gobj->flags |= GOBJ_FLAG_NORUN;
}

static void ndsFighterLivePreviewApplyFromSYController(u32 slot,
                                                       FTStruct *fp)
{
    NDSFighterLivePreviewState *state;
    SYController *controller;
    s32 stick_x_abs;
    s32 stick_y_abs;
    s32 previous_x_abs;
    s32 previous_y_abs;

    if ((slot >= 2u) || (slot >= MAXCONTROLLERS) || (fp == NULL))
    {
        return;
    }
    state = &sNdsFighterLivePreviewStates[slot];
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
            gNdsFighterLivePreviewP0DashTapEligibleCount++;
        }
        else
        {
            gNdsFighterLivePreviewP1DashTapEligibleCount++;
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

    if ((controller->stick_range.x != 0) ||
        (controller->stick_range.y != 0) ||
        (controller->button_hold != 0u) ||
        (controller->button_tap != 0u) ||
        (controller->button_release != 0u))
    {
        gNdsFighterLivePreviewAnyInputSeen = 1u;
    }

    if (slot == 0u)
    {
        gNdsFighterLivePreviewP0ControllerToFTInputCount++;
        gNdsFighterLivePreviewP0ButtonTapMask |= controller->button_tap;
        gNdsFighterLivePreviewP0ButtonHoldMask |= controller->button_hold;
        gNdsFighterLivePreviewP0LastStickX = controller->stick_range.x;
        gNdsFighterLivePreviewP0LastStickY = controller->stick_range.y;
        if ((controller->button_tap & U_CBUTTONS) != 0u)
        {
            gNdsFighterLivePreviewP0JumpButtonTapCount++;
        }
    }
    else
    {
        gNdsFighterLivePreviewP1ControllerToFTInputCount++;
        gNdsFighterLivePreviewP1ButtonTapMask |= controller->button_tap;
        gNdsFighterLivePreviewP1ButtonHoldMask |= controller->button_hold;
        gNdsFighterLivePreviewP1LastStickX = controller->stick_range.x;
        gNdsFighterLivePreviewP1LastStickY = controller->stick_range.y;
        if ((controller->button_tap & U_CBUTTONS) != 0u)
        {
            gNdsFighterLivePreviewP1JumpButtonTapCount++;
        }
    }
}

static void ndsFighterLivePreviewRecordState(u32 slot, FTStruct *fp,
                                             s32 previous_status,
                                             s32 previous_ga)
{
    NDSFighterLivePreviewState *state;
    u32 transition_bit;

    if ((slot >= 2u) || (fp == NULL))
    {
        return;
    }
    state = &sNdsFighterLivePreviewStates[slot];
    state->status_visit_mask |= ndsFighterProcessLoopStatusBit(fp->status_id);
    transition_bit = ndsFighterProcessLoopTransitionBit(previous_status,
                                                       fp->status_id);
    state->transition_mask |= transition_bit;
    if (fp->status_id == nFTCommonStatusWait)
    {
        state->wait_visit_count++;
    }
    else if (NDS_DEV_LIVE_INPUT_PREVIEW == 0)
    {
        gNdsFighterLivePreviewUnexpectedStatusCount++;
    }
    if (previous_ga != fp->ga)
    {
        gNdsFighterLivePreviewGADriftCount++;
    }
    if (slot == 0u)
    {
        gNdsFighterLivePreviewP0StatusVisitMask = state->status_visit_mask;
        gNdsFighterLivePreviewP0TransitionMask = state->transition_mask;
    }
    else
    {
        gNdsFighterLivePreviewP1StatusVisitMask = state->status_visit_mask;
        gNdsFighterLivePreviewP1TransitionMask = state->transition_mask;
    }
}

static void ndsFighterLivePreviewRunSlotProcess(u32 slot, FTStruct *fp)
{
    NDSFighterLivePreviewState *state;
    s32 previous_status;
    s32 previous_ga;

    if ((slot >= 2u) || (fp == NULL) || (fp->fighter_gobj == NULL))
    {
        gNdsFighterLivePreviewProcessAttachEscapeCount++;
        return;
    }
    state = &sNdsFighterLivePreviewStates[slot];
    if (state->total_frames >= gNdsFighterLivePreviewFrameMax)
    {
        state->completed = 1u;
        return;
    }
    previous_status = fp->status_id;
    previous_ga = fp->ga;

    ndsFighterLivePreviewApplyFromSYController(slot, fp);
    sNdsFighterLivePreviewActive = TRUE;
    sNdsFighterProcessLoopActive = TRUE;
    ndsFighterProcessLoopRunFrame(slot, fp);
    sNdsFighterProcessLoopActive = FALSE;
    sNdsFighterLivePreviewActive = FALSE;
    ndsFighterLivePreviewRecordState(slot, fp, previous_status, previous_ga);
    state->total_frames++;
    if ((NDS_DEV_LIVE_INPUT_PREVIEW == 0) &&
        (state->total_frames >= NDS_FIGHTER_LIVE_PREVIEW_IDLE_FRAME_TARGET))
    {
        state->completed = 1u;
    }
    if (slot == 0u)
    {
        gNdsFighterLivePreviewP0FrameCount = state->total_frames;
        gNdsFighterLivePreviewP0Completed = state->completed;
        gNdsFighterLivePreviewP0InterruptCount++;
        gNdsFighterLivePreviewP0PhysicsCount++;
        gNdsFighterLivePreviewP0IntegrateCount++;
        gNdsFighterLivePreviewP0MapCount++;
    }
    else
    {
        gNdsFighterLivePreviewP1FrameCount = state->total_frames;
        gNdsFighterLivePreviewP1Completed = state->completed;
        gNdsFighterLivePreviewP1InterruptCount++;
        gNdsFighterLivePreviewP1PhysicsCount++;
        gNdsFighterLivePreviewP1IntegrateCount++;
        gNdsFighterLivePreviewP1MapCount++;
    }
}

static void ndsFighterLivePreviewGObjProc(GObj *fighter_gobj)
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
        gNdsFighterLivePreviewProcessAttachEscapeCount++;
        return;
    }
    if (slot == 0u)
    {
        gNdsFighterLivePreviewP0ProcCallbackCount++;
        gNdsFighterLivePreviewP0GObjProcessRunCount++;
    }
    else
    {
        gNdsFighterLivePreviewP1ProcCallbackCount++;
        gNdsFighterLivePreviewP1GObjProcessRunCount++;
    }
    ndsFighterLivePreviewRunSlotProcess(slot, fp);
}

static void ndsFighterLivePreviewRecordStart(u32 slot, FTStruct *fp,
                                             DObj *root)
{
    if ((slot >= 2u) || (fp == NULL) || (root == NULL))
    {
        return;
    }
    sNdsFighterLivePreviewStates[slot].root_y_start =
        root->translate.vec.f.y;
    sNdsFighterLivePreviewStates[slot].status_visit_mask =
        ndsFighterProcessLoopStatusBit(fp->status_id);
    fp->tap_stick_x = FTINPUT_STICKBUFFER_TICS_MAX;
    fp->tap_stick_y = FTINPUT_STICKBUFFER_TICS_MAX;
    fp->hold_stick_x = FTINPUT_STICKBUFFER_TICS_MAX;
    fp->hold_stick_y = FTINPUT_STICKBUFFER_TICS_MAX;

    if (slot == 0u)
    {
        gNdsFighterLivePreviewP0StatusStart = (u32)fp->status_id;
        gNdsFighterLivePreviewP0MotionStart = (u32)fp->motion_id;
        gNdsFighterLivePreviewP0RootXStartMilli =
            ndsFloatToMilliSigned(root->translate.vec.f.x);
        gNdsFighterLivePreviewP0FloorYMilli =
            ndsFloatToMilliSigned(fp->coll_data.floor_dist);
    }
    else
    {
        gNdsFighterLivePreviewP1StatusStart = (u32)fp->status_id;
        gNdsFighterLivePreviewP1MotionStart = (u32)fp->motion_id;
        gNdsFighterLivePreviewP1RootXStartMilli =
            ndsFloatToMilliSigned(root->translate.vec.f.x);
        gNdsFighterLivePreviewP1FloorYMilli =
            ndsFloatToMilliSigned(fp->coll_data.floor_dist);
    }
}

static void ndsFighterLivePreviewRecordFinal(u32 slot, FTStruct *fp,
                                             DObj *root)
{
    s32 root_y_final;
    s32 floor_y;
    s32 root_delta;

    if ((slot >= 2u) || (fp == NULL) || (root == NULL))
    {
        return;
    }
    root_y_final = ndsFloatToMilliSigned(root->translate.vec.f.y);
    floor_y = ndsFloatToMilliSigned(fp->coll_data.floor_dist);
    root_delta = ndsFloatToMilliSigned(root->translate.vec.f.x) -
        ((slot == 0u) ? gNdsFighterLivePreviewP0RootXStartMilli :
            gNdsFighterLivePreviewP1RootXStartMilli);

    if (slot == 0u)
    {
        gNdsFighterLivePreviewP0StatusFinal = (u32)fp->status_id;
        gNdsFighterLivePreviewP0MotionFinal = (u32)fp->motion_id;
        gNdsFighterLivePreviewP0GAFinal = (u32)fp->ga;
        gNdsFighterLivePreviewP0RootYFinalMilli = root_y_final;
        gNdsFighterLivePreviewP0RootDeltaXMilli = root_delta;
        gNdsFighterLivePreviewP0FloorOK =
            (root_y_final == floor_y) ? 1u : 0u;
    }
    else
    {
        gNdsFighterLivePreviewP1StatusFinal = (u32)fp->status_id;
        gNdsFighterLivePreviewP1MotionFinal = (u32)fp->motion_id;
        gNdsFighterLivePreviewP1GAFinal = (u32)fp->ga;
        gNdsFighterLivePreviewP1RootYFinalMilli = root_y_final;
        gNdsFighterLivePreviewP1RootDeltaXMilli = root_delta;
        gNdsFighterLivePreviewP1FloorOK =
            (root_y_final == floor_y) ? 1u : 0u;
    }
}

static void ndsFighterLivePreviewCopyDrawFromPreview(void)
{
    gNdsFighterLivePreviewPreviewWidth =
        gNdsFighterPreviewLoopPreviewWidth;
    gNdsFighterLivePreviewPreviewHeight =
        gNdsFighterPreviewLoopPreviewHeight;
    gNdsFighterLivePreviewPreviewPitch =
        gNdsFighterPreviewLoopPreviewPitch;
    gNdsFighterLivePreviewPreviewReady =
        gNdsFighterPreviewLoopPreviewReady;
    gNdsFighterLivePreviewPreviewCommitBefore =
        gNdsFighterPreviewLoopPreviewCommitBefore;
    gNdsFighterLivePreviewPreviewCommitAfter =
        gNdsFighterPreviewLoopPreviewCommitAfter;
    gNdsFighterLivePreviewPreviewCommitDelta =
        gNdsFighterPreviewLoopPreviewCommitDelta;
    gNdsFighterLivePreviewDrawFrameCount =
        gNdsFighterPreviewLoopDrawFrameCount;
    gNdsFighterLivePreviewDisplayCallbackCount =
        gNdsFighterPreviewLoopDisplayCallbackCount;
    gNdsFighterLivePreviewP0DisplayCallbackCount =
        gNdsFighterPreviewLoopP0DisplayCallbackCount;
    gNdsFighterLivePreviewP1DisplayCallbackCount =
        gNdsFighterPreviewLoopP1DisplayCallbackCount;
    gNdsFighterLivePreviewP0CandidateCount =
        gNdsFighterPreviewLoopP0CandidateCount;
    gNdsFighterLivePreviewP1CandidateCount =
        gNdsFighterPreviewLoopP1CandidateCount;
    gNdsFighterLivePreviewP0DrawnDObjCount =
        gNdsFighterPreviewLoopP0DrawnDObjCount;
    gNdsFighterLivePreviewP1DrawnDObjCount =
        gNdsFighterPreviewLoopP1DrawnDObjCount;
    gNdsFighterLivePreviewP0PixelCount =
        gNdsFighterPreviewLoopP0PixelCount;
    gNdsFighterLivePreviewP1PixelCount =
        gNdsFighterPreviewLoopP1PixelCount;
    gNdsFighterLivePreviewTotalPixelCount =
        gNdsFighterPreviewLoopTotalPixelCount;
    gNdsFighterLivePreviewP0ColorChecksum =
        gNdsFighterPreviewLoopP0ColorChecksum;
    gNdsFighterLivePreviewP1ColorChecksum =
        gNdsFighterPreviewLoopP1ColorChecksum;
}

static void ndsFighterLivePreviewDrawKeyframe(void)
{
    ndsFighterPreviewLoopDrawKeyframe();
    ndsFighterLivePreviewCopyDrawFromPreview();
}

void ndsFighterMarioFoxLivePreviewPrepare(void)
{
    u32 i;

    if ((ndsFighterMarioFoxLivePreviewProofEnabled() == FALSE) ||
        (gNdsFighterLivePreviewPrepared != 0u))
    {
        return;
    }
    if ((gNdsFighterMarioFoxGCRunAllLoopResult !=
            NDS_FIGHTER_MARIOFOX_GCRUNALL_LOOP_PASS) ||
        (gNdsFighterMarioFoxGCRunAllLoopSafeResult !=
            NDS_FIGHTER_MARIOFOX_GCRUNALL_LOOP_SAFE_PASS) ||
        ((gNdsFighterMarioFoxGCRunAllLoopMask & 0x7ffu) != 0x7ffu) ||
        (gNdsFighterMarioFoxGCRunAllLoopDeferredMask != 0xffu) ||
        (gNdsFighterMarioFoxGCRunAllLoopCount != 2u) ||
        (gNdsFighterGCRunAllLoopP0StatusFinal !=
            (u32)nFTCommonStatusWait) ||
        (gNdsFighterGCRunAllLoopP1StatusFinal !=
            (u32)nFTCommonStatusWait) ||
        (gNdsFighterGCRunAllLoopP0MotionFinal !=
            (u32)nFTCommonMotionWait) ||
        (gNdsFighterGCRunAllLoopP1MotionFinal !=
            (u32)nFTCommonMotionWait) ||
        (gNdsFighterGCRunAllLoopP0GAFinal != (u32)nMPKineticsGround) ||
        (gNdsFighterGCRunAllLoopP1GAFinal != (u32)nMPKineticsGround))
    {
        return;
    }

    bzero(sNdsFighterLivePreviewStates,
          sizeof(sNdsFighterLivePreviewStates));
    bzero(sNdsFighterLivePreviewProcesses,
          sizeof(sNdsFighterLivePreviewProcesses));
    bzero(sNdsFighterPreviewLoopStates,
          sizeof(sNdsFighterPreviewLoopStates));
    ndsControllerPlaybackReset();
    ndsControllerPlaybackSetConnectedMask(0u);
    ndsControllerPlaybackSetEnabled(FALSE);
    ndsFighterLivePreviewResetDiagnostics();
    gNdsFighterLivePreviewGObjCountBefore = (u32)gcGetGObjsActiveNum();

    ndsFighterLivePreviewPauseProofOwnedProcesses();
    gcFuncGObjAll(ndsFighterLivePreviewPauseNonTargetGObjVisitor, 0u);

    for (i = 0u; i < 2u; i++)
    {
        FTStruct *fp = &sNdsFighterStructPool[i];
        GObj *fighter_gobj = fp->fighter_gobj;
        DObj *root = fp->joints[nFTPartsJointTopN];

        if ((ndsFighterStructIsPoolPointer(fp) == FALSE) ||
            (fighter_gobj == NULL) || (root == NULL))
        {
            gNdsFighterLivePreviewProcessAttachEscapeCount++;
            continue;
        }
        fighter_gobj->flags &= ~GOBJ_FLAG_NORUN;
        ndsFighterLivePreviewRecordStart(i, fp, root);
        ndsFighterPreviewLoopRecordStart(i, fp, root);
        sNdsFighterLivePreviewProcesses[i] =
            gcAddGObjProcess(fighter_gobj,
                             ndsFighterLivePreviewGObjProc,
                             nGCProcessKindFunc,
                             3);
        if (sNdsFighterLivePreviewProcesses[i] == NULL)
        {
            gNdsFighterLivePreviewProcessAttachEscapeCount++;
        }
        else if (i == 0u)
        {
            gNdsFighterLivePreviewP0ProcessAttachCount++;
        }
        else
        {
            gNdsFighterLivePreviewP1ProcessAttachCount++;
        }
    }

    if ((sNdsFighterLivePreviewProcesses[0] != NULL) &&
        (sNdsFighterLivePreviewProcesses[1] != NULL))
    {
        gNdsFighterLivePreviewPrepared = 1u;
    }
}

s32 ndsFighterMarioFoxLivePreviewUpdateEnabled(void)
{
    return ((ndsFighterMarioFoxLivePreviewProofEnabled() != FALSE) &&
            (gNdsFighterLivePreviewPrepared != 0u) &&
            (gNdsFighterMarioFoxLivePreviewResult == 0u)) ? TRUE : FALSE;
}

void ndsFighterMarioFoxLivePreviewRunVSBattleUpdate(void)
{
    u32 i;
    u32 mask = 0u;

    if (ndsFighterMarioFoxLivePreviewUpdateEnabled() == FALSE)
    {
        return;
    }
    gNdsFighterLivePreviewVSBattleUpdateCount++;

    syControllerReadDeviceData();
    gNdsFighterLivePreviewSYReadCount++;
    syControllerUpdateGlobalData();
    gNdsFighterLivePreviewSYUpdateCount++;
    gNdsFighterLivePreviewLiveReadAfter = gNdsControllerLiveReadCount;
    gNdsFighterLivePreviewPlaybackReadAfter =
        gNdsControllerPlaybackReadCount;
    gNdsFighterLivePreviewLiveReadDelta =
        gNdsFighterLivePreviewLiveReadAfter -
        gNdsFighterLivePreviewLiveReadBefore;
    gNdsFighterLivePreviewPlaybackReadDelta =
        gNdsFighterLivePreviewPlaybackReadAfter -
        gNdsFighterLivePreviewPlaybackReadBefore;
    gNdsFighterLivePreviewControllerLiveConnectedMask =
        gNdsControllerLiveConnectedMask;

    sNdsFighterLivePreviewActive = TRUE;
    gcRunAll();
    sNdsFighterLivePreviewActive = FALSE;
    gNdsFighterLivePreviewRunAllCount++;

    for (i = 0u; i < 2u; i++)
    {
        FTStruct *fp = &sNdsFighterStructPool[i];
        DObj *root = fp->joints[nFTPartsJointTopN];
        ndsFighterLivePreviewRecordFinal(i, fp, root);
    }

    if (((gNdsFighterLivePreviewVSBattleUpdateCount %
            NDS_FIGHTER_LIVE_PREVIEW_DRAW_FRAME_INTERVAL) == 0u) ||
        ((gNdsFighterLivePreviewP0Completed == 1u) &&
         (gNdsFighterLivePreviewP1Completed == 1u)))
    {
        ndsFighterLivePreviewDrawKeyframe();
    }

    if (gNdsFighterMarioFoxGCRunAllLoopResult ==
        NDS_FIGHTER_MARIOFOX_GCRUNALL_LOOP_PASS)
    {
        mask |= 1u << 0;
    }
    if ((gNdsControllerPlaybackEnabled == 0u) &&
        (gNdsFighterLivePreviewLiveReadDelta > 0u) &&
        (gNdsFighterLivePreviewPlaybackReadDelta == 0u) &&
        ((gNdsFighterLivePreviewControllerLiveConnectedMask & 1u) != 0u))
    {
        mask |= 1u << 1;
    }
    if ((gNdsFighterLivePreviewPrepared == 1u) &&
        (gNdsFighterLivePreviewTaskmanUpdateCount > 0u) &&
        (gNdsFighterLivePreviewVSBattleUpdateCount > 0u) &&
        (gNdsFighterLivePreviewRunAllCount > 0u))
    {
        mask |= 1u << 2;
    }
    if ((gNdsFighterLivePreviewP0ProcessAttachCount == 1u) &&
        (gNdsFighterLivePreviewP1ProcessAttachCount == 1u) &&
        (gNdsFighterLivePreviewP0ProcCallbackCount > 0u) &&
        (gNdsFighterLivePreviewP1ProcCallbackCount > 0u) &&
        (gNdsFighterLivePreviewManualGObjProcessRunCount == 0u))
    {
        mask |= 1u << 3;
    }
    if ((gNdsFighterLivePreviewP0ControllerToFTInputCount > 0u) &&
        (gNdsFighterLivePreviewP1ControllerToFTInputCount > 0u) &&
        (gNdsFighterLivePreviewP0DirectFTInputWriteCount == 0u) &&
        (gNdsFighterLivePreviewP1DirectFTInputWriteCount == 0u))
    {
        mask |= 1u << 4;
    }
    if ((gNdsFighterLivePreviewP0FrameCount >=
            gNdsFighterLivePreviewIdleFrameTarget) &&
        (gNdsFighterLivePreviewP1FrameCount >=
            gNdsFighterLivePreviewIdleFrameTarget))
    {
        mask |= 1u << 5;
    }
    if ((gNdsFighterLivePreviewP0StatusFinal ==
            (u32)nFTCommonStatusWait) &&
        (gNdsFighterLivePreviewP1StatusFinal ==
            (u32)nFTCommonStatusWait) &&
        (gNdsFighterLivePreviewP0MotionFinal ==
            (u32)nFTCommonMotionWait) &&
        (gNdsFighterLivePreviewP1MotionFinal ==
            (u32)nFTCommonMotionWait) &&
        (gNdsFighterLivePreviewP0GAFinal == (u32)nMPKineticsGround) &&
        (gNdsFighterLivePreviewP1GAFinal == (u32)nMPKineticsGround))
    {
        mask |= 1u << 6;
    }
    if ((gNdsFighterLivePreviewP0InterruptCount >=
            gNdsFighterLivePreviewIdleFrameTarget) &&
        (gNdsFighterLivePreviewP1InterruptCount >=
            gNdsFighterLivePreviewIdleFrameTarget) &&
        (gNdsFighterLivePreviewP0PhysicsCount >=
            gNdsFighterLivePreviewIdleFrameTarget) &&
        (gNdsFighterLivePreviewP1PhysicsCount >=
            gNdsFighterLivePreviewIdleFrameTarget) &&
        (gNdsFighterLivePreviewP0MapCount >=
            gNdsFighterLivePreviewIdleFrameTarget) &&
        (gNdsFighterLivePreviewP1MapCount >=
            gNdsFighterLivePreviewIdleFrameTarget))
    {
        mask |= 1u << 7;
    }
    if ((gNdsFighterLivePreviewPreviewReady != 0u) &&
        (gNdsFighterLivePreviewDrawFrameCount >=
            NDS_FIGHTER_LIVE_PREVIEW_DRAW_FRAME_MIN) &&
        (gNdsFighterLivePreviewP0PixelCount > 0u) &&
        (gNdsFighterLivePreviewP1PixelCount > 0u))
    {
        mask |= 1u << 8;
    }
    if ((gNdsFighterLivePreviewP0RootDeltaXMilli == 0) &&
        (gNdsFighterLivePreviewP1RootDeltaXMilli == 0) &&
        (gNdsFighterLivePreviewP0FloorOK == 1u) &&
        (gNdsFighterLivePreviewP1FloorOK == 1u))
    {
        mask |= 1u << 9;
    }

    gNdsFighterLivePreviewGObjCountAfter = (u32)gcGetGObjsActiveNum();
    gNdsFighterLivePreviewGObjDelta =
        (gNdsFighterLivePreviewGObjCountAfter >=
         gNdsFighterLivePreviewGObjCountBefore) ?
        (gNdsFighterLivePreviewGObjCountAfter -
         gNdsFighterLivePreviewGObjCountBefore) :
        (gNdsFighterLivePreviewGObjCountBefore -
         gNdsFighterLivePreviewGObjCountAfter);
    if ((gNdsFighterLivePreviewGObjDelta == 0u) &&
        (gNdsFighterLivePreviewUnexpectedStatusCount == 0u) &&
        (gNdsFighterLivePreviewDeniedStatusCount == 0u) &&
        (gNdsFighterLivePreviewProcessAttachEscapeCount == 0u) &&
        (gNdsFighterLivePreviewDisplayProbeCount == 0u) &&
        (gNdsFighterLivePreviewGameplayUpdateCount == 0u) &&
        (gNdsFighterLivePreviewDrawCallCount == 0u) &&
        (gNdsFighterLivePreviewMatrixCallCount == 0u) &&
        (gNdsFighterLivePreviewRootYDriftCount == 0u) &&
        (gNdsFighterLivePreviewGADriftCount == 0u))
    {
        mask |= 1u << 10;
    }

    if ((NDS_DEV_LIVE_INPUT_PREVIEW == 0) &&
        (gNdsFighterLivePreviewAnyInputSeen != 0u))
    {
        mask &= ~(1u << 1);
    }

    gNdsFighterMarioFoxLivePreviewMask = mask;
    gNdsFighterMarioFoxLivePreviewDeferredMask = 0xffu;
    gNdsFighterMarioFoxLivePreviewCount =
        gNdsFighterLivePreviewP0Completed +
        gNdsFighterLivePreviewP1Completed;

    if ((NDS_DEV_LIVE_INPUT_PREVIEW == 0) &&
        ((mask & 0x7ffu) == 0x7ffu))
    {
        gNdsFighterMarioFoxLivePreviewResult =
            NDS_FIGHTER_MARIOFOX_LIVE_PREVIEW_PASS;
        gNdsFighterMarioFoxLivePreviewSafeResult =
            NDS_FIGHTER_MARIOFOX_LIVE_PREVIEW_SAFE_PASS;
    }
}

static void ndsFighterMarioFoxRecordWaitTickBefore(u32 slot, FTStruct *fp,
                                                   DObj *root_dobj)
{
    u32 root_x = (root_dobj != NULL) ?
        ndsFloatBits(root_dobj->translate.vec.f.x) : 0u;
    u32 root_y = (root_dobj != NULL) ?
        ndsFloatBits(root_dobj->translate.vec.f.y) : 0u;

    if (slot == 0u)
    {
        gNdsFighterWaitTickP0StatusBefore = (u32)fp->status_id;
        gNdsFighterWaitTickP0MotionBefore = (u32)fp->motion_id;
        gNdsFighterWaitTickP0GABefore = (u32)fp->ga;
        gNdsFighterWaitTickP0RootXBeforeBits = root_x;
        gNdsFighterWaitTickP0RootYBeforeBits = root_y;
        gNdsFighterWaitTickP0VelGroundXBeforeBits =
            ndsFloatBits(fp->vel_ground.x);
    }
    else if (slot == 1u)
    {
        gNdsFighterWaitTickP1StatusBefore = (u32)fp->status_id;
        gNdsFighterWaitTickP1MotionBefore = (u32)fp->motion_id;
        gNdsFighterWaitTickP1GABefore = (u32)fp->ga;
        gNdsFighterWaitTickP1RootXBeforeBits = root_x;
        gNdsFighterWaitTickP1RootYBeforeBits = root_y;
        gNdsFighterWaitTickP1VelGroundXBeforeBits =
            ndsFloatBits(fp->vel_ground.x);
    }
}

static void ndsFighterMarioFoxRecordWaitTickAfter(u32 slot, FTStruct *fp,
                                                  DObj *root_dobj)
{
    u32 root_x = (root_dobj != NULL) ?
        ndsFloatBits(root_dobj->translate.vec.f.x) : 0u;
    u32 root_y = (root_dobj != NULL) ?
        ndsFloatBits(root_dobj->translate.vec.f.y) : 0u;

    if (slot == 0u)
    {
        gNdsFighterWaitTickP0StatusAfter = (u32)fp->status_id;
        gNdsFighterWaitTickP0MotionAfter = (u32)fp->motion_id;
        gNdsFighterWaitTickP0GAAfter = (u32)fp->ga;
        gNdsFighterWaitTickP0RootXAfterBits = root_x;
        gNdsFighterWaitTickP0RootYAfterBits = root_y;
        gNdsFighterWaitTickP0VelGroundXAfterBits =
            ndsFloatBits(fp->vel_ground.x);
    }
    else if (slot == 1u)
    {
        gNdsFighterWaitTickP1StatusAfter = (u32)fp->status_id;
        gNdsFighterWaitTickP1MotionAfter = (u32)fp->motion_id;
        gNdsFighterWaitTickP1GAAfter = (u32)fp->ga;
        gNdsFighterWaitTickP1RootXAfterBits = root_x;
        gNdsFighterWaitTickP1RootYAfterBits = root_y;
        gNdsFighterWaitTickP1VelGroundXAfterBits =
            ndsFloatBits(fp->vel_ground.x);
    }
}

static void ndsFighterMarioFoxRunWaitCallbackTickProbe(void)
{
    u32 mask = 0u;
    u32 callback_ready_count = 0u;
    u32 i;

    if ((ndsFighterMarioFoxWaitTickProofEnabled() == FALSE) ||
        (gNdsFighterMarioFoxWaitTickResult != 0u))
    {
        return;
    }

    if ((gNdsFighterMarioFoxWaitStatusResult ==
            NDS_FIGHTER_MARIOFOX_WAIT_STATUS_PASS) &&
        ((gNdsFighterMarioFoxWaitMask & 0xfffu) == 0xfffu) &&
        (gNdsFighterMarioFoxWaitCount == 2u))
    {
        mask |= 1u << 0;
    }
    else
    {
        gNdsFighterMarioFoxWaitTickMask = mask;
        return;
    }

    gNdsFighterWaitTickGObjCountBefore = (u32)gcGetGObjsActiveNum();

    for (i = 0u; i < 2u; i++)
    {
        FTStruct *fp = &sNdsFighterStructPool[i];
        GObj *probe_gobj;
        DObj *root_dobj;
        u32 status_before;
        u32 motion_before;
        u32 ga_before;
        u32 root_x_before;
        u32 root_y_before;

        if ((ndsFighterStructIsPoolPointer(fp) == FALSE) ||
            (fp->fighter_gobj == NULL) ||
            (fp->status_id != nFTCommonStatusWait) ||
            (fp->motion_id != nFTCommonMotionWait))
        {
            continue;
        }

        probe_gobj = fp->fighter_gobj;
        root_dobj = fp->joints[nFTPartsJointTopN];

        fp->input.pl.stick_range.x = 0;
        fp->input.pl.stick_range.y = 0;
        fp->input.pl.button_hold = 0;
        fp->input.pl.button_tap = 0;

        status_before = (u32)fp->status_id;
        motion_before = (u32)fp->motion_id;
        ga_before = (u32)fp->ga;
        root_x_before = (root_dobj != NULL) ?
            ndsFloatBits(root_dobj->translate.vec.f.x) : 0u;
        root_y_before = (root_dobj != NULL) ?
            ndsFloatBits(root_dobj->translate.vec.f.y) : 0u;

        ndsFighterMarioFoxRecordWaitTickBefore(i, fp, root_dobj);

        if ((fp->proc_interrupt == ftCommonWaitProcInterrupt) &&
            (fp->proc_physics == ftPhysicsApplyGroundVelFriction) &&
            (fp->proc_map == mpCommonProcFighterOnCliffEdge))
        {
            callback_ready_count++;
        }
        else
        {
            continue;
        }

        fp->proc_interrupt(probe_gobj);
        gNdsFighterWaitTickOriginalInterruptCount++;
        fp->proc_physics(probe_gobj);
        fp->proc_map(probe_gobj);

        ndsFighterMarioFoxRecordWaitTickAfter(i, fp, root_dobj);

        if (fp->status_id != (s32)status_before)
        {
            gNdsFighterWaitTickStatusChangeCount++;
        }
        if (fp->motion_id != (s32)motion_before)
        {
            gNdsFighterWaitTickMotionChangeCount++;
        }
        if (fp->ga != (s32)ga_before)
        {
            gNdsFighterWaitTickGADriftCount++;
        }
        if ((root_dobj == NULL) ||
            (ndsFloatBits(root_dobj->translate.vec.f.x) != root_x_before) ||
            (ndsFloatBits(root_dobj->translate.vec.f.y) != root_y_before))
        {
            gNdsFighterWaitTickRootDriftCount++;
        }

        gNdsFighterMarioFoxWaitTickCount++;
    }

    if (callback_ready_count == 2u)
    {
        mask |= 1u << 1;
    }
    if ((gNdsFighterWaitTickOriginalInterruptCount == 2u) &&
        (gNdsFighterWaitTickGroundInterruptCheckCount == 2u))
    {
        mask |= 1u << 2;
        mask |= 1u << 3;
    }
    if (gNdsFighterWaitTickPhysicsCallbackCount == 2u)
    {
        mask |= 1u << 4;
    }
    if (gNdsFighterWaitTickMapCallbackCount == 2u)
    {
        mask |= 1u << 5;
    }
    if ((gNdsFighterWaitTickStatusChangeCount == 0u) &&
        (gNdsFighterWaitTickMotionChangeCount == 0u))
    {
        mask |= 1u << 6;
    }

    gNdsFighterWaitTickGObjCountAfter = (u32)gcGetGObjsActiveNum();
    if (gNdsFighterWaitTickGObjCountAfter >=
        gNdsFighterWaitTickGObjCountBefore)
    {
        gNdsFighterWaitTickGObjDelta =
            gNdsFighterWaitTickGObjCountAfter -
            gNdsFighterWaitTickGObjCountBefore;
    }
    else
    {
        gNdsFighterWaitTickGObjDelta =
            gNdsFighterWaitTickGObjCountBefore -
            gNdsFighterWaitTickGObjCountAfter;
    }

    if ((gNdsFighterWaitTickGADriftCount == 0u) &&
        (gNdsFighterWaitTickRootDriftCount == 0u) &&
        (gNdsFighterWaitTickGObjDelta == 0u))
    {
        mask |= 1u << 7;
    }
    if ((gNdsFighterWaitTickDeniedStatusCount == 0u) &&
        (gNdsFighterWaitTickProcessAttachCount == 0u) &&
        (gNdsFighterWaitTickDisplayProbeCount == 0u) &&
        (gNdsFighterWaitTickGameplayUpdateCount == 0u))
    {
        mask |= 1u << 8;
    }
    if ((gSCManagerSceneData.scene_curr == nSCKindVSBattle) &&
        (gSCManagerSceneData.scene_prev == nSCKindMaps))
    {
        mask |= 1u << 9;
    }

    gNdsFighterMarioFoxWaitTickMask = mask;
    gNdsFighterMarioFoxWaitTickDeferredMask = 0xffu;

    if ((mask & 0x3ffu) == 0x3ffu)
    {
        gNdsFighterMarioFoxWaitTickResult =
            NDS_FIGHTER_MARIOFOX_WAIT_TICK_PASS;
        gNdsFighterMarioFoxWaitCallbackResult =
            NDS_FIGHTER_MARIOFOX_WAIT_CB_PASS;
        gNdsFighterMarioFoxWaitSafeResult =
            NDS_FIGHTER_MARIOFOX_WAIT_SAFE_PASS;
    }
}
