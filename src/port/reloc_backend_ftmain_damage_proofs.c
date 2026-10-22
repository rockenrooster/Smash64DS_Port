
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





































#define NDS_STAGE_MPLIVEHIT_SHIELD_CONTACT_IS_SHIELD (1u << 0)
#define NDS_STAGE_MPLIVEHIT_SHIELD_CONTACT_DETECT    (1u << 1)
#define NDS_STAGE_MPLIVEHIT_SHIELD_CONTACT_ACTIVE    (1u << 2)
#define NDS_STAGE_MPLIVEHIT_SHIELD_CONTACT_JOINT     (1u << 3)
#define NDS_STAGE_MPLIVEHIT_SHIELD_CONTACT_SPHERE    (1u << 4)
#define NDS_STAGE_MPLIVEHIT_SHIELD_CONTACT_STATS     (1u << 5)
#define NDS_STAGE_MPLIVEHIT_SHIELD_CONTACT_CLEAR     (1u << 6)
#define NDS_STAGE_MPLIVEHIT_SHIELD_CONTACT_RESTORE   (1u << 7)
#define NDS_STAGE_MPLIVEHIT_SHIELD_CONTACT_SKIP_OFF  (1u << 8)
#define NDS_STAGE_MPLIVEHIT_SHIELD_CONTACT_SKIP_DET  (1u << 9)
#define NDS_STAGE_MPLIVEHIT_SHIELD_CONTACT_SKIP_HIT  (1u << 10)
#define NDS_STAGE_MPLIVEHIT_SHIELD_CONTACT_SKIP_REST (1u << 11)
#define NDS_STAGE_MPLIVEHIT_SHIELD_CONTACT_DETOFF    (1u << 12)
#define NDS_STAGE_MPLIVEHIT_SHIELD_CONTACT_DETOFF_HIT (1u << 13)
#define NDS_STAGE_MPLIVEHIT_SHIELD_CONTACT_DETOFF_REST (1u << 14)
#define NDS_STAGE_MPLIVEHIT_SHIELD_CONTACT_HEALTH    (1u << 15)
#define NDS_STAGE_MPLIVEHIT_SHIELD_CONTACT_BREAK     (1u << 16)
#define NDS_STAGE_MPLIVEHIT_SHIELD_CONTACT_HEAL      (1u << 17)
#define NDS_STAGE_MPLIVEHIT_SHIELD_CONTACT_BREAK_CLEAR (1u << 18)
#define NDS_STAGE_MPLIVEHIT_SHIELD_CONTACT_TAIL_CLEAR (1u << 19)
#define NDS_STAGE_MPLIVEHIT_SHIELD_CONTACT_SPECIAL_CLEAR (1u << 20)
#define NDS_STAGE_MPLIVEHIT_SHIELD_CONTACT_HITLAG_MUL_CLEAR (1u << 21)
#define NDS_STAGE_MPLIVEHIT_SHIELD_CONTACT_REPEAT    (1u << 22)
