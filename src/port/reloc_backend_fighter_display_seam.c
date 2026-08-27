void ftDisplayMainProcDisplay(GObj *fighter_gobj)
{
#if NDS_IMPORT_BATTLESHIP_VS_RESULTS
    extern volatile u32 gNdsVSResultsFighterDisplayCount;

    if (gSCManagerSceneData.scene_curr == nSCKindVSResults)
    {
        gNdsVSResultsFighterDisplayCount++;
    }
#endif
    if ((ndsFighterMarioFoxDisplayProofEnabled() != FALSE) &&
        (sNdsFighterDisplayProbeActive != FALSE))
    {
        ndsFighterMarioFoxRecordDisplayProbe(fighter_gobj);
        return;
    }
    if ((ndsFighterMarioFoxDLAllDrawProofEnabled() != FALSE) &&
        (sNdsFighterDLAllDrawProbeActive != FALSE))
    {
        ndsFighterMarioFoxRecordDLAllDrawFromDisplayCallback(fighter_gobj);
        return;
    }
    if ((ndsFighterMarioFoxPreviewLoopProofEnabled() != FALSE) &&
        (sNdsFighterPreviewLoopDisplayActive != FALSE))
    {
        ndsFighterPreviewLoopRecordDisplayFromCallback(fighter_gobj);
        return;
    }
    if ((ndsFighterMarioFoxGCDrawAllLoopProofEnabled() != FALSE) &&
        (sNdsFighterGCDrawAllLoopDisplayActive != FALSE))
    {
        ndsFighterGCDrawAllLoopRecordDisplayFromCallback(fighter_gobj);
        return;
    }
    if (ndsFighterMarioFoxWalkLoopProofEnabled() != FALSE)
    {
        gNdsFighterWalkLoopDisplayProbeCount++;
        return;
    }
    if (ndsFighterMarioFoxWaitProofEnabled() != FALSE)
    {
        gNdsFighterWaitDisplayProbeCount++;
    }
    if (ndsFighterMarioFoxWaitTickProofEnabled() != FALSE)
    {
        gNdsFighterWaitTickDisplayProbeCount++;
    }
    if (ndsFighterMarioFoxWaitGroundProofEnabled() != FALSE)
    {
        gNdsFighterWaitGroundDisplayProbeCount++;
    }
    if (ndsFighterMarioFoxWalkInputProofEnabled() != FALSE)
    {
        gNdsFighterWalkDisplayProbeCount++;
    }
#if NDS_RENDERER_HW_TRIANGLES
    /* THE SOURCE'S INVISIBILITY GATE, WHICH THIS PATH HAD DROPPED.
     *
     * Owner, 2026-08-23: "fighters are present before intros. The whole point
     * of intros is that the fighters are 'presented' during their intros."
     *
     * ftManagerMakeFighter puts every non-skip-entry fighter into
     * nFTCommonStatusEntry via ftCommonEntrySetStatus, which sets
     * `is_invisible` (ftcommonentry.c:49-56); ifCommonEntryFocusThread then
     * calls ftCommonAppearSetStatus one fighter at a time, and ftMainSetStatus
     * clears `is_invisible` as it does (ftmain.c:4462). So in the original a
     * fighter is UNDRAWN from match setup until its own intro reaches it, and
     * that is what makes an intro a presentation rather than a flourish over
     * an already-visible cast.
     *
     * The gate lives in ftDisplayMainProcDisplay (ftdisplaymain.c:1087-1092) --
     * but on this target that function is replaced wholesale by the native
     * submit below, so the decomp body holding the gate never runs and every
     * fighter drew from frame one. Reinstated here, at the seam that owns the
     * decision, rather than by teaching the renderer about fighter status. */
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);

        if ((fp != NULL) && (fp->is_invisible) &&
            (fp->display_mode == nDBDisplayModeMaster))
        {
            fp->is_magnify_show = FALSE;
            return;
        }
    }
    ndsFighterDisplayContractSubmit(fighter_gobj);
#endif
}

sb32 (*dLBCommonFuncMatrixList[])(void) = { NULL };

#if !NDS_IMPORT_BATTLESHIP_VS_RESULTS
f32 dSCSubsysFighterScales[] =
{
    1.0F, 1.0F, 1.0F, 1.0F, 1.0F, 1.0F,
    1.0F, 1.0F, 1.0F, 1.0F, 1.0F, 1.0F,
    1.0F, 1.0F, 1.0F, 1.0F
};

f32 scSubsysFighterGetLightAngleX(void)
{
    return 0.0F;
}

f32 scSubsysFighterGetLightAngleY(void)
{
    return 0.0F;
}

void scSubsysFighterSetLightParams(f32 light_angle_x, f32 light_angle_y,
                                    u8 r, u8 g, u8 b, u8 a)
{
    (void)light_angle_x;
    (void)light_angle_y;
    (void)r;
    (void)g;
    (void)b;
    (void)a;
}
#endif
