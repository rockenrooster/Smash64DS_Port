#include <nds/nds_ftanim_track.h>

/* `used` on every counter: `--gc-sections` has removed diagnostic globals here
 * before and Boundary reported "Missing ELF symbol" rather than a flag. */
__attribute__((used)) volatile u32 gNdsFtAnimTrackDispatch =
    NDS_R2_FTANIM_TRACK_DISPATCH;
__attribute__((used)) volatile u32 gNdsFtAnimTrackBinds;
__attribute__((used)) volatile u32 gNdsFtAnimTrackBindMiss;
__attribute__((used)) volatile u32 gNdsFtAnimTrackBindFull;
__attribute__((used)) volatile u32 gNdsFtAnimTrackSteps;
__attribute__((used)) volatile u32 gNdsFtAnimTrackEarlyOut;
__attribute__((used)) volatile u32 gNdsFtAnimTrackRowsRun;
__attribute__((used)) volatile u32 gNdsFtAnimTrackOracleRows;
__attribute__((used)) volatile u32 gNdsFtAnimTrackOracleBad;
__attribute__((used)) volatile u32 gNdsFtAnimTrackOracleFirst;


s32 ndsFtAnimTrackBeginClip(DObj *root_dobj, const void *figatree)
{
    (void)root_dobj;
    (void)figatree;
    return -1;
}

s32 ndsFtAnimTrackBindJoint(DObj *dobj, s32 base, s32 index)
{
    (void)dobj;
    (void)base;
    (void)index;
    return FALSE;
}

s32 ndsFtAnimTrackIsDense(const void *p)
{
    (void)p;
    return FALSE;
}

void ndsFtAnimTrackStep(DObj *root_dobj)
{
    (void)root_dobj;
}

