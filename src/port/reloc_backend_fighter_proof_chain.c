#if !NDS_IMPORT_BATTLESHIP_FTMAIN
void ftMainPlayAnimEventsAll(GObj *fighter_gobj)
{
    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopThrowCallbackImmediateActive != FALSE))
    {
        (void)fighter_gobj;
        gNdsStageMPPassiveLoopThrowCallbackImmediateAnimEventsCount++;
        return;
    }
    if ((ndsFighterMarioFoxStageMPPassiveLoopProofEnabled() != FALSE) &&
        (sNdsStageMPPassiveLoopThrowActive != FALSE))
    {
        (void)fighter_gobj;
        gNdsStageMPPassiveLoopThrowAnimEventsCount++;
        return;
    }
    if ((ndsFighterMarioFoxJumpAttackAirProofEnabled() != FALSE) &&
        (sNdsFighterJumpAttackAirActive != FALSE))
    {
        gNdsFighterJumpAttackAirAnimEventsCount++;
        return;
    }
    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunGuardOnActive != FALSE))
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);

        if ((fp != NULL) &&
            (fp->status_id == nFTCommonStatusGuardOn) &&
            (fp->motion_id == nFTCommonMotionGuardOn))
        {
            if (fp == &sNdsFighterStructPool[0])
            {
                gNdsFighterDashRunGuardAnimEventsMask |= 1u << 0u;
            }
            else if (fp == &sNdsFighterStructPool[1])
            {
                gNdsFighterDashRunGuardAnimEventsMask |= 1u << 1u;
            }
        }
    }
    if ((ndsFighterMarioFoxDashRunProofEnabled() != FALSE) &&
        (sNdsFighterDashRunAttack1Active != FALSE))
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);

        if ((fp != NULL) && (fp->player < 2))
        {
            u32 event_bit = 0xffffffffu;

            if (fp->status_id == nFTCommonStatusAttack11)
            {
                event_bit = fp->player;
            }
            else if (fp->status_id == nFTCommonStatusAttack12)
            {
                event_bit = fp->player + 2u;
            }
            else if ((fp->status_id == nFTMarioStatusAttack13) &&
                (fp->player == 0))
            {
                event_bit = 4u;
            }
            else if ((fp->status_id == nFTFoxStatusAttack100Start) &&
                (fp->player == 1))
            {
                event_bit = 5u;
            }
            if (event_bit != 0xffffffffu)
            {
                gNdsFighterDashRunAttackAnimEventsMask |=
                    1u << event_bit;
            }
        }
    }
    if ((ndsFighterMarioFoxStageMPCliffCatchFloorLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffCatchFloorLoopSetStatusActive != FALSE))
    {
        gNdsStageMPCliffCatchFloorLoopPlayAnimEventsCount++;
        return;
    }
    if ((ndsFighterMarioFoxStageMPCliffWaitDamageLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffWaitDamageLoopCliffCatchSetStatusActive != FALSE))
    {
        gNdsStageMPCliffWaitDamageLoopCliffCatchPlayAnimEventsCount++;
        return;
    }
    if ((ndsFighterMarioFoxStageMPCeilStatusFloorLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCeilStatusFloorLoopSetStatusActive != FALSE))
    {
        gNdsStageMPCeilStatusFloorLoopPlayAnimEventsCount++;
        return;
    }
    if ((ndsFighterMarioFoxStageMPCliffAttackFloorLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffAttackFloorLoopSetStatusActive != FALSE))
    {
        gNdsStageMPCliffAttackFloorLoopAnimEventsCount++;
        return;
    }
    if ((ndsFighterMarioFoxStageMPCliffClimbFloorLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffClimbFloorLoopSetStatusActive != FALSE))
    {
        gNdsStageMPCliffClimbFloorLoopAnimEventsCount++;
        return;
    }
    if ((ndsFighterMarioFoxStageMPCliffClimbFloorLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffClimbFloorLoopRecatchSetStatusActive != FALSE))
    {
        return;
    }
    if ((ndsFighterMarioFoxStageMPCliffEscapeActionLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffEscapeActionLoopSetStatusActive != FALSE))
    {
        gNdsStageMPCliffEscapeActionLoopAnimEventsCount++;
        return;
    }
    if ((ndsFighterMarioFoxStageMPDownWaitLoopProofEnabled() != FALSE) &&
        (sNdsStageMPDownWaitLoopDownAttackSetStatusActive != FALSE))
    {
        gNdsStageMPDownWaitLoopAttackAnimEventsCount++;
        ndsStageMPDownWaitLoopAppendAttackOrder(4u);
        return;
    }
    if ((ndsFighterMarioFoxStageMPDownWaitLoopProofEnabled() != FALSE) &&
        (sNdsStageMPDownWaitLoopDownForwardBackSetStatusActive != FALSE))
    {
        gNdsStageMPDownWaitLoopRollAnimEventsCount++;
        if (sNdsStageMPDownWaitLoopRollForwardProbeActive != FALSE)
        {
            ndsStageMPDownWaitLoopAppendRollForwardOrder(5u);
        }
        else
        {
            ndsStageMPDownWaitLoopAppendRollBackOrder(5u);
        }
        return;
    }
    if ((ndsFighterMarioFoxStageTurnLoopProofEnabled() != FALSE) &&
        (sNdsStageTurnLoopSetStatusActive != FALSE))
    {
        gNdsStageTurnLoopAnimEventsCount++;
        return;
    }
    if ((ndsFighterMarioFoxStageMPDownRecoverLoopProofEnabled() != FALSE) &&
        (sNdsStageMPDownRecoverLoopDownAttackSetStatusActive != FALSE))
    {
        gNdsStageMPDownRecoverLoopAttackAnimEventsCount++;
        ndsStageMPDownRecoverLoopAppendAttackOrder(4u);
        return;
    }
    if ((ndsFighterMarioFoxStageMPDownRecoverLoopProofEnabled() != FALSE) &&
        (sNdsStageMPDownRecoverLoopDownForwardBackSetStatusActive != FALSE))
    {
        gNdsStageMPDownRecoverLoopRollAnimEventsCount++;
        if (sNdsStageMPDownRecoverLoopRollForwardProbeActive != FALSE)
        {
            ndsStageMPDownRecoverLoopAppendRollForwardOrder(5u);
        }
        else
        {
            ndsStageMPDownRecoverLoopAppendRollBackOrder(5u);
        }
        return;
    }
    if ((ndsFighterMarioFoxProcessLoopProofEnabled() != FALSE) &&
        (sNdsFighterProcessLoopActive != FALSE))
    {
        return;
    }
    if (ndsFighterMarioFoxDashRunProofEnabled() != FALSE)
    {
        gNdsFighterDashRunAnimEventsCallCount++;
    }
    if (ndsFighterMarioFoxWalkInputProofEnabled() != FALSE)
    {
        gNdsFighterWalkAnimEventsCallCount++;
    }
}
#endif

static void ndsFighterMarioFoxRunWaitStatusProbe(GObj *fighter_gobj,
                                                 FTDesc *desc)
{
    u32 i;

    (void)fighter_gobj;
    (void)desc;

    if ((ndsFighterMarioFoxWaitProofEnabled() == FALSE) ||
        (gNdsFighterMarioFoxInitResult != NDS_FIGHTER_MARIOFOX_INIT_PASS) ||
        (gNdsFighterMarioFoxInitCount < 2u))
    {
        return;
    }

    for (i = 0; i < 2u; i++)
    {
        FTStruct *fp = &sNdsFighterStructPool[i];
        GObj *probe_gobj;

        if (ndsFighterStructIsPoolPointer(fp) == FALSE)
        {
            continue;
        }
        probe_gobj = fp->fighter_gobj;
        if (probe_gobj == NULL)
        {
            continue;
        }

        if (fp->is_wait_status_setup == FALSE)
        {
            gNdsFighterWaitOriginalSetStatusCallCount++;
            ftCommonWaitSetStatus(probe_gobj);
            fp->is_special_interrupt = TRUE;
            gNdsFighterMarioFoxWaitCount++;
        }
        else
        {
            ndsFighterMarioFoxRecordInstalledWaitState(fp);
        }
        ndsFighterStructRecord(fp);
        ndsFighterMarioFoxRecordWaitStatus(fp);
    }

    ndsFighterMarioFoxRunWaitCallbackTickProbe();
    ndsFighterMarioFoxRunWaitGroundProof();
    ndsFighterMarioFoxRunDisplayProbe();
    ndsFighterMarioFoxRunDLScanProbe();
    ndsFighterMarioFoxRunDLExecuteProbe();
    ndsFighterMarioFoxRunDLDrawProbe();
    ndsFighterMarioFoxRunDLMultiDrawProbe();
    ndsFighterMarioFoxRunDLAllDrawProbe();
    ndsFighterMarioFoxRunWalkInputProof();
}

void ndsFighterMarioFoxRunImmediateProofChain(void)
{
    u32 i;

#if NDS_IMPORT_BATTLESHIP_FTMANAGER
    if ((gNdsFighterManagerResult == NDS_FIGHTER_MANAGER_PASS) &&
        ((gNdsFighterManagerWaitMask & 0x3u) == 0x3u))
    {
        ndsFighterMarioFoxRunDLMultiDrawProbe();
        ndsFighterMarioFoxRunDLAllDrawProbe();
        if ((gNdsFighterMarioFoxDLMultiDrawResult != 0u) ||
            (gNdsFighterMarioFoxDLAllDrawResult != 0u))
        {
            return;
        }
    }
#endif

    for (i = 0u; i < 2u; i++)
    {
        FTStruct *fp = &sNdsFighterStructPool[i];

        if ((ndsFighterStructIsPoolPointer(fp) != FALSE) &&
            (fp->fighter_gobj != NULL))
        {
            ndsFighterMarioFoxRunWaitStatusProbe(fp->fighter_gobj, NULL);
            break;
        }
    }
}
static void ndsFighterMarioFoxInitStateFromOriginalOrder(
    GObj *fighter_gobj, FTDesc *desc, FTAttributes *attr, DObj *root_dobj)
{
    FTStruct *fp;
    sb32 is_floor;

    if ((fighter_gobj == NULL) || (desc == NULL) || (attr == NULL) ||
        (root_dobj == NULL) ||
        (ndsFighterMarioFoxInitProofEnabled() == FALSE))
    {
        return;
    }

    fp = ftGetStruct(fighter_gobj);
    if ((fp == NULL) || (ndsFighterStructIsPoolPointer(fp) == FALSE))
    {
        return;
    }

    fp->lr = desc->lr;
    fp->percent_damage = desc->damage;
    fp->shield_health = 55;
    fp->unk_ft_0x38 = 0.0F;
    fp->hitlag_tics = 0;
    fp->is_knockback_paused = FALSE;
    ftPhysicsStopVelAll(fighter_gobj);

    fp->jumps_used = 0;
    fp->is_reflect = FALSE;
    fp->is_absorb = FALSE;
    fp->is_shield = FALSE;
    fp->is_effect_attach = FALSE;
    fp->is_jostle_ignore = FALSE;
    fp->cliffcatch_wait = 0;
    fp->tics_since_last_z = 0;
    fp->acid_wait = 0;
    fp->twister_wait = 0;
    fp->tarucann_wait = 0;
    fp->damagefloor_wait = 0;

    fp->attack_damage = 0;
    fp->attack_count = 0;
    fp->attack_shield_push = 0;
    fp->shield_damage = 0;
    fp->damage_lag = 0;
    fp->damage_queue = 0;
    fp->damage_angle = 0;
    fp->damage_element = nGMHitElementNormal;
    fp->damage_lr = 0;
    fp->damage_index = 0;
    fp->damage_player_num = 0;
    fp->damage_player = -1;
    fp->damage_object_class = 0;
    fp->damage_object_kind = 0;
    fp->damage_count = 0;
    fp->damage_kind = nFTDamageKindDefault;
    fp->damage_heal = 0;
    fp->damage_joint_id = 0;
    fp->invincible_tics = 0;
    fp->intangible_tics = 0;
    fp->star_invincible_tics = 0;

    fp->hitstatus = nGMHitStatusNormal;
    fp->star_hitstatus = nGMHitStatusNormal;
    fp->special_hitstatus = nGMHitStatusNormal;
    fp->throw_gobj = NULL;
    fp->catch_gobj = NULL;
    fp->capture_gobj = NULL;
    fp->is_catch_or_capture = FALSE;
    fp->item_gobj = NULL;
    fp->reflect_lr = 0;
    fp->absorb_lr = 0;
    fp->reflect_damage = 0;

    fp->attack1_followup_frames = 0.0F;
    fp->attack_knockback = 0.0F;
    fp->attack_rebound = 0.0F;
    fp->damage_knockback_stack = 0.0F;
    fp->knockback_resist_status = 0.0F;
    fp->knockback_resist_passive = 0.0F;
    fp->damage_knockback = 0.0F;
    fp->hitlag_mul = 1.0F;
    fp->shield_heal_wait = 10.0F;
    fp->is_fastfall = FALSE;
    fp->player_num = fp->player;
    fp->public_knockback = 0.0F;
    fp->is_hitstun = FALSE;
    fp->is_use_animlocks = FALSE;
    fp->shuffle_frame_index = 0;
    fp->shuffle_index_max = 0;
    fp->is_use_fogcolor = FALSE;
    fp->is_shuffle_electric = FALSE;
    fp->shuffle_tics = 0;
    fp->motion_attack_id = nFTMotionAttackIDNone;
    fp->motion_count = 0;
    fp->stat_attack_id = nFTStatusAttackIDNone;
    fp->stat_count = 0;
    fp->damage_stat_count = 0;

    root_dobj->translate.vec.f = desc->pos;
    root_dobj->scale.vec.f.x = attr->size;
    root_dobj->scale.vec.f.y = attr->size;
    root_dobj->scale.vec.f.z = attr->size;
    ndsFighterPartsSyncDObj(fp, root_dobj, nFTPartsJointTopN);
    fp->coll_data.p_translate = &root_dobj->translate.vec.f;
    fp->coll_data.p_lr = &fp->lr;
    fp->coll_data.map_coll = attr->map_coll;
    fp->coll_data.p_map_coll = &fp->coll_data.map_coll;
    fp->coll_data.cliffcatch_coll = attr->cliffcatch_coll;
    fp->coll_data.ignore_line_id = -1;
    fp->coll_data.update_tic = gMPCollisionUpdateTic;
    fp->coll_data.mask_curr = 0;
    fp->coll_data.mask_stat = MAP_FLAG_FLOOR;
    fp->coll_data.is_coll_end = FALSE;

    fp->nds_init_floor_project_attempted = 1u;
    is_floor = mpCollisionCheckProjectFloor(
        &root_dobj->translate.vec.f,
        &fp->coll_data.floor_line_id,
        &fp->coll_data.floor_dist,
        &fp->coll_data.floor_flags,
        &fp->coll_data.floor_angle);
    fp->nds_init_floor_project_result = (is_floor != FALSE) ? 1u : 0u;

    if ((is_floor != FALSE) && (fp->coll_data.floor_dist > -300.0F))
    {
        fp->ga = nMPKineticsGround;
        root_dobj->translate.vec.f.y += fp->coll_data.floor_dist;
        fp->coll_data.floor_dist = 0.0F;
    }
    else
    {
        fp->ga = nMPKineticsAir;
        fp->jumps_used = 1;
        fp->coll_data.floor_line_id = -1;
    }
    fp->coll_data.pos_prev = root_dobj->translate.vec.f;
    if (fp->ga == nMPKineticsGround)
    {
        fp->coll_data.mask_stat = MAP_FLAG_FLOOR;
        fp->coll_data.is_coll_end = FALSE;
    }

    if (fp->fkind == nFTKindMario)
    {
        fp->passive_vars.mario.is_expend_tornado = FALSE;
    }
    else if (fp->fkind == nFTKindFox)
    {
    }

    ftParamClearAttackCollAll(fighter_gobj);
    ndsFighterMarioFoxSeedDamageColls(fp);
    ftParamSetHitStatusPartAll(fighter_gobj, nGMHitStatusNormal);
    ftParamResetFighterColAnim(fighter_gobj);

    fp->nds_init_mask = 0x3fffu;
    gNdsFighterMarioFoxInitCount++;
    ndsFighterStructRecord(fp);
    ndsFighterMarioFoxRecordInitState(fp);
}

static GObj *ndsFighterMarioFoxMakeFighter(FTDesc *desc)
{
    FTAttributes *attr;
    FTCommonPartContainer *commonparts;
    FTCommonPart *commonpart;
    DObjDesc *dobjdesc;
    GObj *fighter_gobj;
    DObj *root_dobj;
    s32 detail_index;

    if ((desc == NULL) ||
        ((desc->fkind != nFTKindMario) && (desc->fkind != nFTKindFox)))
    {
        return NULL;
    }

    gNdsFighterModelLastPlayer = (u32)desc->player;
    gNdsFighterModelLastFKind = (u32)desc->fkind;
    gNdsFighterModelLastStageMask = 1u;
    gNdsFighterModelLastAttrPtr = 0u;
    gNdsFighterModelLastCommonPartsPtr = 0u;
    gNdsFighterModelLastCommonPartPtr = 0u;
    gNdsFighterModelLastDObjDescPtr = 0u;
    gNdsFighterModelLastGObjPtr = 0u;

    ndsFighterMarioFoxSetupFilesKind(desc->fkind);
    gNdsFighterModelLastStageMask |= 1u << 1;
    attr = ndsFighterMarioFoxGetAttributes(desc->fkind);
    gNdsFighterModelLastAttrPtr = (uintptr_t)attr;
    gNdsFighterModelLastStageMask |= 1u << 2;
    if (desc->fkind == nFTKindMario)
    {
        gNdsFighterMarioAttrPtrReady = (attr != NULL) ? 1u : 0u;
        gNdsFighterMarioFoxSetupMask |=
            (attr != NULL) ? NDS_FIGHTER_MARIOFOX_SETUP_MARIO_ATTR : 0u;
    }
    else
    {
        gNdsFighterFoxAttrPtrReady = (attr != NULL) ? 1u : 0u;
        gNdsFighterMarioFoxSetupMask |=
            (attr != NULL) ? NDS_FIGHTER_MARIOFOX_SETUP_FOX_ATTR : 0u;
    }
    if (attr == NULL)
    {
        return NULL;
    }

    commonparts = attr->commonparts_container;
    gNdsFighterModelLastCommonPartsPtr = (uintptr_t)commonparts;
    gNdsFighterModelLastStageMask |= 1u << 3;
    detail_index = (desc->detail >= nFTPartsDetailStart) ?
        (s32)(desc->detail - nFTPartsDetailStart) : 0;
    if (detail_index > 1)
    {
        detail_index = 0;
    }
    commonpart = (commonparts != NULL) ?
        &commonparts->commonparts[detail_index] : NULL;
    gNdsFighterModelLastCommonPartPtr = (uintptr_t)commonpart;
    gNdsFighterModelLastStageMask |= 1u << 4;
    dobjdesc = (commonpart != NULL) ? commonpart->dobjdesc : NULL;
    gNdsFighterModelLastDObjDescPtr = (uintptr_t)dobjdesc;
    gNdsFighterModelLastStageMask |= 1u << 5;
    if (desc->fkind == nFTKindMario)
    {
        gNdsFighterMarioCommonPartsReady =
            (dobjdesc != NULL) ? 1u : 0u;
        gNdsFighterMarioFoxSetupMask |=
            (dobjdesc != NULL) ?
                NDS_FIGHTER_MARIOFOX_SETUP_MARIO_COMMONPART : 0u;
    }
    else
    {
        gNdsFighterFoxCommonPartsReady =
            (dobjdesc != NULL) ? 1u : 0u;
        gNdsFighterMarioFoxSetupMask |=
            (dobjdesc != NULL) ?
                NDS_FIGHTER_MARIOFOX_SETUP_FOX_COMMONPART : 0u;
    }
    if (dobjdesc == NULL)
    {
        return NULL;
    }

    fighter_gobj = gcMakeGObjSPAfter(nGCCommonKindFighter,
                                     NULL,
                                     nGCCommonLinkIDFighter,
                                     GOBJ_PRIORITY_DEFAULT);
    gNdsFighterModelLastGObjPtr = (uintptr_t)fighter_gobj;
    gNdsFighterModelLastStageMask |= 1u << 6;
    if (fighter_gobj == NULL)
    {
        return NULL;
    }

    fighter_gobj->user_data.s = desc->player;
    gcAddGObjDisplay(fighter_gobj,
                     ftDisplayMainProcDisplay,
                     FTDISPLAY_DLLINK_DEFAULT,
                     GOBJ_PRIORITY_DEFAULT,
                     ~0u);
    gcSetupCustomDObjs(fighter_gobj,
                       dobjdesc,
                       NULL,
                       0x4B,
                       nGCMatrixKindNull,
                       nGCMatrixKindNull);
    root_dobj = DObjGetStruct(fighter_gobj);
    if (root_dobj != NULL)
    {
        root_dobj->translate.vec.f = desc->pos;
    }

    gNdsFighterModelRealGObjCount++;
    gNdsFighterModelProcessDeferredCount++;
    gNdsFighterMarioFoxSetupMask |=
        NDS_FIGHTER_MARIOFOX_SETUP_DISPLAY |
        NDS_FIGHTER_MARIOFOX_SETUP_PROCESS_DEFER;
    if (desc->fkind == nFTKindMario)
    {
        gNdsFighterMarioFoxSetupMask |=
            NDS_FIGHTER_MARIOFOX_SETUP_MARIO_GOBJ;
    }
    else
    {
        gNdsFighterMarioFoxSetupMask |=
            NDS_FIGHTER_MARIOFOX_SETUP_FOX_GOBJ;
    }
    if (gNdsFighterModelRealGObjCount >= 2u)
    {
        gNdsFighterMarioFoxGObjResult = NDS_FIGHTER_MARIOFOX_GOBJ_PASS;
    }
    if ((gNdsFighterMarioFoxSetupMask & 0x0ffu) == 0x0ffu)
    {
        gNdsFighterMarioFoxModelResult =
            NDS_FIGHTER_MARIOFOX_MODEL_PASS;
    }

    gNdsSCVSBattleOriginalFighterGObjCount++;
    gNdsSCVSBattleOriginalFighterCreateCount++;
    gNdsSCVSBattleOriginalActivePlayerCount++;
    gNdsSCVSBattleOriginalActivePlayerMask |= 1u << (desc->player & 3);
    /* P2-3r15: one byte per slot, `fkind + 1`, so a four-name roster is
     * readable at all -- the P0/P1 kind pair below stops at two. The
     * recorder that actually runs in a VSBattle is
     * ndsFighterManagerRecordCreatedFighter; this path is the compat/proof
     * twin and writes the same field the same way. */
    gNdsSCVSBattleOriginalFighterKinds =
        (gNdsSCVSBattleOriginalFighterKinds &
         ~(0xffu << ((desc->player & 3u) * 8u))) |
        ((((u32)desc->fkind + 1u) & 0xffu) << ((desc->player & 3u) * 8u));
    if (desc->player == 0)
    {
        gNdsSCVSBattleOriginalP0FKind = (u32)desc->fkind;
        gNdsSCVSBattleOriginalP0LR = (u32)desc->lr;
    }
    else if (desc->player == 1)
    {
        gNdsSCVSBattleOriginalP1FKind = (u32)desc->fkind;
        gNdsSCVSBattleOriginalP1LR = (u32)desc->lr;
    }
    gNdsSCVSBattleCompatManagerMask |= 1u << 3;
    gNdsSCVSBattleCompatMask |= NDS_SCVSBATTLE_COMPAT_FIGHTER_MANAGER;
    ndsFighterRecordModelGObj(desc, fighter_gobj, root_dobj);
    if (ndsFighterMarioFoxStructProofEnabled() != FALSE)
    {
        FTStruct *fp = ndsFighterStructInitFromDesc(fighter_gobj, desc, attr,
                                                    root_dobj);

        if ((fp != NULL) && (ndsFighterMarioFoxInitProofEnabled() != FALSE))
        {
            ndsFighterMarioFoxInitStateFromOriginalOrder(
                fighter_gobj, desc, attr, root_dobj);
            if (ndsFighterMarioFoxWaitProofEnabled() != FALSE)
            {
                ndsFighterMarioFoxRunWaitStatusProbe(fighter_gobj, desc);
            }
        }
    }
    return fighter_gobj;
}

static void ndsFighterMarioFoxRecordStubFighter(FTDesc *desc,
                                                GObj *fighter_gobj)
{
    (void)fighter_gobj;
    if ((ndsFighterMarioFoxModelProofEnabled() != FALSE) &&
        (desc != NULL) &&
        ((desc->fkind == nFTKindMario) || (desc->fkind == nFTKindFox)))
    {
        gNdsFighterModelStubGObjCount++;
    }
}
