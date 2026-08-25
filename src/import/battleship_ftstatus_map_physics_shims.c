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
    /* BattleShip mpcommon.c:684 exactly. This used to carry its own copy of
     * mpCommonSetFighterWaitOrLanding's body because the port had no such
     * function; P2-3f5 added it at the mpcommon seam (Falcon Dive needs it as a
     * proc_map), so the duplicate is gone. */
    mpCommonProcFighterLanding(fighter_gobj, mpCommonSetFighterWaitOrLanding);
}

sb32 mpCommonProcFighterOnEdge(GObj *fighter_gobj, void (*proc_map)(GObj *))
{
    if (mpCommonCheckFighterOnEdge(fighter_gobj) == FALSE)
    {
        proc_map(fighter_gobj);
        return FALSE;
    }
    return TRUE;
}

sb32 mpCommonProcFighterLanding(GObj *fighter_gobj, void (*proc_map)(GObj *))
{
    if (mpCommonCheckFighterLanding(fighter_gobj) != FALSE)
    {
        proc_map(fighter_gobj);
        return TRUE;
    }
    return FALSE;
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
