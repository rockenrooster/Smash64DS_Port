/* Fighter movement, the natural moveset drive: its phases and recovery, when
 * a move has settled, the specials' mask and advance, teetering and the
 * motion witnesses. A slice of reloc_backend.c, after
 * reloc_backend_movement.c. */

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
        NDS_DIAG(gNdsFighterNaturalCombatStallCount++);
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

static sb32 ndsFighterNaturalSpecialsStartNext(FTStruct *fp[2])
{
    ndsFighterNaturalSpecialsUpdateMask();
    {
        FTStruct *samus = fp[gNdsFighterSamusSpecialsSlot];

        if (ndsFighterNaturalIsSamus(samus) != FALSE)
        {
            /* BattleShip stores a full Charge Shot at passive charge 7, then a
             * fresh neutral-B from Wait takes Start -> End and launches it.
             * Run that exact source sequence before down-B. */
            if ((gNdsFighterSamusSpecialsNPressFrames == 0u) ||
                (gNdsFighterSamusSpecialsNLoopFrames == 0u) ||
                (gNdsFighterSamusSpecialsNChargeMax <
                 NDS_FIGHTER_SAMUS_CHARGE_MAX) ||
                (gNdsFighterSamusSpecialsNFullWaitFrames == 0u) ||
                (gNdsFighterSamusSpecialsNReleasePressFrames == 0u) ||
                (gNdsFighterSamusSpecialsNEndFrames == 0u) ||
                (gNdsFighterSamusSpecialsNReleaseWaitFrames <
                 NDS_FIGHTER_NATURAL_SPECIAL_SETTLE_FRAMES_REQUIRED))
            {
                ndsFighterNaturalSpecialsSetPhase(
                    nNDSNaturalSpecialsPhaseSamusNFullCharge);
                return FALSE;
            }
            if ((gNdsFighterSamusSpecialsLwPressFrames == 0u) ||
                (gNdsFighterSamusSpecialsLwFrames == 0u) ||
                (gNdsFighterSamusSpecialsLwWaitFrames <
                 NDS_FIGHTER_NATURAL_SPECIAL_SETTLE_FRAMES_REQUIRED))
            {
                ndsFighterNaturalSpecialsSetPhase(
                    nNDSNaturalSpecialsPhaseSamusLw);
                return FALSE;
            }
        }
    }
#if NDS_P2_DONKEY
    {
        FTStruct *donkey = fp[gNdsFighterDonkeySpecialsSlot];

        if (ndsFighterNaturalIsDonkey(donkey) != FALSE)
        {
            if ((gNdsFighterDonkeySpecialsNStorePressFrames == 0u) ||
                (gNdsFighterDonkeySpecialsNStoredChargeMax <
                 NDS_FIGHTER_DONKEY_GIANTPUNCH_STORE_CHARGE_REQUIRED) ||
                (gNdsFighterDonkeySpecialsNStoredWaitFrames <
                 NDS_FIGHTER_DONKEY_SPECIAL_SETTLE_FRAMES_REQUIRED))
            {
                ndsFighterNaturalSpecialsSetPhase(
                    nNDSNaturalSpecialsPhaseDonkeyNCharge);
                return FALSE;
            }
            if ((gNdsFighterDonkeySpecialsNResumePressFrames == 0u) ||
                (gNdsFighterDonkeySpecialsNReleaseTapFrames == 0u) ||
                (gNdsFighterDonkeySpecialsNEndFrames == 0u) ||
                (gNdsFighterDonkeySpecialsNReleaseChargeMax <
                 NDS_FIGHTER_DONKEY_GIANTPUNCH_STORE_CHARGE_REQUIRED) ||
                (gNdsFighterDonkeySpecialsNPassiveResetFrames == 0u) ||
                (gNdsFighterDonkeySpecialsNReleaseWaitFrames <
                 NDS_FIGHTER_DONKEY_SPECIAL_SETTLE_FRAMES_REQUIRED))
            {
                ndsFighterNaturalSpecialsSetPhase(
                    nNDSNaturalSpecialsPhaseDonkeyNRelease);
                return FALSE;
            }
            /* The owner-reported full-charge cue was outside the old proof:
             * that tour intentionally stored at charge 2. Add a natural-input
             * leg that lets BattleShip itself reach charge 10, auto-cancel to
             * Wait, and execute the source CommonSpecialNCharge effect script. */
            if ((gNdsFighterDonkeySpecialsNStoredChargeMax <
                 NDS_FIGHTER_DONKEY_GIANTPUNCH_FULL_CHARGE_REQUIRED) ||
                ((gNdsFighterEffectKindMask2 &
                  NDS_FIGHTER_DONKEY_CHARGE_SPARKLE_MASK2) == 0u))
            {
                ndsFighterNaturalSpecialsSetPhase(
                    nNDSNaturalSpecialsPhaseDonkeyNFullCharge);
                return FALSE;
            }
            if ((gNdsFighterDonkeySpecialsHiPressFrames == 0u) ||
                (gNdsFighterDonkeySpecialsHiFrames == 0u) ||
                (gNdsFighterDonkeySpecialsHiWaitFrames <
                 NDS_FIGHTER_DONKEY_SPECIAL_SETTLE_FRAMES_REQUIRED))
            {
                ndsFighterNaturalSpecialsSetPhase(
                    nNDSNaturalSpecialsPhaseDonkeyHi);
                return FALSE;
            }
            if ((gNdsFighterDonkeySpecialsLwPressFrames == 0u) ||
                (gNdsFighterDonkeySpecialsLwStartFrames == 0u) ||
                (gNdsFighterDonkeySpecialsLwLoopFrames == 0u) ||
                (gNdsFighterDonkeySpecialsLwRepeatPressFrames == 0u) ||
                (gNdsFighterDonkeySpecialsLwEndFrames == 0u) ||
                (gNdsFighterDonkeySpecialsLwWaitFrames <
                 NDS_FIGHTER_DONKEY_SPECIAL_SETTLE_FRAMES_REQUIRED))
            {
                ndsFighterNaturalSpecialsSetPhase(
                    nNDSNaturalSpecialsPhaseDonkeyLw);
                return FALSE;
            }
        }
    }
#endif
    if ((ndsFighterNaturalIsMarioSpecialFamily(
             fp[gNdsFighterSpecialsMarioSlot]) != FALSE) &&
        ((gNdsFighterSpecialsProofMask &
         NDS_FIGHTER_SPECIALS_MARIO_LW_MASK) !=
         NDS_FIGHTER_SPECIALS_MARIO_LW_MASK))
    {
        ndsFighterNaturalSpecialsSetPhase(nNDSNaturalSpecialsPhaseMarioLw);
        return FALSE;
    }
    if ((ndsFighterNaturalIsMarioSpecialFamily(
             fp[gNdsFighterSpecialsMarioSlot]) != FALSE) &&
        ((gNdsFighterSpecialsProofMask &
         NDS_FIGHTER_SPECIALS_MARIO_HI_MASK) !=
         NDS_FIGHTER_SPECIALS_MARIO_HI_MASK))
    {
        ndsFighterNaturalSpecialsSetPhase(nNDSNaturalSpecialsPhaseMarioHi);
        return FALSE;
    }
    if ((fp[gNdsFighterSpecialsFoxSlot] != NULL) &&
        (fp[gNdsFighterSpecialsFoxSlot]->fkind == nFTKindFox) &&
        ((gNdsFighterSpecialsProofMask &
         NDS_FIGHTER_SPECIALS_FOX_HI_MASK) !=
         NDS_FIGHTER_SPECIALS_FOX_HI_MASK))
    {
        ndsFighterNaturalSpecialsSetPhase(nNDSNaturalSpecialsPhaseFoxHi);
        return FALSE;
    }
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

static void ndsFighterNaturalSpecialsRecordDamageMax(
    FTStruct *fp, volatile u32 *max_damage)
{
    u32 i;

    if ((fp == NULL) || (max_damage == NULL))
    {
        return;
    }
    for (i = 0u; i < FTATTACKCOLL_NUM_MAX; i++)
    {
        const FTAttackColl *attack = &fp->attack_colls[i];

        if ((attack->attack_state != nGMAttackStateOff) &&
            (attack->damage > 0) && ((u32)attack->damage > *max_damage))
        {
            *max_damage = (u32)attack->damage;
        }
    }
}

static void ndsFighterNaturalSpecialsRecord(FTStruct *fp[2])
{
    FTStruct *mario;
    FTStruct *fox;
#if NDS_P2_DONKEY
    FTStruct *donkey;
#endif
    FTStruct *samus;

    if (ndsFighterNaturalSpecialsProofEnabled() == FALSE)
    {
        return;
    }
    gNdsFighterSpecialsProofPhase = sNdsNaturalSpecialsPhase;
    gNdsFighterSpecialsProofPhaseFrames = sNdsNaturalSpecialsPhaseFrames;
    mario = fp[gNdsFighterSpecialsMarioSlot];
    fox = fp[gNdsFighterSpecialsFoxSlot];
    if (ndsFighterNaturalIsMarioSpecialFamily(mario) == FALSE)
    {
        mario = NULL;
    }
    if ((fox == NULL) || (fox->fkind != nFTKindFox))
    {
        fox = NULL;
    }
#if NDS_P2_DONKEY
    donkey = fp[gNdsFighterDonkeySpecialsSlot];
    if (ndsFighterNaturalIsDonkey(donkey) == FALSE)
    {
        donkey = NULL;
    }
#endif

    if (mario != NULL)
    {
        if ((sNdsNaturalSpecialsPhase ==
                nNDSNaturalSpecialsPhaseMarioHi) ||
            (sNdsNaturalSpecialsPhase ==
                nNDSNaturalSpecialsPhaseSettleMarioHi))
        {
            ndsFighterNaturalSpecialsRecordRoot(
                mario, &gNdsFighterSpecialsMarioHiRootYMilli);
            ndsFighterNaturalSpecialsRecordDamageMax(
                mario, &gNdsFighterSpecialsMarioHiDamageMax);
        }
        if ((sNdsNaturalSpecialsPhase ==
                nNDSNaturalSpecialsPhaseMarioLw) ||
            (sNdsNaturalSpecialsPhase ==
                nNDSNaturalSpecialsPhaseSettleMarioLw))
        {
            ndsFighterNaturalSpecialsRecordDamageMax(
                mario, &gNdsFighterSpecialsMarioLwDamageMax);
        }
        if ((mario->fkind == nFTKindMario) &&
            (mario->status_id == nFTMarioStatusSpecialHi))
        {
            gNdsFighterSpecialsMarioHiFrames++;
        }
        else if ((mario->fkind == nFTKindMario) &&
                 (mario->status_id == nFTMarioStatusSpecialAirHi))
        {
            gNdsFighterSpecialsMarioAirHiFrames++;
        }
#if NDS_P2_LUIGI
        else if ((mario->fkind == nFTKindLuigi) &&
                 (mario->status_id == nFTLuigiStatusSpecialHi))
        {
            gNdsFighterSpecialsMarioHiFrames++;
        }
        else if ((mario->fkind == nFTKindLuigi) &&
                 (mario->status_id == nFTLuigiStatusSpecialAirHi))
        {
            gNdsFighterSpecialsMarioAirHiFrames++;
        }
#endif
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

        if ((mario->fkind == nFTKindMario) &&
            (mario->status_id == nFTMarioStatusSpecialLw))
        {
            gNdsFighterSpecialsMarioLwFrames++;
        }
        else if ((mario->fkind == nFTKindMario) &&
                 (mario->status_id == nFTMarioStatusSpecialAirLw))
        {
            gNdsFighterSpecialsMarioAirLwFrames++;
        }
#if NDS_P2_LUIGI
        else if ((mario->fkind == nFTKindLuigi) &&
                 (mario->status_id == nFTLuigiStatusSpecialLw))
        {
            gNdsFighterSpecialsMarioLwFrames++;
        }
        else if ((mario->fkind == nFTKindLuigi) &&
                 (mario->status_id == nFTLuigiStatusSpecialAirLw))
        {
            gNdsFighterSpecialsMarioAirLwFrames++;
        }
#endif
        if (((gNdsFighterSpecialsMarioLwFrames > 0u) ||
             (gNdsFighterSpecialsMarioAirLwFrames > 0u)) &&
            (mario->status_id == nFTCommonStatusWait) &&
            (mario->ga == nMPKineticsGround))
        {
            gNdsFighterSpecialsMarioLwWaitFrames++;
        }
    }
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
#if NDS_P2_DONKEY
    if (donkey != NULL)
    {
        u32 stored_charge = (u32)donkey->passive_vars.donkey.charge_level;

        if (stored_charge > gNdsFighterDonkeySpecialsNStoredChargeMax)
        {
            gNdsFighterDonkeySpecialsNStoredChargeMax = stored_charge;
        }
        if ((donkey->status_id == nFTDonkeyStatusSpecialNStart) ||
            (donkey->status_id == nFTDonkeyStatusSpecialAirNStart))
        {
            NDS_DIAG(gNdsFighterDonkeySpecialsNStartFrames++);
        }
        else if ((donkey->status_id == nFTDonkeyStatusSpecialNLoop) ||
                 (donkey->status_id == nFTDonkeyStatusSpecialAirNLoop))
        {
            NDS_DIAG(gNdsFighterDonkeySpecialsNLoopFrames++);
        }
        else if ((donkey->status_id == nFTDonkeyStatusSpecialNEnd) ||
                 (donkey->status_id == nFTDonkeyStatusSpecialAirNEnd) ||
                 (donkey->status_id == nFTDonkeyStatusSpecialNFull) ||
                 (donkey->status_id == nFTDonkeyStatusSpecialAirNFull))
        {
            u32 release_charge =
                (u32)donkey->status_vars.donkey.specialn.charge_level;

            gNdsFighterDonkeySpecialsNEndFrames++;
            if (release_charge > gNdsFighterDonkeySpecialsNReleaseChargeMax)
            {
                gNdsFighterDonkeySpecialsNReleaseChargeMax = release_charge;
            }
            if (donkey->passive_vars.donkey.charge_level == 0)
            {
                gNdsFighterDonkeySpecialsNPassiveResetFrames++;
            }
        }
        if ((sNdsNaturalSpecialsPhase ==
                nNDSNaturalSpecialsPhaseDonkeyNCharge) &&
            (gNdsFighterDonkeySpecialsNStorePressFrames > 0u) &&
            (stored_charge >=
             NDS_FIGHTER_DONKEY_GIANTPUNCH_STORE_CHARGE_REQUIRED) &&
            (donkey->status_id == nFTCommonStatusWait) &&
            (donkey->ga == nMPKineticsGround))
        {
            gNdsFighterDonkeySpecialsNStoredWaitFrames++;
        }
        if ((sNdsNaturalSpecialsPhase ==
                nNDSNaturalSpecialsPhaseDonkeyNRelease) &&
            (gNdsFighterDonkeySpecialsNEndFrames > 0u) &&
            (donkey->status_id == nFTCommonStatusWait) &&
            (donkey->ga == nMPKineticsGround))
        {
            gNdsFighterDonkeySpecialsNReleaseWaitFrames++;
        }

        if ((donkey->status_id == nFTDonkeyStatusSpecialHi) ||
            (donkey->status_id == nFTDonkeyStatusSpecialAirHi))
        {
            gNdsFighterDonkeySpecialsHiFrames++;
            if (donkey->ga == nMPKineticsGround)
            {
                NDS_DIAG(gNdsFighterDonkeySpecialsHiGroundGAFrames++);
            }
        }
        if ((sNdsNaturalSpecialsPhase == nNDSNaturalSpecialsPhaseDonkeyHi) &&
            (gNdsFighterDonkeySpecialsHiFrames > 0u) &&
            (donkey->status_id == nFTCommonStatusWait) &&
            (donkey->ga == nMPKineticsGround))
        {
            gNdsFighterDonkeySpecialsHiWaitFrames++;
        }

        if (donkey->status_id == nFTDonkeyStatusSpecialLwStart)
        {
            gNdsFighterDonkeySpecialsLwStartFrames++;
        }
        else if (donkey->status_id == nFTDonkeyStatusSpecialLwLoop)
        {
            gNdsFighterDonkeySpecialsLwLoopFrames++;
            if (donkey->status_vars.donkey.speciallw.is_loop != FALSE)
            {
                NDS_DIAG(gNdsFighterDonkeySpecialsLwLoopFlagFrames++);
            }
        }
        else if (donkey->status_id == nFTDonkeyStatusSpecialLwEnd)
        {
            gNdsFighterDonkeySpecialsLwEndFrames++;
        }
        if ((sNdsNaturalSpecialsPhase == nNDSNaturalSpecialsPhaseDonkeyLw) &&
            (gNdsFighterDonkeySpecialsLwEndFrames > 0u) &&
            (donkey->status_id == nFTCommonStatusWait) &&
            (donkey->ga == nMPKineticsGround))
        {
            gNdsFighterDonkeySpecialsLwWaitFrames++;
        }
    }
#endif
    samus = fp[gNdsFighterSamusSpecialsSlot];
    if (ndsFighterNaturalIsSamus(samus) == FALSE)
    {
        samus = NULL;
    }
    if (samus != NULL)
    {
        u32 charge_level = (u32)samus->passive_vars.samus.charge_level;

        if (charge_level > gNdsFighterSamusSpecialsNChargeMax)
        {
            gNdsFighterSamusSpecialsNChargeMax = charge_level;
        }
        if ((samus->status_id == nFTSamusStatusSpecialNStart) ||
            (samus->status_id == nFTSamusStatusSpecialAirNStart))
        {
            NDS_DIAG(gNdsFighterSamusSpecialsNStartFrames++);
        }
        else if (samus->status_id == nFTSamusStatusSpecialNLoop)
        {
            gNdsFighterSamusSpecialsNLoopFrames++;
        }
        else if ((samus->status_id == nFTSamusStatusSpecialNEnd) ||
                 (samus->status_id == nFTSamusStatusSpecialAirNEnd))
        {
            gNdsFighterSamusSpecialsNEndFrames++;
        }
        if ((sNdsNaturalSpecialsPhase ==
                nNDSNaturalSpecialsPhaseSamusNFullCharge) &&
            (charge_level == NDS_FIGHTER_SAMUS_CHARGE_MAX) &&
            (samus->status_id == nFTCommonStatusWait) &&
            (samus->ga == nMPKineticsGround))
        {
            gNdsFighterSamusSpecialsNFullWaitFrames++;
        }
        if ((sNdsNaturalSpecialsPhase ==
                nNDSNaturalSpecialsPhaseSamusNFullCharge) &&
            (gNdsFighterSamusSpecialsNEndFrames > 0u) &&
            (charge_level == 0u) &&
            (samus->status_id == nFTCommonStatusWait) &&
            (samus->ga == nMPKineticsGround))
        {
            gNdsFighterSamusSpecialsNReleaseWaitFrames++;
        }
        if (samus->status_id == nFTSamusStatusSpecialLw)
        {
            gNdsFighterSamusSpecialsLwFrames++;
        }
        if ((sNdsNaturalSpecialsPhase == nNDSNaturalSpecialsPhaseSamusLw) &&
            (gNdsFighterSamusSpecialsLwFrames > 0u) &&
            (samus->status_id == nFTCommonStatusWait) &&
            (samus->ga == nMPKineticsGround))
        {
            gNdsFighterSamusSpecialsLwWaitFrames++;
        }
    }
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
        return ndsFighterNaturalSpecialsStartNext(fp);
    }
    sNdsNaturalSpecialsPhaseFrames++;
    gNdsFighterSpecialsProofPhaseFrames = sNdsNaturalSpecialsPhaseFrames;
    if (sNdsNaturalSpecialsPhaseFrames >
        NDS_FIGHTER_NATURAL_MOVESET_PHASE_TIMEOUT)
    {
        NDS_DIAG(gNdsFighterNaturalCombatStallCount++);
        return FALSE;
    }
    switch (sNdsNaturalSpecialsPhase)
    {
    case nNDSNaturalSpecialsPhaseSamusNFullCharge:
        if ((gNdsFighterSamusSpecialsNChargeMax >=
             NDS_FIGHTER_SAMUS_CHARGE_MAX) &&
            (gNdsFighterSamusSpecialsNFullWaitFrames > 0u) &&
            (gNdsFighterSamusSpecialsNReleasePressFrames > 0u) &&
            (gNdsFighterSamusSpecialsNEndFrames > 0u) &&
            (gNdsFighterSamusSpecialsNReleaseWaitFrames >=
             NDS_FIGHTER_NATURAL_SPECIAL_SETTLE_FRAMES_REQUIRED) &&
            (ndsFighterNaturalSpecialsBothGroundWait(fp) != FALSE))
        {
            return ndsFighterNaturalSpecialsStartNext(fp);
        }
        break;
    case nNDSNaturalSpecialsPhaseSamusLw:
        if ((gNdsFighterSamusSpecialsLwPressFrames > 0u) &&
            (gNdsFighterSamusSpecialsLwFrames > 0u) &&
            (gNdsFighterSamusSpecialsLwWaitFrames >=
             NDS_FIGHTER_NATURAL_SPECIAL_SETTLE_FRAMES_REQUIRED) &&
            (ndsFighterNaturalSpecialsBothGroundWait(fp) != FALSE))
        {
            return ndsFighterNaturalSpecialsStartNext(fp);
        }
        break;
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
            return ndsFighterNaturalSpecialsStartNext(fp);
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
            return ndsFighterNaturalSpecialsStartNext(fp);
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
            return ndsFighterNaturalSpecialsStartNext(fp);
        }
        break;
#if NDS_P2_DONKEY
    case nNDSNaturalSpecialsPhaseDonkeyNCharge:
        if ((gNdsFighterDonkeySpecialsNStorePressFrames > 0u) &&
            (gNdsFighterDonkeySpecialsNStoredChargeMax >=
             NDS_FIGHTER_DONKEY_GIANTPUNCH_STORE_CHARGE_REQUIRED) &&
            (gNdsFighterDonkeySpecialsNStoredWaitFrames >=
             NDS_FIGHTER_DONKEY_SPECIAL_SETTLE_FRAMES_REQUIRED) &&
            (ndsFighterNaturalSpecialsBothGroundWait(fp) != FALSE))
        {
            return ndsFighterNaturalSpecialsStartNext(fp);
        }
        break;
    case nNDSNaturalSpecialsPhaseDonkeyNRelease:
        if ((gNdsFighterDonkeySpecialsNResumePressFrames > 0u) &&
            (gNdsFighterDonkeySpecialsNReleaseTapFrames > 0u) &&
            (gNdsFighterDonkeySpecialsNEndFrames > 0u) &&
            (gNdsFighterDonkeySpecialsNReleaseChargeMax >=
             NDS_FIGHTER_DONKEY_GIANTPUNCH_STORE_CHARGE_REQUIRED) &&
            (gNdsFighterDonkeySpecialsNPassiveResetFrames > 0u) &&
            (gNdsFighterDonkeySpecialsNReleaseWaitFrames >=
             NDS_FIGHTER_DONKEY_SPECIAL_SETTLE_FRAMES_REQUIRED) &&
            (ndsFighterNaturalSpecialsBothGroundWait(fp) != FALSE))
        {
            return ndsFighterNaturalSpecialsStartNext(fp);
        }
        break;
    case nNDSNaturalSpecialsPhaseDonkeyNFullCharge:
        if ((gNdsFighterDonkeySpecialsNStoredChargeMax >=
             NDS_FIGHTER_DONKEY_GIANTPUNCH_FULL_CHARGE_REQUIRED) &&
            ((gNdsFighterEffectKindMask2 &
              NDS_FIGHTER_DONKEY_CHARGE_SPARKLE_MASK2) != 0u) &&
            (ndsFighterNaturalSpecialsBothGroundWait(fp) != FALSE))
        {
            return ndsFighterNaturalSpecialsStartNext(fp);
        }
        break;
    case nNDSNaturalSpecialsPhaseDonkeyHi:
        if ((gNdsFighterDonkeySpecialsHiPressFrames > 0u) &&
            (gNdsFighterDonkeySpecialsHiFrames > 0u) &&
            (gNdsFighterDonkeySpecialsHiWaitFrames >=
             NDS_FIGHTER_DONKEY_SPECIAL_SETTLE_FRAMES_REQUIRED) &&
            (ndsFighterNaturalSpecialsBothGroundWait(fp) != FALSE))
        {
            return ndsFighterNaturalSpecialsStartNext(fp);
        }
        break;
    case nNDSNaturalSpecialsPhaseDonkeyLw:
        if ((gNdsFighterDonkeySpecialsLwPressFrames > 0u) &&
            (gNdsFighterDonkeySpecialsLwStartFrames > 0u) &&
            (gNdsFighterDonkeySpecialsLwLoopFrames > 0u) &&
            (gNdsFighterDonkeySpecialsLwRepeatPressFrames > 0u) &&
            (gNdsFighterDonkeySpecialsLwEndFrames > 0u) &&
            (gNdsFighterDonkeySpecialsLwWaitFrames >=
             NDS_FIGHTER_DONKEY_SPECIAL_SETTLE_FRAMES_REQUIRED) &&
            (ndsFighterNaturalSpecialsBothGroundWait(fp) != FALSE))
        {
            return ndsFighterNaturalSpecialsStartNext(fp);
        }
        break;
#endif
    default:
        break;
    }
    return FALSE;
}

static void ndsFighterNaturalCombatAdvancePhase(FTStruct *fp[2])
{
    NDSFighterNaturalMotionState *s0 = &sNdsFighterNaturalMotionStates[0];
    NDSFighterNaturalMotionState *s1 = &sNdsFighterNaturalMotionStates[1];
    FTStruct *attacker = fp[sNdsNaturalCombatAttackerSlot];
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
        NDS_DIAG(gNdsFighterNaturalCombatStallCount++);
    }

    switch (sNdsNaturalCombatPhase)
    {
    case nNDSNaturalCombatPhaseWait:
        if ((s0->wait_frames >=
                NDS_FIGHTER_NATURAL_MOTION_WAIT_FRAMES_REQUIRED) &&
            (s1->wait_frames >=
                NDS_FIGHTER_NATURAL_MOTION_WAIT_FRAMES_REQUIRED) &&
            (ndsFighterNaturalCombatBothGroundWait(fp) != FALSE) &&
            (dy <= NDS_FIGHTER_NATURAL_COMBAT_APPROACH_FLOOR_Y_RANGE))
        {
            gNdsFighterNaturalMotionWalkInputFrame =
                gNdsFighterNaturalMotionUpdateCount + 1u;
            sNdsNaturalCombatPassPressed = 0u;
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
            sNdsNaturalCombatApproachStagnant = 0u;
            ndsFighterNaturalCombatSetPhase(
                nNDSNaturalCombatPhaseSettleApproach);
        }
        else if (dy <= NDS_FIGHTER_NATURAL_COMBAT_APPROACH_FLOOR_Y_RANGE)
        {
            /* P2-3r3 (2026-08-23): arrival by body contact. A big fighter's
             * push radius can exceed the tuned stop range -- Donkey Kong vs
             * Fox reaches push equilibrium at dx=311.85 against the 240 stop
             * range, so the approach drove into the body forever (7,601
             * frames parked). When the drive is active at the same floor
             * level and dx stops shrinking for a second, the pair is
             * touching: that IS arrival, for every body size the roster will
             * ever stage. */
            f32 addx = (dx < 0.0F) ? -dx : dx;

            if ((sNdsNaturalCombatApproachLastDX - addx) > 0.5F)
            {
                sNdsNaturalCombatApproachStagnant = 0u;
            }
            else if (++sNdsNaturalCombatApproachStagnant >= 60u)
            {
                /* Adopt the contact distance as the pair's stop range so the
                 * settle and attack checks agree with it; an attack whiff
                 * retry may still shrink it, and if the pair re-parks the
                 * stagnation re-adopts it. */
                sNdsNaturalCombatApproachStagnant = 0u;
                sNdsNaturalCombatApproachStopRange = addx + 8.0F;
                ndsFighterNaturalCombatSetPhase(
                    nNDSNaturalCombatPhaseSettleApproach);
            }
            sNdsNaturalCombatApproachLastDX = addx;
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
                NDS_DIAG(gNdsFighterNaturalCombatStallCount++);
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
            sNdsNaturalCombatRollPressed = 0u;
            ndsFighterNaturalCombatSetPhase(
                nNDSNaturalCombatPhaseGuardRoll);
        }
        break;
    case nNDSNaturalCombatPhaseGuardRoll:
        if (gNdsFighterNaturalCombatRollFrames > 0u)
        {
            ndsFighterNaturalCombatSetPhase(
                nNDSNaturalCombatPhaseSettleGuardRoll);
        }
        break;
    case nNDSNaturalCombatPhaseSettleGuardRoll:
        if ((victim->status_id == nFTCommonStatusWait) &&
            (ndsFighterNaturalCombatSettled(fp) != FALSE))
        {
            /* The existing guard lifecycle proof should still measure a fresh
             * GuardOn -> Guard -> GuardOff after the roll. These are diagnostics
             * only; no fighter/controller state is written here. */
            gNdsFighterNaturalCombatGuardOnFrames = 0u;
            gNdsFighterNaturalCombatGuardFrames = 0u;
            gNdsFighterNaturalCombatGuardOffFrames = 0u;
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
            sNdsNaturalCombatAppealPressed = 0u;
            ndsFighterNaturalCombatSetPhase(nNDSNaturalCombatPhaseAppeal);
        }
        break;
    case nNDSNaturalCombatPhaseAppeal:
        if (gNdsFighterNaturalCombatAppealFrames > 0u)
        {
            ndsFighterNaturalCombatSetPhase(
                nNDSNaturalCombatPhaseSettleAppeal);
        }
        break;
    case nNDSNaturalCombatPhaseSettleAppeal:
        if ((attacker->status_id == nFTCommonStatusWait) &&
            (ndsFighterNaturalCombatSettled(fp) != FALSE))
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
            NDS_DIAG(gNdsFighterNaturalCombatStallCount++);
            ndsFighterNaturalCombatStartKOExit(victim);
        }
        break;
    case nNDSNaturalCombatPhaseProjectileObserve:
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
                NDS_DIAG(gNdsFighterNaturalCombatStallCount++);
                ndsFighterNaturalCombatStartKOExit(victim);
            }
            break;
        }
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
            NDS_DIAG(gNdsFighterNaturalCombatStallCount++);
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
            ndsFighterNaturalCombatSetPhase(
                nNDSNaturalCombatPhaseBattlePlayableDone);
        }
        break;
    default:
        break;
    }
}

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
            f32 y0 = fp[0]->coll_data.p_translate->y;
            f32 y1 = fp[1]->coll_data.p_translate->y;
            f32 dy = (y0 > y1) ? (y0 - y1) : (y1 - y0);
            u32 pass_slot = (y0 > y1) ? 0u : 1u;

            /* P2-3r3 (2026-08-23). A post-KO rebirth Fall can land the victim
             * on a pass-through platform (observed: Dream Land top platform,
             * y=1542, while the attacker ground-chased at y=0 and sat in
             * Catch for 6,125 frames). Both fighters are then legitimately in
             * ground Wait, so the grab drive mashed at an unreachable target
             * forever. Reuse the Wait-phase Down-tap pass drive: pulse the
             * elevated fighter down through its floor until the pair shares
             * a level, then chase and grab as before. Same re-arm latch; the
             * proof still exercises ftCommonPassCheckInputSuccess and forces
             * no position, collision, or fighter state. */
            if ((dy > NDS_FIGHTER_NATURAL_COMBAT_APPROACH_FLOOR_Y_RANGE) &&
                (fp[pass_slot]->status_id == nFTCommonStatusWait) &&
                (fp[pass_slot]->ga == nMPKineticsGround) &&
                ((fp[pass_slot]->coll_data.floor_flags &
                  MAP_VERTEX_COLL_PASS) != 0))
            {
                if (sNdsNaturalCombatPassPressed == 0u)
                {
                    stick_y[pass_slot] = -80;
                    sNdsNaturalCombatPassPressed = 1u;
                }
                else if (ABS(fp[pass_slot]->input.pl.stick_range.y) < 20)
                {
                    sNdsNaturalCombatPassPressed = 0u;
                }
                break;
            }
            if (dy <= NDS_FIGHTER_NATURAL_COMBAT_APPROACH_FLOOR_Y_RANGE)
            {
                sNdsNaturalCombatPassPressed = 0u;
            }

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

static sb32 ndsFighterNaturalSpecialsApplyInput(FTStruct *fp[2],
                                                u16 button[2],
                                                s8 stick_x[2],
                                                s8 stick_y[2])
{
    u32 mario_slot = gNdsFighterSpecialsMarioSlot;
    u32 fox_slot = gNdsFighterSpecialsFoxSlot;
#if NDS_P2_DONKEY
    u32 donkey_slot = gNdsFighterDonkeySpecialsSlot;
#endif
    u32 samus_slot = gNdsFighterSamusSpecialsSlot;
    (void)stick_x;

    if ((ndsFighterNaturalSpecialsProofEnabled() == FALSE) ||
        (sNdsNaturalSpecialsPhase == nNDSNaturalSpecialsPhaseIdle) ||
        (sNdsNaturalSpecialsPhase == nNDSNaturalSpecialsPhaseDone))
    {
        return FALSE;
    }

    switch (sNdsNaturalSpecialsPhase)
    {
    case nNDSNaturalSpecialsPhaseSamusNFullCharge:
        if ((sNdsNaturalSpecialsButtonPressed == 0u) &&
            (ndsFighterNaturalSpecialsBothGroundWait(fp) != FALSE))
        {
            /* First B starts the source Start -> Loop charging path. */
            button[samus_slot] = B_BUTTON;
            sNdsNaturalSpecialsButtonPressed = 1u;
            gNdsFighterSamusSpecialsNPressFrames++;
        }
        else if ((sNdsNaturalSpecialsButtonPressed == 1u) &&
                 (gNdsFighterSamusSpecialsNFullWaitFrames > 0u) &&
                 (ndsFighterNaturalSpecialsBothGroundWait(fp) != FALSE))
        {
            /* At charge 7 BattleShip destroyed the held charge object and
             * returned to Wait. A fresh B now makes Start's is_release path
             * enter End and launch the fully charged shot. */
            button[samus_slot] = B_BUTTON;
            sNdsNaturalSpecialsButtonPressed = 2u;
            gNdsFighterSamusSpecialsNReleasePressFrames++;
        }
        break;
    case nNDSNaturalSpecialsPhaseSamusLw:
        if ((sNdsNaturalSpecialsButtonPressed == 0u) &&
            (ndsFighterNaturalSpecialsBothGroundWait(fp) != FALSE))
        {
            button[samus_slot] = B_BUTTON;
            stick_y[samus_slot] = -80;
            sNdsNaturalSpecialsButtonPressed = 1u;
            gNdsFighterSamusSpecialsLwPressFrames++;
        }
        break;
    case nNDSNaturalSpecialsPhaseMarioHi:
        if (((gNdsFighterSpecialsMarioHiFrames > 0u) ||
             (ndsFighterNaturalMarioFamilyStatusIsSpecialHi(
                  fp[mario_slot]) != FALSE)) ||
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
             (ndsFighterNaturalMarioFamilyStatusIsSpecialLw(
                  fp[mario_slot]) != FALSE)) ||
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
#if NDS_P2_DONKEY
    case nNDSNaturalSpecialsPhaseDonkeyNCharge:
        if ((sNdsNaturalSpecialsButtonPressed == 0u) &&
            (ndsFighterNaturalSpecialsBothGroundWait(fp) != FALSE))
        {
            button[donkey_slot] = B_BUTTON;
            sNdsNaturalSpecialsButtonPressed = 1u;
            NDS_DIAG(gNdsFighterDonkeySpecialsNChargePressFrames++);
        }
        else if ((sNdsNaturalSpecialsButtonPressed == 1u) &&
                 ((fp[donkey_slot]->status_id ==
                       nFTDonkeyStatusSpecialNLoop) ||
                  (fp[donkey_slot]->status_id ==
                       nFTDonkeyStatusSpecialAirNLoop)) &&
                 ((u32)fp[donkey_slot]->passive_vars.donkey.charge_level >=
                  NDS_FIGHTER_DONKEY_GIANTPUNCH_STORE_CHARGE_REQUIRED))
        {
            /* BattleShip stores Giant Punch with Z from the charge loop. */
            button[donkey_slot] = Z_TRIG;
            sNdsNaturalSpecialsButtonPressed = 2u;
            gNdsFighterDonkeySpecialsNStorePressFrames++;
        }
        break;
    case nNDSNaturalSpecialsPhaseDonkeyNRelease:
        if ((sNdsNaturalSpecialsButtonPressed == 0u) &&
            (ndsFighterNaturalSpecialsBothGroundWait(fp) != FALSE))
        {
            button[donkey_slot] = B_BUTTON;
            sNdsNaturalSpecialsButtonPressed = 1u;
            gNdsFighterDonkeySpecialsNResumePressFrames++;
        }
        else if ((sNdsNaturalSpecialsButtonPressed == 1u) &&
                 ((fp[donkey_slot]->status_id ==
                       nFTDonkeyStatusSpecialNLoop) ||
                  (fp[donkey_slot]->status_id ==
                       nFTDonkeyStatusSpecialAirNLoop)))
        {
            /* A second B tap is the source release path; the status callback
             * copies the stored passive charge into status_vars then clears it. */
            button[donkey_slot] = B_BUTTON;
            sNdsNaturalSpecialsButtonPressed = 2u;
            gNdsFighterDonkeySpecialsNReleaseTapFrames++;
        }
        break;
    case nNDSNaturalSpecialsPhaseDonkeyNFullCharge:
        /* One B press starts the source loop. No further proof input is sent:
         * ftDonkeySpecialNLoopProcUpdate must earn all ten charges itself and
         * take its own full-charge cancel path. */
        if ((sNdsNaturalSpecialsButtonPressed == 0u) &&
            (ndsFighterNaturalSpecialsBothGroundWait(fp) != FALSE))
        {
            button[donkey_slot] = B_BUTTON;
            sNdsNaturalSpecialsButtonPressed = 1u;
            NDS_DIAG(gNdsFighterDonkeySpecialsNChargePressFrames++);
        }
        break;
    case nNDSNaturalSpecialsPhaseDonkeyHi:
        if ((sNdsNaturalSpecialsButtonPressed == 0u) &&
            (ndsFighterNaturalSpecialsBothGroundWait(fp) != FALSE))
        {
            button[donkey_slot] = B_BUTTON;
            stick_y[donkey_slot] = 80;
            sNdsNaturalSpecialsButtonPressed = 1u;
            gNdsFighterDonkeySpecialsHiPressFrames++;
        }
        break;
    case nNDSNaturalSpecialsPhaseDonkeyLw:
        if ((sNdsNaturalSpecialsButtonPressed == 0u) &&
            (ndsFighterNaturalSpecialsBothGroundWait(fp) != FALSE))
        {
            button[donkey_slot] = B_BUTTON;
            stick_y[donkey_slot] = -80;
            sNdsNaturalSpecialsButtonPressed = 1u;
            gNdsFighterDonkeySpecialsLwPressFrames++;
        }
        else if ((sNdsNaturalSpecialsButtonPressed == 1u) &&
                 (gNdsFighterDonkeySpecialsLwLoopFrames > 0u) &&
                 (fp[donkey_slot]->status_id ==
                      nFTDonkeyStatusSpecialLwLoop))
        {
            /* Hand Slap repeats only when its source interrupt sees a fresh B
             * tap during the loop.  One repeat is enough to qualify the seam. */
            button[donkey_slot] = B_BUTTON;
            sNdsNaturalSpecialsButtonPressed = 2u;
            gNdsFighterDonkeySpecialsLwRepeatPressFrames++;
        }
        break;
#endif
    default:
        break;
    }
    return TRUE;
}






static sb32 ndsFighterNaturalCombatRecoverTeeter(FTStruct *fp[2], s8 stick[2])
{
    u32 i;
    sb32 active = FALSE;

    for (i = 0u; i < 2u; i++)
    {
        if ((fp[i]->status_id == nFTCommonStatusOttottoWait) ||
            (fp[i]->status_id == nFTCommonStatusOttotto))
        {
            /* BattleShip keeps both teeter states interruptible.  Feed the
             * ordinary ground input back toward stage center so the source
             * ftCommonOttottoProcInterrupt path resolves through Turn/Walk;
             * never rewrite position, collision, or status for the proof. */
            stick[i] = (ndsFighterNaturalCombatPosX(fp[i]) > 0.0F) ? -40 : 40;
            active = TRUE;
        }
    }
    return active;
}

static void ndsFighterNaturalCombatApplyInput(FTStruct *fp[2])
{
    (void)fp;
    return;
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
    NDS_DIAG(gNdsFighterNaturalMotionControllerReadCount++);

    gcRunAll();
    gNdsFighterNaturalMotionRunAllCount++;
    gNdsFighterNaturalMotionUpdateCount++;

    for (i = 0u; i < 2u; i++)
    {
        ndsFighterNaturalMotionRecordSlot(i, fp[i]);
    }
    ndsFighterNaturalCombatRecordPair(fp[sNdsNaturalCombatAttackerSlot],
                                      fp[sNdsNaturalCombatVictimSlot]);
    ndsFighterNaturalSpecialsRecord(fp);
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
            NDS_FIGHTER_BATTLE_PLAYABLE_PASS)
       )
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
