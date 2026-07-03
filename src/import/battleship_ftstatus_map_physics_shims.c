#include <ft/fighter.h>

#if NDS_IMPORT_BATTLESHIP_FTMANAGER
void ftCommonHammerFallSetStatus(GObj *fighter_gobj);

void ftPhysicsApplyGroundVelTransferAir(GObj *fighter_gobj)
{
    ftPhysicsSetGroundVelTransferAir(fighter_gobj);
}

void ftPhysicsApplyGroundFrictionOrTransN(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if ((fp != NULL) && (fp->anim_desc.flags.is_use_transn_joint))
    {
        ftPhysicsApplyGroundVelTransN(fighter_gobj);
    }
    else
    {
        ftPhysicsApplyGroundVelFriction(fighter_gobj);
    }
}

void mpCommonUpdateFighterProjectFloor(GObj *fighter_gobj)
{
    mpCommonSetFighterProjectFloor(fighter_gobj);
}

void mpCommonProcFighterWaitOrLanding(GObj *fighter_gobj)
{
    if (mpCommonCheckFighterLanding(fighter_gobj) != FALSE)
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);

        if ((fp != NULL) &&
            (fp->physics.vel_air.y > FTCOMMON_ATTACKAIR_SKIPLANDING_VEL_Y_MAX))
        {
            ftCommonWaitSetStatus(fighter_gobj);
        }
        else
        {
            ftCommonLandingSetStatus(fighter_gobj);
        }
    }
}

void mpCommonProcFighterProject(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    Vec3f *pos = NULL;

    if (fp == NULL)
    {
        return;
    }
    if ((fp != NULL) && (fp->coll_data.p_translate != NULL))
    {
        pos = fp->coll_data.p_translate;
    }
    mpCommonRunFighterCollisionDefault(fighter_gobj, pos, &fp->coll_data);
}

void mpCommonUpdateFighterKinetics(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp == NULL)
    {
        return;
    }
    if (fp->ga == nMPKineticsAir)
    {
        if (mpCommonCheckFighterLanding(fighter_gobj) != FALSE)
        {
            mpCommonSetFighterGround(fp);
        }
    }
    else if (mpCommonCheckFighterOnFloor(fighter_gobj) == FALSE)
    {
        mpCommonSetFighterAir(fp);
    }
}

void ftHammerProcMap(GObj *fighter_gobj)
{
    if (mpCommonCheckFighterOnFloor(fighter_gobj) == FALSE)
    {
        ftCommonHammerFallSetStatus(fighter_gobj);
    }
}

#endif
