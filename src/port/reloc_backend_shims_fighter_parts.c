/* Source-function shims, fighter parts and ground statuses: the parts' MObjs,
 * guard, the damage status, cliff and down statuses, the ground physics, the
 * cliff, floor and ceiling collisions, the struct stubs and costumes. A slice
 * of reloc_backend.c. */

void lbCommonAddMObjForFighterPartsDObj(DObj *dobj, MObjSub **mobjsubs,
                                        AObjEvent32 **costume_matanim_joints,
                                        AObjEvent32 **main_matanim_joints,
                                        f32 anim_frame)
{
    if ((dobj == NULL) || (mobjsubs == NULL))
    {
        return;
    }

    while (*mobjsubs != NULL)
    {
        MObjSub normalized_mobjsub = **mobjsubs;
        MObj *mobj;

        ndsRelocNormalizeMObjSubWordSwapped(&normalized_mobjsub);
        mobj = gcAddMObjForDObj(dobj, &normalized_mobjsub);

        if (mobj == NULL)
        {
            return;
        }
        if (costume_matanim_joints != NULL)
        {
            AObjEvent32 *costume_matanim_joint = *costume_matanim_joints;

            if (costume_matanim_joint != NULL)
            {
                /* A one-shot bake (its AObjs are removed right after):
                 * the source rate, whatever tick of a batch this is. */
                gcAddMObjMatAnimJoint(mobj, costume_matanim_joint,
                                      anim_frame);
                ndsGcParseMObjMatAnimJointNow(mobj);
                ndsGcPlayMObjMatAnimNow(mobj);
                gcRemoveAObjFromMObj(mobj);
            }
            costume_matanim_joints++;
        }
        if (main_matanim_joints != NULL)
        {
            AObjEvent32 *main_matanim_joint = *main_matanim_joints;

            if (main_matanim_joint != NULL)
            {
                gcAddMObjMatAnimJoint(mobj, main_matanim_joint, 0.0F);
                ndsGcParseMObjMatAnimJointNow(mobj);
                ndsGcPlayMObjMatAnimNow(mobj);
            }
            main_matanim_joints++;
        }
        mobjsubs++;
    }
}

void mpCommonUpdateFighterSlopeContour(GObj *fighter_gobj)
{
    (void)fighter_gobj;
}

void ftCommonReboundProcUpdate(GObj *fighter_gobj)
{
    ndsBaseFTCommonReboundProcUpdate(fighter_gobj);
    return;
}

void ftCommonReboundSetStatus(GObj *fighter_gobj)
{
    ndsBaseFTCommonReboundSetStatus(fighter_gobj);
    return;
}

void ftCommonReboundWaitProcUpdate(GObj *fighter_gobj)
{
    ndsBaseFTCommonReboundWaitProcUpdate(fighter_gobj);
    return;
}

void ftCommonReboundWaitSetStatus(GObj *fighter_gobj)
{
    ndsBaseFTCommonReboundWaitSetStatus(fighter_gobj);
    return;
}

void ftCommonGuardSetOffProcUpdate(GObj *fighter_gobj)
{
    ndsBaseFTCommonGuardSetOffProcUpdate(fighter_gobj);
    return;
}

void ftCommonGuardSetOffSetStatus(GObj *fighter_gobj)
{
    ndsBaseFTCommonGuardSetOffSetStatus(fighter_gobj);
    return;
}

alSoundEffect *lbCommonMakePositionFGM(u16 fgm, f32 pos)
{
    s32 balance = (s32)((pos / 8000.0F) * 60.0F);

    if (balance > 60)
    {
        balance = 60;
    }
    if (balance < -60)
    {
        balance = -60;
    }
    return ndsPlayFGMAtPan(fgm, (u8)(64 - balance));
}

sb32 ftCommonCliffAttackCheckInterruptCommon(GObj *fighter_gobj)
{
    return ndsBaseFTCommonCliffAttackCheckInterruptCommon(fighter_gobj);
}

sb32 ftCommonCliffEscapeCheckInterruptCommon(GObj *fighter_gobj)
{
    return ndsBaseFTCommonCliffEscapeCheckInterruptCommon(fighter_gobj);
}

sb32 ftCommonCliffClimbOrFallCheckInterruptCommon(GObj *fighter_gobj)
{
    return ndsBaseFTCommonCliffClimbOrFallCheckInterruptCommon(fighter_gobj);
}

void ftCommonDamageUpdateDustEffect(GObj *fighter_gobj)
{
    ndsBaseFTCommonDamageUpdateDustEffect(fighter_gobj);
    return;
}

void ftCommonDamageDecHitStunSetPublic(GObj *fighter_gobj)
{
    ndsBaseFTCommonDamageDecHitStunSetPublic(fighter_gobj);
    return;
}

void ftCommonDamageUpdateCatchResist(GObj *fighter_gobj)
{
    ndsBaseFTCommonDamageUpdateCatchResist(fighter_gobj);
    return;
}

void ftCommonDamageCommonProcUpdate(GObj *fighter_gobj)
{
    ndsBaseFTCommonDamageCommonProcUpdate(fighter_gobj);
    return;
}

void ftCommonDamageAirCommonProcUpdate(GObj *fighter_gobj)
{
    ndsBaseFTCommonDamageAirCommonProcUpdate(fighter_gobj);
    return;
}

void ftCommonDamageCheckSetInvincible(GObj *fighter_gobj)
{
    ndsBaseFTCommonDamageCheckSetInvincible(fighter_gobj);
    return;
}

void ftCommonDamageSetStatus(GObj *fighter_gobj)
{
    ndsBaseFTCommonDamageSetStatus(fighter_gobj);
    return;
}

void ftCommonDamageCommonProcInterrupt(GObj *fighter_gobj)
{
    ndsBaseFTCommonDamageCommonProcInterrupt(fighter_gobj);
    return;
}

void ftCommonDamageAirCommonProcInterrupt(GObj *fighter_gobj)
{
    ndsBaseFTCommonDamageAirCommonProcInterrupt(fighter_gobj);
    return;
}

void ftCommonDamageFlyRollUpdateModelPitch(GObj *fighter_gobj)
{
    ndsBaseFTCommonDamageFlyRollUpdateModelPitch(fighter_gobj);
    return;
}

void ftCommonDamageCommonProcPhysics(GObj *fighter_gobj)
{
    ndsBaseFTCommonDamageCommonProcPhysics(fighter_gobj);
    return;
}

void ftCommonDamageCommonProcLagUpdate(GObj *fighter_gobj)
{
    ndsBaseFTCommonDamageCommonProcLagUpdate(fighter_gobj);
    return;
}


/* Defined further down this same translation unit, and the damage proc below
 * is the first caller. */
f32 lbCommonMag2D(Vec3f *vec);

static sb32 ndsMPCommonProcFighterDamage(MPCollData *coll_data,
                                                  GObj *fighter_gobj,
                                                  u32 flags)
{
    FTStruct *fp;
    sb32 is_collide = FALSE;
    f32 root_y_before;

    (void)flags;
    NDS_DIAG(gNdsCollisionRuntimeDiagnostics.damage_proc_calls++);
    if ((coll_data == NULL) || (fighter_gobj == NULL) ||
        (coll_data->p_translate == NULL))
    {
        NDS_DIAG(gNdsCollisionRuntimeDiagnostics.damage_invalid++);
        return FALSE;
    }
    fp = ftGetStruct(fighter_gobj);
    if (fp == NULL)
    {
        NDS_DIAG(gNdsCollisionRuntimeDiagnostics.damage_invalid++);
        return FALSE;
    }
    root_y_before = coll_data->p_translate->y;

    /* BattleShip mpCommonProcFighterDamage (mpcommon.c:725) in source order:
     * left wall, right wall, ceiling, then floor.
     *
     * The wall and ceiling branches used to be deferred here "until the O2R
     * side runners publish source-faithful line IDs, angles, and clamps", and
     * this function actively CLEARED LWALL/RWALL/CEIL out of mask_stat. A
     * fighter in any damage status therefore had no wall or ceiling collision
     * at all -- it flew through them -- which is what Castle's blue upper side
     * ramps failing "during knockback/falling" is (BUGS.md S02, owner: still
     * not fixed after the walking-case repairs). The side runners those
     * branches waited on now exist: the AdjNew wall tests, the tilted-surface
     * and LR-angle helpers and the faithful wall sweep all landed with the
     * 2026-09-21 wall-pass work, so the deferral has no remaining cause.
     *
     * Each branch is the source's, including its two-armed bookkeeping: a
     * contact only ENDS the sub-step loop when this is a newly struck surface
     * (not already in coll_mask_prev), the displacement is over 30 units and
     * the approach angle exceeds 110 degrees -- otherwise the surface is
     * recorded in coll_mask_ignore and the knockback continues through the
     * remaining sub-steps. Dropping either arm turns a grazing contact into a
     * full stop, or a real wall slam into a pass-through. */
    if (mpProcessCheckTestLWallCollisionAdjNew(coll_data) != FALSE)
    {
        mpProcessRunLWallCollisionAdjNew(coll_data);

        if (((fp->status_vars.common.damage.coll_mask_prev &
              MAP_FLAG_LWALL) == 0u) &&
            (lbCommonMag2D(&coll_data->pos_diff) > 30.0F) &&
            (syVectorAngleDiff3D(&coll_data->pos_diff,
                                 &coll_data->lwall_angle) >
             F_CLC_DTOR32(110.0F)))
        {
            fp->status_vars.common.damage.coll_mask_curr |= MAP_FLAG_LWALL;
            is_collide = TRUE;
            coll_data->is_coll_end = TRUE;
            NDS_DIAG(gNdsCollisionRuntimeDiagnostics.damage_wall_hits++);
        }
        else if ((coll_data->mask_prev & MAP_FLAG_LWALL) == 0u)
        {
            fp->status_vars.common.damage.coll_mask_ignore |= MAP_FLAG_LWALL;
        }
    }
    if (mpProcessCheckTestRWallCollisionAdjNew(coll_data) != FALSE)
    {
        mpProcessRunRWallCollisionAdjNew(coll_data);

        if (((fp->status_vars.common.damage.coll_mask_prev &
              MAP_FLAG_RWALL) == 0u) &&
            (lbCommonMag2D(&coll_data->pos_diff) > 30.0F) &&
            (syVectorAngleDiff3D(&coll_data->pos_diff,
                                 &coll_data->rwall_angle) >
             F_CLC_DTOR32(110.0F)))
        {
            fp->status_vars.common.damage.coll_mask_curr |= MAP_FLAG_RWALL;
            is_collide = TRUE;
            coll_data->is_coll_end = TRUE;
            NDS_DIAG(gNdsCollisionRuntimeDiagnostics.damage_wall_hits++);
        }
        else if ((coll_data->mask_prev & MAP_FLAG_RWALL) == 0u)
        {
            fp->status_vars.common.damage.coll_mask_ignore |= MAP_FLAG_RWALL;
        }
    }
    if (mpProcessCheckTestCeilCollisionAdjNew(coll_data) != FALSE)
    {
        mpProcessRunCeilCollisionAdjNew(coll_data);

        if ((coll_data->mask_stat & MAP_FLAG_CEIL) != 0u)
        {
            mpProcessRunCeilEdgeAdjust(coll_data);
        }
        if (((fp->status_vars.common.damage.coll_mask_prev &
              MAP_FLAG_CEIL) == 0u) &&
            (lbCommonMag2D(&coll_data->pos_diff) > 30.0F) &&
            (syVectorAngleDiff3D(&coll_data->pos_diff,
                                 &coll_data->ceil_angle) >
             F_CLC_DTOR32(110.0F)))
        {
            fp->status_vars.common.damage.coll_mask_curr |= MAP_FLAG_CEIL;
            is_collide = TRUE;
            coll_data->is_coll_end = TRUE;
            NDS_DIAG(gNdsCollisionRuntimeDiagnostics.damage_ceil_hits++);
        }
        else if ((coll_data->mask_prev & MAP_FLAG_CEIL) == 0u)
        {
            fp->status_vars.common.damage.coll_mask_ignore |= MAP_FLAG_CEIL;
        }
    }
    NDS_DIAG(gNdsCollisionRuntimeDiagnostics.damage_floor_tests++);
    if (mpProcessRunFloorCollisionAdjNewNULL(coll_data) != FALSE)
    {
        if ((mpCollisionCheckExistLineID(coll_data->floor_line_id) == FALSE) ||
            (mpCollisionGetLineTypeID(coll_data->floor_line_id) !=
                nMPLineKindFloor))
        {
            NDS_DIAG(gNdsCollisionRuntimeDiagnostics.damage_invalid++);
            coll_data->mask_curr &= (u16)~MAP_FLAG_FLOOR;
            mpProcessSetCollProjectFloorID(coll_data);
            return FALSE;
        }
        NDS_DIAG(gNdsCollisionRuntimeDiagnostics.damage_floor_hits++);
        if (fp->hitlag_tics > 0)
        {
            mpProcessSetCollideFloor(coll_data);
            if ((coll_data->mask_stat & MAP_FLAG_FLOOR) != 0u)
            {
                mpProcessRunFloorEdgeAdjust(coll_data);
            }
            else
            {
                mpProcessSetCollProjectFloorID(coll_data);
            }
        }
        else if (syVectorAngleDiff3D(&coll_data->pos_diff,
                     &coll_data->floor_angle) > F_CLC_DTOR32(110.0F))
        {
            mpProcessSetLandingFloor(coll_data);
            mpCommonSetFighterLandingParams(fighter_gobj);
            if ((coll_data->mask_stat & MAP_FLAG_FLOOR) != 0u)
            {
                mpProcessRunFloorEdgeAdjust(coll_data);
                fp->status_vars.common.damage.coll_mask_curr |=
                    MAP_FLAG_FLOOR;
                is_collide = TRUE;
                coll_data->is_coll_end = TRUE;
                NDS_DIAG(gNdsCollisionRuntimeDiagnostics.damage_floor_landings++);
            }
        }
        else
        {
            mpProcessSetCollideFloor(coll_data);
            if ((coll_data->mask_stat & MAP_FLAG_FLOOR) != 0u)
            {
                mpProcessRunFloorEdgeAdjust(coll_data);
                if ((coll_data->mask_prev & MAP_FLAG_FLOOR) == 0u)
                {
                    fp->status_vars.common.damage.coll_mask_ignore |=
                        MAP_FLAG_FLOOR;
                    fp->status_vars.common.damage.wall_collide_angle =
                        coll_data->floor_angle;
                }
            }
            else
            {
                mpProcessSetCollProjectFloorID(coll_data);
            }
        }
    }
    else
    {
        mpProcessSetCollProjectFloorID(coll_data);
    }
    if (is_collide != FALSE)
    {
        gNdsCollisionRuntimeDiagnostics.damage_last_line =
            coll_data->floor_line_id;
        gNdsCollisionRuntimeDiagnostics.damage_last_status = fp->status_id;
        gNdsCollisionRuntimeDiagnostics.damage_last_root_y_before_milli =
            ndsFloatToMilliSigned(root_y_before);
        gNdsCollisionRuntimeDiagnostics.damage_last_root_y_after_milli =
            ndsFloatToMilliSigned(coll_data->p_translate->y);
        gNdsCollisionRuntimeDiagnostics.damage_last_pos_diff_y_milli =
            ndsFloatToMilliSigned(coll_data->pos_diff.y);
        gNdsCollisionRuntimeDiagnostics.damage_last_angle_y_milli =
            ndsFloatToMilliSigned(coll_data->floor_angle.y);
        gNdsCollisionRuntimeDiagnostics.damage_last_mask_curr =
            coll_data->mask_curr;
        gNdsCollisionRuntimeDiagnostics.damage_last_mask_stat =
            coll_data->mask_stat;
    }
    return is_collide;
}

sb32 mpCommonCheckFighterDamageCollision(GObj *fighter_gobj)
{
    FTStruct *fp = (fighter_gobj != NULL) ? ftGetStruct(fighter_gobj) : NULL;

    if (ndsBattlePlayableRuntimeEnabled() != FALSE)
    {
        sb32 result;

        NDS_DIAG(gNdsCollisionRuntimeDiagnostics.damage_check_calls++);
        if ((fp == NULL) || (fp->coll_data.p_translate == NULL))
        {
            NDS_DIAG(gNdsCollisionRuntimeDiagnostics.damage_invalid++);
            return FALSE;
        }
        fp->status_vars.common.damage.coll_mask_prev =
            fp->status_vars.common.damage.coll_mask_curr;
        fp->status_vars.common.damage.coll_mask_curr = 0u;
        fp->status_vars.common.damage.coll_mask_ignore = 0u;
        result = mpProcessUpdateMain(&fp->coll_data,
                                     ndsMPCommonProcFighterDamage,
                                     fighter_gobj, MAP_PROC_TYPE_DEFAULT);
        if (result != FALSE)
        {
            NDS_DIAG(gNdsCollisionRuntimeDiagnostics.damage_results++);
        }
        return result;
    }
    return FALSE;
}

sb32 ftCommonWallDamageCheckGoto(GObj *fighter_gobj)
{
    return ndsBaseFTCommonWallDamageCheckGoto(fighter_gobj);
}

void ftCommonDamageAirCommonProcMap(GObj *fighter_gobj)
{
    ndsBaseFTCommonDamageAirCommonProcMap(fighter_gobj);
    return;
}

f32 ftParamGetHitStun(f32 knockback)
{
    return knockback / 1.875F;
}

void ftParamSetTimedHitStatusInvincible(FTStruct *fp, s32 invincible_tics)
{
    if (fp == NULL)
    {
        return;
    }
    if (fp->invincible_tics < invincible_tics)
    {
        fp->invincible_tics = invincible_tics;
    }
    fp->special_hitstatus = (fp->intangible_tics != 0) ?
        nGMHitStatusIntangible : nGMHitStatusInvincible;
    ftParamCheckSetFighterColAnimID(fp->fighter_gobj,
                                    nGMColAnimFighterNoDamage, 0);
}

void ftParamSetTimedHitStatusIntangible(FTStruct *fp, s32 intangible_tics)
{
    if (fp == NULL)
    {
        return;
    }
    /* BattleShip ftparam.c:1732-1740 extends an existing intangible window but
     * never shortens it. The old DS shim overwrote it unconditionally. */
    if (fp->intangible_tics < intangible_tics)
    {
        fp->intangible_tics = intangible_tics;
    }
    fp->special_hitstatus = nGMHitStatusIntangible;
    ftParamCheckSetFighterColAnimID(fp->fighter_gobj,
                                    nGMColAnimFighterNoDamage, 0);
}

void ftCommonDamageFallProcInterrupt(GObj *fighter_gobj)
{
    ndsBaseFTCommonDamageFallProcInterrupt(fighter_gobj);
    return;
}

void ftCommonDamageFallProcMap(GObj *fighter_gobj)
{
    ndsBaseFTCommonDamageFallProcMap(fighter_gobj);
    return;
}

void ftCommonDamageFallClampRumble(GObj *fighter_gobj)
{
    ndsBaseFTCommonDamageFallClampRumble(fighter_gobj);
    return;
}

void ftCommonDamageFallSetStatusFromDamage(GObj *fighter_gobj)
{
    ndsBaseFTCommonDamageFallSetStatusFromDamage(fighter_gobj);
    return;
}

/* Owned by battleship_ftcommon_hammer.c wherever the item core is on -- that
 * TU imports the whole of ftcommonhammerfall.c. */

static sb32 ndsMPCommonRunFighterCliffFloorCeilCollisions(
    MPCollData *coll_data, GObj *fighter_gobj, u32 flags);

sb32 mpCommonCheckFighterCliff(GObj *fighter_gobj)
{
    {
        FTStruct *fp = (fighter_gobj != NULL) ? ftGetStruct(fighter_gobj) : NULL;

        if (fp != NULL)
        {
            return mpProcessUpdateMain(
                &fp->coll_data,
                ndsMPCommonRunFighterCliffFloorCeilCollisions,
                fighter_gobj, MAP_PROC_TYPE_CLIFF);
        }
    }
    return FALSE;
}

sb32 ftCommonPassiveStandCheckInterruptDamage(GObj *fighter_gobj)
{
    return ndsBaseFTCommonPassiveStandCheckInterruptDamage(fighter_gobj);
}

sb32 ftCommonPassiveCheckInterruptDamage(GObj *fighter_gobj)
{
    return ndsBaseFTCommonPassiveCheckInterruptDamage(fighter_gobj);
}

void ftCommonDownBounceSetStatus(GObj *fighter_gobj)
{
    ndsBaseFTCommonDownBounceSetStatus(fighter_gobj);
    return;
}

void ftCommonCliffCatchSetStatus(GObj *fighter_gobj)
{
    ndsBaseFTCommonCliffCatchSetStatus(fighter_gobj);
    return;
}

void ftCommonDownWaitProcUpdate(GObj *fighter_gobj)
{
    ndsBaseFTCommonDownWaitProcUpdate(fighter_gobj);
    return;
}

void ftCommonDownWaitProcInterrupt(GObj *fighter_gobj)
{
    ndsBaseFTCommonDownWaitProcInterrupt(fighter_gobj);
    return;
}

void ftCommonDownWaitSetStatus(GObj *fighter_gobj)
{
    ndsBaseFTCommonDownWaitSetStatus(fighter_gobj);
    return;
}

void ftCommonDownBounceProcUpdate(GObj *fighter_gobj)
{
    ndsBaseFTCommonDownBounceProcUpdate(fighter_gobj);
    return;
}

sb32 ftCommonDownBounceCheckUpOrDown(GObj *fighter_gobj)
{
    return ndsBaseFTCommonDownBounceCheckUpOrDown(fighter_gobj);
}

void ftCommonDownBounceUpdateEffects(GObj *fighter_gobj)
{
    ndsBaseFTCommonDownBounceUpdateEffects(fighter_gobj);
    return;
}

void ftCommonDownStandProcInterrupt(GObj *fighter_gobj)
{
    ndsBaseFTCommonDownStandProcInterrupt(fighter_gobj);
    return;
}

void ftCommonDownStandSetStatus(GObj *fighter_gobj)
{
    ndsBaseFTCommonDownStandSetStatus(fighter_gobj);
    return;
}

void ftCommonDownAttackSetStatus(GObj *fighter_gobj, s32 status_id)
{
    ndsBaseFTCommonDownAttackSetStatus(fighter_gobj, status_id);
    return;
}

sb32 ftCommonDownAttackCheckInterruptDownWait(GObj *fighter_gobj)
{
    return ndsBaseFTCommonDownAttackCheckInterruptDownWait(fighter_gobj);
}

void ftCommonDownForwardOrBackSetStatus(GObj *fighter_gobj, s32 status_id)
{
    ndsBaseFTCommonDownForwardOrBackSetStatus(fighter_gobj, status_id);
    return;
}

sb32 ftCommonDownForwardOrBackCheckInterruptCommon(GObj *fighter_gobj)
{
    return ndsBaseFTCommonDownForwardOrBackCheckInterruptCommon(fighter_gobj);
}

sb32 ftCommonDownStandCheckInterruptCommon(GObj *fighter_gobj)
{
    return ndsBaseFTCommonDownStandCheckInterruptCommon(fighter_gobj);
}

sb32 ftCommonDownAttackCheckInterruptDownBounce(GObj *fighter_gobj)
{
    return ndsBaseFTCommonDownAttackCheckInterruptDownBounce(fighter_gobj);
}

void ftCommonDamageFallSetStatusFromCliffWait(GObj *fighter_gobj)
{
    ndsBaseFTCommonDamageFallSetStatusFromCliffWait(fighter_gobj);
    return;
}

void ftCommonCliffCommon2UpdateCollData(GObj *fighter_gobj)
{
    ndsBaseFTCommonCliffCommon2UpdateCollData(fighter_gobj);
    return;
}

void ftCommonCliffCommon2InitStatusVars(GObj *fighter_gobj)
{
    ndsBaseFTCommonCliffCommon2InitStatusVars(fighter_gobj);
    return;
}

void ftPhysicsApplyGroundVelFriction(GObj *fighter_gobj)
{
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);

        if ((fp != NULL) &&
            (ndsFighterStructIsPoolPointer(fp) == FALSE) &&
            (ndsFighterStructIsTrackedPointer(fp) != FALSE) &&
            (fp->attr != NULL))
        {
            u32 material = fp->coll_data.floor_flags & MAP_VERTEX_MAT_MASK;

            ftPhysicsSetGroundVelFriction(fp,
                dMPCollisionMaterialFrictions[material & 0x0fu] *
                fp->attr->traction);
            ftPhysicsSetGroundVelTransferAir(fighter_gobj);
        }
    }
}

void ftPhysicsSetGroundVelAbsStickRange(FTStruct *fp, f32 vel, f32 friction)
{
    f32 target;

    if (fp == NULL)
    {
        return;
    }
    target = ABS(fp->input.pl.stick_range.x) * vel;
    if (fp->physics.vel_ground.x < target)
    {
        fp->physics.vel_ground.x = target;
    }
    else
    {
        fp->physics.vel_ground.x -= friction;
        if (fp->physics.vel_ground.x < target)
        {
            fp->physics.vel_ground.x = target;
        }
    }
}

/* BattleShip ftphysics.c:113-151. Samus's grounded Bomb physics is the first
 * promoted fighter path that needs the signed-stick form; keep the original
 * branch/order rather than substituting the absolute-stick helper. */
void ftPhysicsSetGroundVelStickRange(FTStruct *fp, f32 vel, f32 friction)
{
    f32 target;

    if (fp == NULL)
    {
        return;
    }
    target = fp->input.pl.stick_range.x * vel * fp->lr;
    if (fp->physics.vel_ground.x >= 0.0F)
    {
        if (fp->physics.vel_ground.x < target)
        {
            fp->physics.vel_ground.x = target;
        }
        else
        {
            fp->physics.vel_ground.x -= friction;
            if (fp->physics.vel_ground.x < target)
            {
                fp->physics.vel_ground.x = target;
            }
        }
    }
    else
    {
        if (fp->physics.vel_ground.x > target)
        {
            fp->physics.vel_ground.x = target;
        }
        else
        {
            fp->physics.vel_ground.x += friction;
            if (target < fp->physics.vel_ground.x)
            {
                fp->physics.vel_ground.x = target;
            }
        }
    }
}

void ftPhysicsSetGroundVelFriction(FTStruct *fp, f32 friction)
{
    if ((fp == NULL) || (friction < 0.0F))
    {
        return;
    }
    if (fp->physics.vel_ground.x > 0.0F)
    {
        fp->physics.vel_ground.x -= friction;
        if (fp->physics.vel_ground.x < 0.0F)
        {
            fp->physics.vel_ground.x = 0.0F;
        }
    }
    else if (fp->physics.vel_ground.x < 0.0F)
    {
        fp->physics.vel_ground.x += friction;
        if (fp->physics.vel_ground.x > 0.0F)
        {
            fp->physics.vel_ground.x = 0.0F;
        }
    }
}

void ftPhysicsSetGroundVelTransferAir(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if ((fp == NULL) || (ndsFighterStructIsTrackedPointer(fp) == FALSE))
    {
        return;
    }
    ndsFTPhysicsSetGroundVelTransferAirOriginal(fighter_gobj, fp);
    ndsFighterSyncPhysicsToLegacyVel(fp);
}

void mpCommonProcFighterOnCliffEdge(GObj *fighter_gobj)
{
    if (ndsBattlePlayableRuntimeEnabled() != FALSE)
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);

        if ((fp != NULL) &&
            (mpCommonCheckFighterOnCliffEdge(fighter_gobj) == FALSE))
        {
            if ((fp->coll_data.mask_stat & MAP_FLAG_FLOOREDGE) != 0u)
            {
                ftCommonOttottoSetStatus(fighter_gobj);
            }
            else
            {
                ftCommonFallSetStatus(fighter_gobj);
            }
        }
        return;
    }
}

static sb32 (*sNdsMPCommonProcPass)(GObj *);

static sb32 ndsMPProcessCheckTestFloorCollisionAdjNew(
    MPCollData *coll_data, sb32 (*proc_map)(GObj *), GObj *gobj)
{
    return mpProcessCheckTestFloorCollisionAdjNew(coll_data, proc_map, gobj);
}

static sb32 ndsMPCommonRunFighterCliffFloorCeilCollisions(
    MPCollData *coll_data, GObj *fighter_gobj, u32 flags)
{
    FTStruct *this_fp = ftGetStruct(fighter_gobj);
    GObj *cliffcatch_gobj;
    sb32 is_ceilstop = FALSE;

    if ((coll_data == NULL) || (this_fp == NULL))
    {
        return FALSE;
    }

    /* BUGS.md #1: the wall pair and the ceiling-edge adjust were both missing
     * here, which is the "Mario/Fox special wall, ceiling, and edge adjustment
     * is incomplete" row in KNOWN_ISSUES. This runner stands in for the
     * source's mpCommonRunFighterSpecialCollisions (mpcommon.c:472), which
     * every special/project/pass/landing entry point uses, so the gap took out
     * Up B: without the wall pass coll_data->mask_unk never carries
     * MAP_FLAG_LWALL/RWALL, and that is exactly what
     * mpProcessCheckTestCeilCollisionAdjNew's fallback branches
     * (mpprocess.c:1911-1934) read to catch a rise that enters the platform
     * past the end of the ceiling segment. Mario went straight through. */
    if (mpProcessCheckTestLWallCollisionAdjNew(coll_data) != FALSE)
    {
        mpProcessRunLWallCollisionAdjNew(coll_data);
    }
    if (mpProcessCheckTestRWallCollisionAdjNew(coll_data) != FALSE)
    {
        mpProcessRunRWallCollisionAdjNew(coll_data);
    }
    if (mpProcessCheckTestCeilCollisionAdjNew(coll_data) != FALSE)
    {
        mpProcessRunCeilCollisionAdjNew(coll_data);
        if ((coll_data->mask_stat & MAP_FLAG_CEIL) != 0u)
        {
            mpProcessRunCeilEdgeAdjust(coll_data);
        }
        if (((flags & MAP_PROC_TYPE_CEILHEAVY) != 0u) &&
            (this_fp->physics.vel_air.y >= 30.0F))
        {
            coll_data->mask_curr |= MAP_FLAG_CEILHEAVY;
            is_ceilstop = TRUE;
            coll_data->is_coll_end = TRUE;
        }
    }

    if (ndsMPProcessCheckTestFloorCollisionAdjNew(
            coll_data,
            ((flags & MAP_PROC_TYPE_PASS) != 0u) ?
                sNdsMPCommonProcPass : NULL,
            ((flags & MAP_PROC_TYPE_PASS) != 0u) ? fighter_gobj : NULL) !=
        FALSE)
    {
        if ((flags & MAP_PROC_TYPE_PROJECT) != 0u)
        {
            mpProcessSetCollideFloor(coll_data);
            if ((coll_data->mask_stat & MAP_FLAG_FLOOR) != 0u)
            {
                mpProcessRunFloorEdgeAdjust(coll_data);
            }
            else
            {
                mpProcessSetCollProjectFloorID(coll_data);
            }
        }
        else
        {
            mpProcessSetLandingFloor(coll_data);
            mpCommonSetFighterLandingParams(fighter_gobj);
            if ((coll_data->mask_stat & MAP_FLAG_FLOOR) != 0u)
            {
                mpProcessRunFloorEdgeAdjust(coll_data);
                coll_data->is_coll_end = TRUE;
                return TRUE;
            }
        }
    }
    else
    {
        mpProcessSetCollProjectFloorID(coll_data);
    }

    if (((flags & MAP_PROC_TYPE_CLIFF) == 0u) ||
        (this_fp->cliffcatch_wait != 0))
    {
        return is_ceilstop;
    }
    if ((mpProcessCheckTestLCliffCollision(coll_data) == FALSE) &&
        (mpProcessCheckTestRCliffCollision(coll_data) == FALSE))
    {
        return is_ceilstop;
    }

    cliffcatch_gobj = gGCCommonLinks[nGCCommonLinkIDFighter];
    while (cliffcatch_gobj != NULL)
    {
        if (cliffcatch_gobj != fighter_gobj)
        {
            FTStruct *cliffcatch_fp = ftGetStruct(cliffcatch_gobj);

            if ((cliffcatch_fp != NULL) &&
                (cliffcatch_fp->is_cliff_hold != FALSE) &&
                (this_fp->coll_data.cliff_id ==
                 cliffcatch_fp->coll_data.cliff_id) &&
                (this_fp->lr == cliffcatch_fp->lr))
            {
                return is_ceilstop;
            }
        }
        cliffcatch_gobj = cliffcatch_gobj->link_next;
    }
    sNdsP4CliffLanding = TRUE;
    mpCommonSetFighterLandingParams(fighter_gobj);
    sNdsP4CliffLanding = FALSE;
    coll_data->is_coll_end = TRUE;
    return TRUE;
}

void mpCommonProcFighterCliffFloorCeil(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    NDS_DIAG(gNdsFighterBattlePlayableMapCallCount++);
    if ((fp != NULL) &&
        (mpProcessUpdateMain(&fp->coll_data,
                             ndsMPCommonRunFighterCliffFloorCeilCollisions,
                             fighter_gobj,
                             MAP_PROC_TYPE_CEILHEAVY | MAP_PROC_TYPE_CLIFF) !=
         FALSE))
    {
        NDS_DIAG(gNdsFighterBattlePlayableMapHitCount++);
        gNdsFighterBattlePlayableMapLastMaskStat =
            (u32)fp->coll_data.mask_stat;
        gNdsFighterBattlePlayableMapLastMaskCurr =
            (u32)fp->coll_data.mask_curr;
        if ((fp->coll_data.mask_stat & MAP_FLAG_CLIFF_MASK) != 0u)
        {
            NDS_DIAG(gNdsFighterBattlePlayableMapCliffHitCount++);
            ftCommonCliffCatchSetStatus(fighter_gobj);
        }
        else if ((fp->coll_data.mask_stat & MAP_FLAG_FLOOR) != 0u)
        {
            NDS_DIAG(gNdsFighterBattlePlayableMapFloorHitCount++);
            if (fp->physics.vel_air.y >
                FTCOMMON_ATTACKAIR_SKIPLANDING_VEL_Y_MAX)
            {
                ftCommonWaitSetStatus(fighter_gobj);
            }
            else
            {
                ftCommonLandingSetStatus(fighter_gobj);
            }
        }
        else if ((fp->coll_data.mask_curr & MAP_FLAG_CEILHEAVY) != 0u)
        {
            NDS_DIAG(gNdsFighterBattlePlayableMapCeilHitCount++);
            ftCommonStopCeilSetStatus(fighter_gobj);
        }
    }
    return;
}

void mpCommonSetFighterFallOnGroundBreak(GObj *fighter_gobj)
{
    if (ndsBattlePlayableRuntimeEnabled() != FALSE)
    {
        (void)mpCommonProcFighterOnFloor(
            fighter_gobj, ftCommonFallSetStatus);
        return;
    }
    mpCommonProcFighterOnCliffEdge(fighter_gobj);
}

void mpCommonSetFighterWaitOrFall(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if ((fp != NULL) && (fp->ga == nMPKineticsGround))
    {
        ftCommonWaitSetStatus(fighter_gobj);
    }
    else
    {
        ftCommonFallSetStatus(fighter_gobj);
    }
}

void mpCommonSetFighterFallOnEdgeBreak(GObj *fighter_gobj)
{
    if (ndsBattlePlayableRuntimeEnabled() != FALSE)
    {
        if (mpCommonCheckFighterOnEdge(fighter_gobj) == FALSE)
        {
            ftCommonFallSetStatus(fighter_gobj);
        }
        return;
    }
    mpCommonSetFighterFallOnGroundBreak(fighter_gobj);
}

sb32 mpCommonCheckFighterLanding(GObj *fighter_gobj)
{
    FTStruct *fp = (fighter_gobj != NULL) ? ftGetStruct(fighter_gobj) : NULL;

    if (fp != NULL)
    {
        return mpProcessUpdateMain(&fp->coll_data,
                                   ndsMPCommonRunFighterCliffFloorCeilCollisions,
                                   fighter_gobj, MAP_PROC_TYPE_DEFAULT);
    }

    if ((ndsFighterMarioFoxJumpAttackAirProofEnabled() != FALSE) &&
        (sNdsFighterJumpAttackAirMapLandingActive != FALSE) &&
        (fp != NULL) &&
        (fp->status_id == nFTCommonStatusAttackAirN) &&
        (fp->motion_id == nFTCommonMotionAttackAirN))
    {
        gNdsFighterJumpAttackAirMapLandingMask |= 1u;
        return TRUE;
    }
    return FALSE;
}

sb32 mpCommonProcFighterOnFloor(GObj *fighter_gobj,
                                void (*proc_map)(GObj *))
{
    if (mpCommonCheckFighterOnFloor(fighter_gobj) == FALSE)
    {
        if (proc_map != NULL)
        {
            proc_map(fighter_gobj);
        }
        return FALSE;
    }
    return TRUE;
}

sb32 mpCommonCheckFighterProject(GObj *fighter_gobj)
{
    FTStruct *fp = (fighter_gobj != NULL) ? ftGetStruct(fighter_gobj) : NULL;

    if (fp == NULL)
    {
        return FALSE;
    }
    return mpProcessUpdateMain(&fp->coll_data,
                               ndsMPCommonRunFighterCliffFloorCeilCollisions,
                               fighter_gobj, MAP_PROC_TYPE_PROJECT);
}

sb32 mpCommonCheckFighterPass(GObj *fighter_gobj,
                              sb32 (*proc_map)(GObj *))
{
    FTStruct *fp = (fighter_gobj != NULL) ? ftGetStruct(fighter_gobj) : NULL;

    if (fp == NULL)
    {
        return FALSE;
    }
    sNdsMPCommonProcPass = proc_map;
    return mpProcessUpdateMain(&fp->coll_data,
                               ndsMPCommonRunFighterCliffFloorCeilCollisions,
                               fighter_gobj, MAP_PROC_TYPE_PASS);
}

sb32 mpCommonCheckFighterPassCliff(GObj *fighter_gobj,
                                   sb32 (*proc_map)(GObj *))
{
    FTStruct *fp = (fighter_gobj != NULL) ? ftGetStruct(fighter_gobj) : NULL;

    if (fp == NULL)
    {
        return FALSE;
    }
    sNdsMPCommonProcPass = proc_map;
    return mpProcessUpdateMain(&fp->coll_data,
                               ndsMPCommonRunFighterCliffFloorCeilCollisions,
                               fighter_gobj,
                               MAP_PROC_TYPE_PASS | MAP_PROC_TYPE_CLIFF);
}

sb32 mpCommonProcFighterCliff(GObj *fighter_gobj,
                              void (*proc_map)(GObj *))
{
    FTStruct *fp = (fighter_gobj != NULL) ? ftGetStruct(fighter_gobj) : NULL;

    if ((fp != NULL) && (mpCommonCheckFighterCliff(fighter_gobj) != FALSE))
    {
        if ((fp->coll_data.mask_stat & MAP_FLAG_CLIFF_MASK) != 0u)
        {
            ftCommonCliffCatchSetStatus(fighter_gobj);
        }
        else if (proc_map != NULL)
        {
            proc_map(fighter_gobj);
        }
        return TRUE;
    }
    return FALSE;
}

/* P2-3f5. BattleShip mp/mpcommon.c:668 and :684 and :697, ported at the seam
 * that reimplements mpcommon.c rather than forked into the fighter that needed
 * them first.
 *
 * `mpCommonSetFighterWaitOrLanding` did not exist here even though its three
 * lines had been copied inline into `mpCommonProcFighterWaitOrLanding` and
 * `mpCommonProcFighterCliffFloorCeil`; it is a function in the source because
 * the source passes it as a proc_map, which is exactly what
 * `mpCommonProcFighterCliffWaitOrLanding` does with it.
 *
 * `mpCommonCheckFighterCeilHeavyCliff` was DECLARED in include/mp/map.h with no
 * definition anywhere -- a link error waiting for its first caller. Falcon Dive
 * is that caller (`ftCaptainSpecialHiProcMap` takes the ceiling arm when
 * vel_air.y >= 0), and Kirby, Link and Yoshi all reach it too, so it belongs
 * here for all of them. Same collision runner and the same
 * CEILHEAVY|CLIFF process flags `mpCommonProcFighterCliffFloorCeil` above
 * already uses. */
void mpCommonSetFighterWaitOrLanding(GObj *fighter_gobj)
{
    FTStruct *fp = (fighter_gobj != NULL) ? ftGetStruct(fighter_gobj) : NULL;

    if (fp == NULL)
    {
        return;
    }
    if (fp->physics.vel_air.y > FTCOMMON_ATTACKAIR_SKIPLANDING_VEL_Y_MAX)
    {
        ftCommonWaitSetStatus(fighter_gobj);
    }
    else
    {
        ftCommonLandingSetStatus(fighter_gobj);
    }
}

void mpCommonProcFighterCliffWaitOrLanding(GObj *fighter_gobj)
{
    mpCommonProcFighterCliff(fighter_gobj, mpCommonSetFighterWaitOrLanding);
}

sb32 mpCommonCheckFighterCeilHeavyCliff(GObj *fighter_gobj)
{
    FTStruct *fp = (fighter_gobj != NULL) ? ftGetStruct(fighter_gobj) : NULL;

    if (fp == NULL)
    {
        return FALSE;
    }
    return mpProcessUpdateMain(&fp->coll_data,
                               ndsMPCommonRunFighterCliffFloorCeilCollisions,
                               fighter_gobj,
                               MAP_PROC_TYPE_CEILHEAVY | MAP_PROC_TYPE_CLIFF);
}

/* mpcommon.c:665, the same body without the cliff arm. Yoshi Bomb's start
 * (ftyoshispeciallw.c:60) is the first caller: while still rising it asks for
 * a heavy ceiling only, and only a heavy ceiling (MAP_FLAG_CEILHEAVY) drops it
 * into the falling loop. Same runner as the source's special-collision one. */
sb32 mpCommonCheckFighterCeilHeavy(GObj *fighter_gobj)
{
    FTStruct *fp = (fighter_gobj != NULL) ? ftGetStruct(fighter_gobj) : NULL;

    if (fp == NULL)
    {
        return FALSE;
    }
    return mpProcessUpdateMain(&fp->coll_data,
                               ndsMPCommonRunFighterCliffFloorCeilCollisions,
                               fighter_gobj, MAP_PROC_TYPE_CEILHEAVY);
}

f32 ftParamGetStickAngleRads(FTStruct *fp)
{
    if (fp == NULL)
    {
        return 0.0F;
    }
    return atan2f((f32)fp->input.pl.stick_range.y,
                  (f32)ABS(fp->input.pl.stick_range.x));
}

f32 lbCommonMag2D(Vec3f *vec)
{
    if (vec == NULL)
    {
        return 0.0F;
    }
    return sqrtf((vec->x * vec->x) + (vec->y * vec->y));
}

sb32 lbCommonCheckAdjustSim2D(Vec3f *a, Vec3f *b, f32 angle)
{
    f32 similarity;
    f32 orientation;
    f32 magnitude;
    f32 denom;

    if ((a == NULL) || (b == NULL))
    {
        return FALSE;
    }
    denom = lbCommonMag2D(a) + lbCommonMag2D(b);
    if (denom == 0.0F)
    {
        return FALSE;
    }
    similarity = ((b->x * a->x) + (b->y * a->y)) / denom;
    if ((similarity <= 0.0F) &&
        (similarity >= cosf(angle + F_CST_DTOR32(90.0F))))
    {
        orientation = (b->x * a->y) - (b->y * a->x);
        orientation = (orientation < 0.0F) ? -1.0F : 1.0F;
        magnitude = lbCommonMag2D(a) * orientation;
        a->x = -b->y * magnitude;
        a->y = b->x * magnitude;
        return TRUE;
    }
    return FALSE;
}

Vec3f *lbCommonAdd2D(Vec3f *a, Vec3f *b)
{
    if ((a == NULL) || (b == NULL))
    {
        return a;
    }
    a->x += b->x;
    a->y += b->y;
    return a;
}

Vec3f *lbCommonScale2D(Vec3f *vec, f32 factor)
{
    if (vec == NULL)
    {
        return NULL;
    }
    vec->x *= factor;
    vec->y *= factor;
    return vec;
}

Vec3f *lbCommonReflect2D(Vec3f *a, Vec3f *b)
{
    f32 negative_two_dot_product;

    if ((a == NULL) || (b == NULL))
    {
        return a;
    }
    negative_two_dot_product = ((b->x * a->x) + (b->y * a->y)) * -2.0F;
    a->x += b->x * negative_two_dot_product;
    a->y += b->y * negative_two_dot_product;
    return a;
}

/* R2-07 L9. These were `return sinf(angle);` / `return cosf(angle);`, which is
 * both slower and LESS faithful than the source: SSB64 does not call a libm
 * sine at all, it indexes a 1024-entry quarter-turn table (`lb/lbcommon.c`,
 * 0x800C7840). Gameplay reads those exact quantised numbers, so the table is
 * the reference behaviour and libm was the approximation.
 *
 * The port never compiled `lbcommon.c`, so the table had nothing to link
 * against; `src/port/lbcommon_sin_table.c` carries it, extracted verbatim.
 *
 * Cost, measured on the R2-07 L6 census (`artifacts/task37-census/r207-L6-*`):
 * the libm path is ~237 cycles/call, and an over-gate frame makes ~197 calls
 * for ~46,772 cycles of trig -- `__ieee754_rem_pio2f` argument reduction plus
 * `__kernel_sinf`/`__kernel_cosf`, none of which a 4096-step table needs. The
 * table form is one multiply, one float-to-int, three masks and a load.
 *
 * Index is in 4096ths of a turn (651.8986206 = 4096 / 2pi). Bit 0x400 mirrors
 * the quarter into the second one, bit 0x800 negates for the lower half; cosine
 * is the same table read a quarter turn ahead. */
/* Read gSYSinTable, which is already resident, rather than a second table.
 *
 * L9 brought in dLBCommonSinLookup -- lbcommon.c's own 1024-entry f32 quarter
 * wave -- and that cost 4,096 bytes of main RAM. The taskman arena is sized by
 * a boot-time loop that steps DOWN 0x1000 per failed calloc, so 4,096 bytes of
 * new static RAM is exactly one step: the arena shrank by a page, Sudden Death
 * lost the free space it was already running on, and it froze at the banner.
 * docs/BUGS.md carries the measurement (42,992 -> 38,896 free, 26 -> 27 steps).
 *
 * gSYSinTable is sys/sintable.c's 2048-entry u16 half wave, and it is the SAME
 * function at the SAME resolution -- both index by angle * 2048/PI masked to
 * 0xFFF, both take the sign from bit 0x800, and both carry the source's own
 * one-sample stretch (the table spans 0..PI inclusive over 2048 samples, worth
 * about 0.0016 against a true sine; that is SSB64's sine and reproducing it is
 * the point). Only the storage differs: quarter wave in f32 against half wave
 * in Q15.
 *
 * scripts/check-lbcommon-sin-table-dedup.py compares them exhaustively over all
 * 4096 indices, not a sample: they agree to 0.00003070, one Q15 quantum, which
 * is 91x inside the 0.0028 bound E64b/E65 established. The index arithmetic
 * below is unchanged from lbcommon.c:321 and :340 -- including cos adding 90
 * degrees to the ANGLE before the multiply, which is not the same as adding
 * 0x400 to the index after it. */
#include <nds/nds_r2_collision_fixed.h>

/* P2-2p8 (2026-09-29): the same two expressions without soft float.
 *
 * `(s32)(x * 651.8986206F)` is ndsR2CfxAngleIndexBits' rounded integer
 * product (proven equal to the float expression over every binary32 input
 * whose product fits s32; the rest still evaluate the float expression), and
 * `(f32)v * (1.0F / 32768.0F)` of a u16 table entry is exact -- float(v)
 * with its exponent lowered by 15 -- so it is built from v's bits, with the
 * negation a sign flip (zero included, as `-sin` of +0.0F is -0.0F). This
 * TU is Thumb, where the 64-bit product and CLZ would be helper calls, so the
 * shared core is ARM. On the gate these were ~70 calls and ~320 soft-float
 * operations a frame. Cosine still adds its quarter turn to the angle in
 * float first, exactly as before. */
static u32 __attribute__((noinline, target("arm"), section(".itcm")))
ndsLbCommonSinTableBits(f32 x)
{
    u32 bits;
    s32 index;
    u32 value;
    u32 out = 0u;

    __builtin_memcpy(&bits, &x, sizeof(bits));
    if (ndsR2CfxAngleIndexBits(bits, &index) == 0)
    {
        index = (s32)(x * 651.8986206F);
    }
    index &= 0xFFF;
    value = gSYSinTable[index & SINTABLE_MASK_ID];
    if (value != 0u)
    {
        const u32 lead = (u32)__builtin_clz(value);

        out = ((143u - lead) << 23) | (((value << lead) << 1) >> 9);
    }
    if ((index & 0x800) != 0)
    {
        out ^= 0x80000000u;
    }
    return out;
}

f32 __attribute__((section(".itcm"))) lbCommonSin(f32 angle)
{
    u32 bits;
    f32 sin;

    bits = ndsLbCommonSinTableBits(angle);
    __builtin_memcpy(&sin, &bits, sizeof(sin));
    return sin;
}

f32 __attribute__((section(".itcm"))) lbCommonCos(f32 angle)
{
    u32 bits;
    f32 cos;

    bits = ndsLbCommonSinTableBits(angle + F_CST_DTOR32(90.0F));
    __builtin_memcpy(&cos, &bits, sizeof(cos));
    return cos;
}

void ftPhysicsApplyGroundVelTransN(GObj *fighter_gobj)
{
    {
        FTStruct *fp = (fighter_gobj != NULL) ?
            ftGetStruct(fighter_gobj) : NULL;
        DObj *root = (fighter_gobj != NULL) ?
            DObjGetStruct(fighter_gobj) : NULL;
        DObj *transn_joint;

        if ((fp == NULL) || (root == NULL) ||
            (fp->joints[nFTPartsJointTransN] == NULL))
        {
            return;
        }
        transn_joint = fp->joints[nFTPartsJointTransN];

        /* BattleShip ftphysics.c:168-184. TransN Z owns forward ground
         * motion; TransN X owns lateral motion after facing/orientation. */
        fp->physics.vel_ground.x =
            (transn_joint->translate.vec.f.z - fp->anim_vel.z) *
            root->scale.vec.f.z;
        fp->physics.vel_ground.z =
            (transn_joint->translate.vec.f.x - fp->anim_vel.x) *
            -fp->lr * root->scale.vec.f.x;

        if ((fp->lr * root->rotate.vec.f.y) < 0.0F)
        {
            fp->physics.vel_ground.x = -fp->physics.vel_ground.x;
            fp->physics.vel_ground.z = -fp->physics.vel_ground.z;
        }
        /* Remix Command.asm apply_ground_z_/apply_ground_x_: the 0xD3 TOPJOINT
         * TRANSLATION MULTIPLIER scales both TransN terms before the air
         * transfer. Only P4 scripts ever set it off 1.0. */
        if (ndsP4TranslationMultiplier(fp) != 1.0F)
        {
            fp->physics.vel_ground.x *= ndsP4TranslationMultiplier(fp);
            fp->physics.vel_ground.z *= ndsP4TranslationMultiplier(fp);
        }
        ftPhysicsSetGroundVelTransferAir(fighter_gobj);
    }
}

void ftPhysicsGetAirVelTransN(FTStruct *fp, f32 *vel_x, f32 *vel_y,
                              f32 *vel_z)
{
    DObj *topn_joint;
    DObj *transn_joint;
    f32 anim_vel_z;
    f32 anim_vel_y;
    f32 transn_cos;
    f32 transn_sin;
    f32 next_x = 0.0F;
    f32 next_y = 0.0F;
    f32 next_z = 0.0F;

    if ((fp != NULL) &&
        (fp->joints[nFTPartsJointTopN] != NULL) &&
        (fp->joints[nFTPartsJointTransN] != NULL))
    {
        topn_joint = fp->joints[nFTPartsJointTopN];
        transn_joint = fp->joints[nFTPartsJointTransN];
        anim_vel_z =
            (transn_joint->translate.vec.f.z - fp->anim_vel.z) *
            fp->lr * topn_joint->scale.vec.f.z;
        anim_vel_y =
            (transn_joint->translate.vec.f.y - fp->anim_vel.y) *
            topn_joint->scale.vec.f.y;
        transn_cos = cosf(transn_joint->rotate.vec.f.z);
        transn_sin = __sinf(transn_joint->rotate.vec.f.z);

        /* BattleShip ftphysics.c:393-413. The original parameter names are
         * Z/Y/X, but ApplyAirVelTransNAll passes vel_air X/Y/Z in that order. */
        next_x = (anim_vel_z * transn_cos) -
            (anim_vel_y * transn_sin);
        next_y = (anim_vel_z * transn_sin) +
            (anim_vel_y * transn_cos);
        next_z =
            (transn_joint->translate.vec.f.x - fp->anim_vel.x) *
            -fp->lr * topn_joint->scale.vec.f.x;
        /* Remix apply_air_z_/_y_/_x_: the same multiplier on all three. */
        if (ndsP4TranslationMultiplier(fp) != 1.0F)
        {
            next_x *= ndsP4TranslationMultiplier(fp);
            next_y *= ndsP4TranslationMultiplier(fp);
            next_z *= ndsP4TranslationMultiplier(fp);
        }
    }
    if (vel_x != NULL)
    {
        *vel_x = next_x;
    }
    if (vel_y != NULL)
    {
        *vel_y = next_y;
    }
    if (vel_z != NULL)
    {
        *vel_z = next_z;
    }
}

void ftPhysicsApplyAirVelTransNAll(GObj *fighter_gobj)
{
    FTStruct *fp = (fighter_gobj != NULL) ? ftGetStruct(fighter_gobj) : NULL;

    if (fp != NULL)
    {
        ftPhysicsGetAirVelTransN(fp, &fp->physics.vel_air.x,
                                 &fp->physics.vel_air.y,
                                 &fp->physics.vel_air.z);
    }
}

void ftPhysicsApplyAirVelTransNYZ(GObj *fighter_gobj)
{
    FTStruct *fp = (fighter_gobj != NULL) ? ftGetStruct(fighter_gobj) : NULL;

    if (fp != NULL)
    {
        ftPhysicsGetAirVelTransN(fp, NULL, &fp->physics.vel_air.y,
                                 &fp->physics.vel_air.z);
    }
}

void mpCollisionGetSpeedLineID(s32 line_id, Vec3f *vel)
{
    u32 yakumono_id = 0u;
    DObj *yakumono_dobj;

    if (vel != NULL)
    {
        vel->x = 0.0F;
        vel->y = 0.0F;
        vel->z = 0.0F;
    }
    if ((vel == NULL) || (line_id == -1) || (line_id == -2) ||
        (ndsMPFindLineYakumonoID(line_id, &yakumono_id) == FALSE) ||
        (gMPCollisionYakumonoDObjs == NULL) ||
        (gMPCollisionSpeeds == NULL) ||
        (yakumono_id >= NDS_MP_YAKUMONO_DOBJ_SLOTS))
    {
        return;
    }

    yakumono_dobj = gMPCollisionYakumonoDObjs->dobjs[yakumono_id];
    if ((yakumono_dobj == NULL) ||
        (yakumono_dobj->user_data.s >= nMPYakumonoStatusOff))
    {
        return;
    }

    *vel = gMPCollisionSpeeds[yakumono_id];
}

/* The stub builder lives out of line so that `ftGetStruct` does not carry its
 * register frame.
 *
 * Decomp resolves this in one load -- `ftGetStruct` is a macro there
 * (`ftparam.c` callers dereference `user_data.p` directly). The port needs a
 * real function because three pointer provenances have to be distinguished, but
 * the profile says the distinguishing costs far less than the *frame*: over
 * 246.3 calls a frame, `push {r3-r7,lr}` costs 1,957 cyc/frame and
 * `pop {r3-r7,pc}` 5,184 -- 7,141 of the function's 18,336 cyc/frame, 38.9%,
 * spent saving five registers that only this cold tail ever needs. GCC cannot
 * shrink-wrap it away: ARMv5TE Thumb-1 has no conditional execution, so the
 * early returns cannot skip a prologue that the tail requires.
 *
 * Splitting the tail leaves the hot path a leaf that needs two registers, and
 * costs nothing -- the code is the same code, one branch further away, on a path
 * that the whole-match profile never executed once.
 *
 * The statics move with it because they are the stub's storage and nothing else
 * reads them. Callers still receive the same pointer on every path. */
static FTStruct *__attribute__((noinline)) ftGetStructBuildStub(GObj *fighter_gobj);

FTStruct *__attribute__((section(".itcm"))) ftGetStruct(GObj *fighter_gobj)
{
    if ((fighter_gobj != NULL) &&
        (sNdsFTCommonCliffCommon2BridgeStruct != NULL) &&
        (fighter_gobj->user_data.p == sNdsFTCommonCliffCommon2BridgeStruct))
    {
        return sNdsFTCommonCliffCommon2BridgeStruct;
    }

    if ((fighter_gobj != NULL) &&
        (ndsFighterStructIsPoolPointer(fighter_gobj->user_data.p) != FALSE))
    {
        return fighter_gobj->user_data.p;
    }
    if ((fighter_gobj != NULL) &&
        (fighter_gobj->id == nGCCommonKindFighter) &&
        ((uintptr_t)fighter_gobj->user_data.p > 0x1000u))
    {
        return fighter_gobj->user_data.p;
    }

    return ftGetStructBuildStub(fighter_gobj);
}

static FTStruct *__attribute__((noinline)) ftGetStructBuildStub(GObj *fighter_gobj)
{
    static FTStruct stub;
    static DObj top_joint;
    static FTParts top_parts;

    if (fighter_gobj != NULL)
    {
        DObj *stub_joint;
        FTParts *stub_parts;

        bzero(&stub, sizeof(stub));
        stub.player = (u8)fighter_gobj->user_data.s;
        stub_joint = DObjGetStruct(fighter_gobj);
        if (stub_joint == NULL)
        {
            bzero(&top_joint, sizeof(top_joint));
            top_joint.translate.vec.f.x = 0.0F;
            top_joint.translate.vec.f.y = 0.0F;
            top_joint.translate.vec.f.z = 0.0F;
            top_joint.scale.vec.f.x = 1.0F;
            top_joint.scale.vec.f.y = 1.0F;
            top_joint.scale.vec.f.z = 1.0F;
            stub_joint = &top_joint;
        }
        stub.joints[nFTPartsJointTopN] = stub_joint;
        stub_joint->parent_gobj = fighter_gobj;
        if (stub_joint->parent == NULL)
        {
            stub_joint->parent = DOBJ_PARENT_NULL;
        }
        stub_parts = &top_parts;
        bzero(stub_parts, sizeof(*stub_parts));
        stub_joint->user_data.p = stub_parts;
        gNdsFtPartsLatchWrites++;
        stub_parts->unk_dobjtrans_0x5 = 1;
        stub_parts->unk_dobjtrans_0x6 = 1;
        stub_parts->unk_dobjtrans_0x7 = 1;
        stub_parts->transform_update_mode = 1;
        stub_parts->gobj = fighter_gobj;
        stub_parts->vec_scale.x = 1.0F;
        stub_parts->vec_scale.y = 1.0F;
        stub_parts->vec_scale.z = 1.0F;
        stub_parts->mtx_translate[0][0] = 1.0F;
        stub_parts->mtx_translate[1][1] = 1.0F;
        stub_parts->mtx_translate[2][2] = 1.0F;
        stub_parts->mtx_translate[3][0] = stub_joint->translate.vec.f.x;
        stub_parts->mtx_translate[3][1] = stub_joint->translate.vec.f.y;
        stub_parts->mtx_translate[3][2] = stub_joint->translate.vec.f.z;
    }
    return &stub;
}

/* BUGS.md #3: this discarded the push vector and only counted the call, so
 * Whispy's wind had no gameplay effect at all. The source is a single
 * assignment (ftparam.c:526-531); its consumer chain is already live here --
 * mpprocess.c:450-452 adds coll_data.vel_push to the translation each step and
 * ftmain.c:1571 clears it every frame, so nothing accumulates. The Pupupu
 * counters stay because the ground-loop proofs assert on them. */
void ftParamSetVelPush(GObj *fighter_gobj, Vec3f *vel)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if ((fp != NULL) && (vel != NULL))
    {
        fp->coll_data.vel_push = *vel;
    }
    gNdsPupupuUpdateVelPushCount++;
    gNdsPupupuGroundDeferredMask |= 1u << 2;
}

void lbCommonSetSpriteScissor(s32 xmin, s32 xmax, s32 ymin, s32 ymax)
{
    (void)xmin;
    (void)xmax;
    (void)ymin;
    (void)ymax;
}


void ftParamInitGame(void)
{
    extern void ftMainClearGroundElementsAll(void);

    /* BattleShip ftparam.c:2660-2664. The placement half is intentionally
     * deferred to ndsSCVSBattleBeginScenePlacement(), where the DS battle seam
     * reinitializes it after the source has populated the live player table and
     * alongside the particle placeholders. The ground-element clear has no such
     * dependency and must still happen HERE on every battle / Sudden Death
     * entry: ftmain's obstacle/hazard arrays are TU statics, so an arena rewind
     * does not clear their stale GObj pointers for us. */
    ftMainClearGroundElementsAll();
    /* The source creates a fresh fighter set later in scVSBattleStartBattle /
     * StartSuddenDeath. The DS live registry is an adapter-only mirror of that
     * set, so clear it at the same source lifetime boundary before any new
     * ftParamInitPlayerBattleStats calls repopulate slots. This is essential for
     * Sudden Death: only tied slots are recreated, and an uncleared non-tied
     * slot would otherwise retain a GObj from the arena that was just rewound. */
    ndsFighterManagerClearLiveGObjs();
    gNdsSCVSBattleCompatManagerMask |= 1u << 5;
    gNdsSCVSBattleCompatMask |= NDS_SCVSBATTLE_COMPAT_FIGHTER_MANAGER;
}

void ftParamInitPlayerBattleStats(s32 player, GObj *fighter_gobj)
{
    s32 i;

    if ((gSCManagerBattleState != NULL) &&
        (player >= 0) &&
        (player < (s32)ARRAY_COUNT(gSCManagerBattleState->players)))
    {
        /* BattleShip ftparam.c:158-183.  This is gameplay state, not merely
         * bookkeeping for the DS bridge: score/falls, the per-opponent KO and
         * damage matrices, combo totals and the stale-move queue are all read
         * later by the original N-player hit/scoring/results code.  The old
         * shim only installed fighter_gobj, which happened to be hard to notice
         * in the two-player demo because the transfer state usually arrived
         * zeroed.  Four-way/rematch play makes that assumption observably
         * wrong, so keep this wrapper byte-for-field equivalent to the source
         * initializer before adding the DS live-fighter registration below. */
        gSCManagerBattleState->players[player].place = 0;
        gSCManagerBattleState->players[player].falls = 0;
        gSCManagerBattleState->players[player].score = 0;
        for (i = 0; i < (s32)ARRAY_COUNT(gSCManagerBattleState->players); i++)
        {
            gSCManagerBattleState->players[player].total_kos_players[i] = 0;
            gSCManagerBattleState->players[player].total_damage_players[i] = 0;
        }
        gSCManagerBattleState->players[player].unk_pblock_0x28 = 0;
        gSCManagerBattleState->players[player].unk_pblock_0x2C = 0;
        gSCManagerBattleState->players[player].total_selfdestructs = 0;
        gSCManagerBattleState->players[player].total_damage_given = 0;
        gSCManagerBattleState->players[player].total_damage_all = 0;
        gSCManagerBattleState->players[player].combo_damage_foe = 0;
        gSCManagerBattleState->players[player].combo_count_foe = 0;
        gSCManagerBattleState->players[player].fighter_gobj = fighter_gobj;
        gSCManagerBattleState->players[player].stale_id = 0;
        for (i = 0;
             i < (s32)ARRAY_COUNT(gSCManagerBattleState->players[player].stale_info);
             i++)
        {
            gSCManagerBattleState->players[player].stale_info[i].attack_id = 0;
            gSCManagerBattleState->players[player].stale_info[i].motion_count = 0;
        }
    }
    ndsFighterManagerRecordCreatedFighter(fighter_gobj, player);
    gNdsSCVSBattleCompatManagerMask |= 1u << 6;
    gNdsSCVSBattleCompatMask |= NDS_SCVSBATTLE_COMPAT_FIGHTER_MANAGER;
}

void ftParamSetKey(GObj *fighter_gobj, FTKeyEvent *script)
{
    (void)fighter_gobj;
    (void)script;
}

/* BattleShip ftparam.c:56-85 dFTParamCostumeIDs, row order = FTKind
 * (ftdef.h:1090-1129, mirrored at include/ft/fighter.h:73-107). All 27 rows,
 * not just the VS roster: the 1P campaign build creates the polygon kinds too,
 * and sc1pgame.c:851 feeds them to ftParamGetCostumeCommonID, so a table that
 * stopped at Ness would be indexed past its end by the imported source callers.
 * 189 bytes. The four rows that were already here were right; the five landed
 * fighters that fell through to the identity -- Samus, Link, Yoshi, Captain and
 * Pikachu -- were not, and Link and Captain also have non-identity ROYAL rows,
 * so free-for-all costume cycling was wrong for them as well. */
static const u8 dNdsFTParamCostumeRoyal[nFTKindEnumCount][4] =
{
    { 0u, 1u, 2u, 3u },            /* Mario */
    { 0u, 1u, 2u, 3u },            /* Fox */
    { 0u, 1u, 2u, 3u },            /* Donkey Kong */
    { 0u, 1u, 2u, 3u },            /* Samus */
    { 0u, 1u, 2u, 3u },            /* Luigi */
    { 0u, 2u, 3u, 1u },            /* Link */
    { 0u, 1u, 2u, 3u },            /* Yoshi */
    { 0u, 4u, 1u, 3u },            /* Captain Falcon */
    { 0u, 1u, 2u, 3u },            /* Kirby */
    { 0u, 1u, 2u, 3u },            /* Pikachu */
    { 0u, 1u, 2u, 3u },            /* Jigglypuff */
    { 0u, 1u, 2u, 3u },            /* Ness */
    { 0u, 0u, 0u, 0u },            /* Master Hand */
    { 0u, 0u, 0u, 0u },            /* Metal Mario */
    { 0u, 0u, 0u, 0u },            /* Poly Mario */
    { 0u, 0u, 0u, 0u },            /* Poly Fox */
    { 0u, 0u, 0u, 0u },            /* Poly Donkey Kong */
    { 0u, 0u, 0u, 0u },            /* Poly Samus */
    { 0u, 0u, 0u, 0u },            /* Poly Luigi */
    { 0u, 0u, 0u, 0u },            /* Poly Link */
    { 0u, 0u, 0u, 0u },            /* Poly Yoshi */
    { 0u, 0u, 0u, 0u },            /* Poly Captain Falcon */
    { 0u, 0u, 0u, 0u },            /* Poly Kirby */
    { 0u, 0u, 0u, 0u },            /* Poly Pikachu */
    { 0u, 0u, 0u, 0u },            /* Poly Jigglypuff */
    { 0u, 0u, 0u, 0u },            /* Poly Ness */
    { 0u, 1u, 2u, 3u }             /* Giant Donkey Kong */
};

static const u8 dNdsFTParamCostumeTeam[nFTKindEnumCount][3] =
{
    { 0u, 3u, 4u },                /* Mario */
    { 1u, 2u, 3u },                /* Fox */
    { 2u, 3u, 4u },                /* Donkey Kong */
    { 0u, 4u, 3u },                /* Samus */
    { 3u, 2u, 0u },                /* Luigi */
    { 2u, 3u, 0u },                /* Link */
    { 1u, 2u, 0u },                /* Yoshi */
    { 1u, 5u, 2u },                /* Captain Falcon */
    { 3u, 2u, 4u },                /* Kirby */
    { 1u, 2u, 3u },                /* Pikachu */
    { 1u, 2u, 3u },                /* Jigglypuff */
    { 0u, 2u, 3u },                /* Ness */
    { 0u, 0u, 0u },                /* Master Hand */
    { 0u, 0u, 0u },                /* Metal Mario */
    { 0u, 0u, 0u },                /* Poly Mario */
    { 0u, 0u, 0u },                /* Poly Fox */
    { 0u, 0u, 0u },                /* Poly Donkey Kong */
    { 0u, 0u, 0u },                /* Poly Samus */
    { 0u, 0u, 0u },                /* Poly Luigi */
    { 0u, 0u, 0u },                /* Poly Link */
    { 0u, 0u, 0u },                /* Poly Yoshi */
    { 0u, 0u, 0u },                /* Poly Captain Falcon */
    { 0u, 0u, 0u },                /* Poly Kirby */
    { 0u, 0u, 0u },                /* Poly Pikachu */
    { 0u, 0u, 0u },                /* Poly Jigglypuff */
    { 0u, 0u, 0u },                /* Poly Ness */
    { 2u, 3u, 4u }                 /* Giant Donkey Kong */
};

/* BattleShip ftparam.c:2641-2651 is a bare index with no bounds check, so out
 * of range it over-reads rodata and there is no behavior to preserve. In range
 * these return the exact source bytes; out of range they return the colour
 * unchanged, which is the policy the previous shim already used, rather than
 * reading past a 189-byte table. */
s32 ftParamGetCostumeCommonID(s32 fkind, s32 color)
{
    if ((fkind < 0) || (fkind >= (s32)nFTKindEnumCount) ||
        (color < 0) || (color >= 4))
    {
        return color;
    }
    return (s32)dNdsFTParamCostumeRoyal[fkind][color];
}

s32 ftParamGetCostumeTeamID(s32 fkind, s32 color)
{
    if ((fkind < 0) || (fkind >= (s32)nFTKindEnumCount) ||
        (color < 0) || (color >= 3))
    {
        return color;
    }
    return (s32)dNdsFTParamCostumeTeam[fkind][color];
}



__attribute__((weak)) void ftKirbyCopyLinkSpecialNDestroyBoomerang(GObj *fighter_gobj)
{
    (void)fighter_gobj;
}

__attribute__((weak)) void ftKirbyCopyLinkSpecialNGetSetStatus(GObj *fighter_gobj)
{
    /* Kirby is not a production fighter yet. The boomerang source keeps this
     * cross-fighter callback, and the weak seam is replaced by Kirby's real
     * source TU when that roster row lands. No live Link-owned boomerang can
     * reach this arm because its parent fkind is Link. */
    (void)fighter_gobj;
}

__attribute__((weak)) void ftLinkSpecialNDestroyBoomerang(GObj *fighter_gobj)
{
    (void)fighter_gobj;
}

#if !NDS_P2_1P_GAME
void ftBossCommonSetNextAttackWait(GObj *fighter_gobj)
{
    (void)fighter_gobj;
}

void ftBossCommonSetDefaultLineID(GObj *fighter_gobj)
{
    (void)fighter_gobj;
}
#endif

void ftParamSetModelPartDefaultID(GObj *fighter_gobj, s32 joint_id,
                                  s32 modelpart_id)
{
    FTStruct *fp = (fighter_gobj != NULL) ? ftGetStruct(fighter_gobj) : NULL;
    s32 slot;

    if ((fp == NULL) || (joint_id < nFTPartsJointCommonStart) ||
        ((u32)joint_id >= ARRAY_COUNT(fp->joints)))
    {
        return;
    }
    slot = joint_id - nFTPartsJointCommonStart;
    if ((u32)slot >= ARRAY_COUNT(fp->modelpart_status))
    {
        return;
    }
    /* BattleShip ftKirbySpecialNCopyInitCopyVars writes copy_id immediately
     * before this call, then resets all modelparts.  Make the DS-native hat
     * executable first. If the synchronous NitroFS beat fails, reject the copy
     * state and leave joint 6 canonical rather than reporting a copied fighter
     * whose required native geometry cannot draw. */
    if (((fp->fkind == nFTKindKirby) || (fp->fkind == nFTKindNKirby)) &&
        (joint_id == FTKIRBY_COPY_MODELPARTS_JOINT) &&
        (modelpart_id >= 3) && (modelpart_id <= 13))
    {
        /* Polygon Kirby keeps copied gameplay in BattleShip (LoseCopy excludes
         * nFTKindNKirby). Its native owner has no copy-hat geometry, so suppress
         * only the visual hat and leave both copy-state unions untouched. */
        if (fp->fkind == nFTKindNKirby)
        {
            NDS_DIAG(gNdsNativeKirbyHatSuppressedCount++);
            modelpart_id = 0;
        }
        /* P4 S8: a Kirby holding a content's power defaults to the power's
         * Remix hat, whose image is bound in the source hat's place; the
         * parent's copy row the source passes first draws nothing. */
        else if (ndsP4KirbyDefaultHat(fp, modelpart_id) != FALSE)
        {
        }
        else
        {
            /* LOAD THE LOW HAT ONLY WHEN THIS FIGHTER CAN EVER SELECT IT.
             *
             * Both details used to load here unconditionally, about 8,556
             * bytes each, and measured on 2026-09-21 (Kirby vs Fox, Dream
             * Land) that took the general heap from 41,684 free to 23,204 --
             * past the 25,600 at which ifCommonSetMaxNumGObj freezes the GObj
             * cap for the rest of the match. From there no further effect
             * object can be created, which is the shape of the intermittent
             * missing VFX. The low image is 7,636 of those bytes.
             *
             * Whether the low hat is reachable at all is decided once, at
             * fighter creation: scvsbattle.c / sc1pgame.c / sc1ptrainingmode.c
             * / scexplain.c all set `desc.detail` to High when
             * `pl_count + cp_count < 3` and Low otherwise, and ftmanager.c:704
             * copies it into BOTH detail_curr and detail_base. Every later
             * writer of detail_curr in a match raises it to High -- the pause
             * camera (ifcommon.c), the dead-up-fall (ftcommondead.c),
             * sc1pgame -- and the two restore paths, ftmain.c:4412 and the
             * pause's saved value, put back detail_base. So a fighter whose
             * base detail is High can never present the low hat, and loading
             * it buys nothing.
             *
             * detail_curr is tested beside detail_base rather than instead of
             * it because scautodemo.c drives detail directly on its own
             * fighters; the union is a superset of what can be reached, which
             * is the safe side of this predicate.
             *
             * THIS DOES NOT BY ITSELF CLEAR THE LATCH EVERYWHERE. On Dream
             * Land it does: 23,204 + 7,636 = 30,840, over the floor. On
             * Saffron City, whose measured free-min with Kirby present is
             * 32,504, a copy still lands near 21,700 and the cap still
             * freezes. A null-safe maker caller is the fix for that, not this
             * lever. */
            u32 hat_low_reachable =
                ((fp->detail_base == nFTPartsDetailLow) ||
                 (fp->detail_curr == nFTPartsDetailLow)) ? 1u : 0u;

            /* A scene that admitted its hats up front (VS, the 1P ladder)
             * binds every detail it admitted, a pointer copy: a fighter made
             * holding a power (the Kirby Team, ftmanager.c:634) gets here
             * before ftmanager.c:704 sets its detail, so the detail fields
             * are stale and cannot say which hat it will draw. */
            if (ndsRendererNativeKirbyHatMatchHasPart(
                    (u32)modelpart_id, 1u) != FALSE)
            {
                hat_low_reachable = 1u;
            }
            /* The slot the draw will look the hat up by. A battle fighter's
             * nds_slot is its player, but it is recorded only after
             * ftManagerMakeFighter returns (reloc_backend_fighter_model.c),
             * and a fighter made holding a power reaches this inside it
             * (ftManagerInitFighter, ftmanager.c:634), where nds_slot is still
             * the previous occupant's or zero. */
            u32 hat_slot = (gNdsSceneManagerCurrIsBattle != 0u) ?
                (u32)fp->player : (u32)fp->nds_slot;
            /* Owner 2026-10-19: a fighter kept low in a battle of three or four
             * (ndsFTManagerLowDetailOnly) never draws the high hat, and the
             * match admitted none (nds_kirby_hat_residency.c). */
            const u32 hat_high_reachable =
                (ndsFTManagerLowDetailOnly(fp->player) == FALSE) ? 1u : 0u;

            if (((hat_high_reachable != 0u) &&
                 (ndsRendererNativeEnsureKirbyCopyHat(
                      hat_slot, (u32)modelpart_id, 0u) == FALSE)) ||
                ((hat_low_reachable != 0u) &&
                 (ndsRendererNativeEnsureKirbyCopyHat(
                      hat_slot, (u32)modelpart_id, 1u) == FALSE)))
            {
                /* A hat that cannot be made resident does not draw; the
                 * copied power itself is set below exactly as the source
                 * sets it. This used to halt -- for good, mid-match: the
                 * Polygon Team's later waves bring powers the match never
                 * admitted (owner playtest 2026-10-01). Counted, not silent. */
                NDS_DIAG(gNdsKirbyHatMissingCount++);
            }
        }
    }
    /* BattleShip ftparam.c:821-828 changes only the reset/default id. The live
     * part remains untouched until the normal reset path consumes this value. */
    fp->modelpart_status[slot].modelpart_id_base = (s8)modelpart_id;
    fp->is_modelpart_modify = TRUE;
}
