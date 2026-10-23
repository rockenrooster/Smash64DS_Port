/* ftDisplayMainProcDisplay, the fighter display proc the source installs,
 * run on the DTCM hot stack (its lean run nests in place under it), with the
 * source's invisibility gate: a fighter stays undrawn from match setup until
 * its own entry presents it (the native submit replaces the decomp body that
 * held the gate). A slice of reloc_backend.c. */

#include <port/coroutine.h>

static void ndsFtDisplayMainProcDisplayBody(GObj *fighter_gobj);

static unsigned int ndsFtDisplayMainProcDisplayOnHotStack(void *arg)
{
    ndsFtDisplayMainProcDisplayBody((GObj *)arg);
    return 0u;
}

/* P2-2p8 (2026-09-27): the fighter display proc runs on the DTCM hot
 * stack; its lean run already did and now runs in place under it (FTR
 * -3.7K in the same-ROM A/B with the stage owner prep). */
void ftDisplayMainProcDisplay(GObj *fighter_gobj)
{
    (void)ndsDtcmHotStackRun(ndsFtDisplayMainProcDisplayOnHotStack,
                             fighter_gobj);
}

static void ndsFtDisplayMainProcDisplayBody(GObj *fighter_gobj)
{
    extern volatile u32 gNdsVSResultsFighterDisplayCount;

    if (gSCManagerSceneData.scene_curr == nSCKindVSResults)
    {
        NDS_DIAG(gNdsVSResultsFighterDisplayCount++);
    }
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
}

sb32 (*dLBCommonFuncMatrixList[])(void) = { NULL };

