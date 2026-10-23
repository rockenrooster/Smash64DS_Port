/* The damage status helpers reloc_backend_ftmain_status_compat.c uses: which
 * statuses are damage statuses and the motion each one plays. A slice of
 * reloc_backend.c. */


static s32 ndsFTCommonDamageMotionForStatus(s32 status_id)
{
    if ((status_id >= nFTCommonStatusDamageHi1) &&
        (status_id <= nFTCommonStatusDamageAir3))
    {
        return nFTCommonMotionDamageHi1 +
               (status_id - nFTCommonStatusDamageHi1);
    }
    if ((status_id == nFTCommonStatusDamageE1) ||
        (status_id == nFTCommonStatusDamageE2))
    {
        return nFTCommonMotionDamageE;
    }
    if ((status_id >= nFTCommonStatusDamageFlyHi) &&
        (status_id <= nFTCommonStatusDamageFlyRoll))
    {
        return nFTCommonMotionDamageFlyHi +
               (status_id - nFTCommonStatusDamageFlyHi);
    }
    if (status_id == nFTCommonStatusWallDamage)
    {
        return nFTCommonMotionWallDamage;
    }
    return -1;
}

static sb32 ndsFTCommonDamageIsStatus(s32 status_id)
{
    return ((status_id >= nFTCommonStatusDamageStart) &&
            (status_id <= nFTCommonStatusWallDamage)) ? TRUE : FALSE;
}





































