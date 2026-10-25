/* Source-function shims, the fighters' common statuses and physics: the DObj
 * animation helpers, status interrupts and setters (catch, attacks, guard,
 * turn, dash, run, jumps, landing, damage, thrown), the air physics and the
 * fighters' map collision. A slice of reloc_backend.c, after
 * reloc_backend_source_shims.c. */

/* Defined beside lbCommonAddFighterPartsFigatree, which owns the shared tree
 * walk and the same pointer resolve. */
void lbCommonAddDObjAnimJointAll(DObj *dobj, AObjEvent32 **anim_joint,
                                 f32 anim_frame);
extern void gcAddDObj3TransformsKind(DObj *dobj, u8 tk1, u8 tk2, u8 tk3);
extern void gcDecideDObj3TransformsKind(DObj *dobj, u8 tk1, u8 tk2,
                                         u8 tk3, s32 flags);
extern DObj *lbCommonGetTreeDObjNextFromRoot(DObj *dobj, DObj *root_dobj);

void lbCommonSetupTreeDObjs(DObj *root_dobj, DObjDesc *dobjdesc,
                            DObj **dobjs, u8 tk1, u8 tk2, u8 tk3)
{
    s32 i;
    DObj *current_dobj;
    s32 id;
    DObj *array_dobjs[DOBJ_ARRAY_MAX];

    /* BattleShip lbcommon.c:914-952, transcribed rather than approximated.
     * This used to be an empty compatibility symbol because no shipped DS
     * effect depended on its tree. P2-2 restores Fox's source Arwing entry,
     * whose maker immediately walks the hierarchy this helper constructs; a
     * no-op therefore left the source's first child NULL and faulted at the
     * first source dereference. The O2R FoxSpecial3 descriptor was audited
     * independently: 12 nodes followed by the source DOBJ_ARRAY_MAX sentinel,
     * and its child/sibling chain matches efManagerFoxEntryArwingMakeEffect.
     * Keep the source topology, transform-kind selection and output ordering
     * exactly; only the reloc loader's already-native pointers are DS-specific. */
    for (i = 0; i < DOBJ_ARRAY_MAX; i++)
    {
        array_dobjs[i] = NULL;
    }
    while (dobjdesc->id != DOBJ_ARRAY_MAX)
    {
        id = dobjdesc->id & 0xFFF;

        if (id != 0)
        {
            current_dobj = array_dobjs[id] =
                gcAddChildForDObj(array_dobjs[id - 1], dobjdesc->dl);
        }
        else
        {
            current_dobj = array_dobjs[0] =
                gcAddChildForDObj(root_dobj, dobjdesc->dl);
        }
        if (dobjdesc->id & 0xF000)
        {
            gcDecideDObj3TransformsKind(current_dobj, tk1, tk2, tk3,
                                         dobjdesc->id & 0xF000);
        }
        else
        {
            gcAddDObj3TransformsKind(current_dobj, tk1, tk2, tk3);
        }
        current_dobj->translate.vec.f = dobjdesc->translate;
        current_dobj->rotate.vec.f = dobjdesc->rotate;
        current_dobj->scale.vec.f = dobjdesc->scale;

        if (dobjs != NULL)
        {
            *dobjs++ = current_dobj;
        }
        dobjdesc++;
    }
}

void lbCommonAddMObjForTreeDObjs(DObj *root_dobj, MObjSub ***p_mobjsubs)
{
    DObj *current_dobj = root_dobj;

    /* BattleShip lbcommon.c:1171-1196. Source model effects share this helper;
     * leaving it as a no-op would build the right hierarchy but silently omit
     * every source material attached through an EFDesc. */
    while (current_dobj != NULL)
    {
        if (p_mobjsubs != NULL)
        {
            if (*p_mobjsubs != NULL)
            {
                MObjSub **mobjsubs = *p_mobjsubs;
                MObjSub *mobjsub = *mobjsubs;

                while (mobjsub != NULL)
                {
                    MObjSub normalized_mobjsub;

                    /* Effects use this lbcommon path rather than gcAddMObjAll.
                     * Restore the same mixed u16/u8/color lanes before the
                     * source constructor copies the record into a live MObj. */
                    if (ndsRelocCopyMObjSubForAttachment(
                            &normalized_mobjsub, mobjsub) < 0)
                    {
                        ndsRendererRecordNativeFailure(NDS_NATIVE_FAILURE_STAGE,
                            (u32)gSCManagerSceneData.scene_curr, 0xffffffffu,
                            mobjsub->flags, (u32)(uintptr_t)mobjsub, 0u,
                            NDS_NATIVE_FAILURE_BAD_ASSET);
                        return;
                    }
                    gcAddMObjForDObj(current_dobj, &normalized_mobjsub);
                    mobjsubs++;
                    mobjsub = *mobjsubs;
                }
            }
            p_mobjsubs++;
        }
        current_dobj =
            lbCommonGetTreeDObjNextFromRoot(current_dobj, root_dobj);
    }
}

void lbCommonAddTreeDObjsAnimAll(DObj *root_dobj,
                                 AObjEvent32 **anim_joints,
                                 AObjEvent32 ***p_matanim_joints,
                                 f32 anim_frame)
{
    DObj *current_dobj = root_dobj;

    /* BattleShip lbcommon.c:838-881. Animation and material-animation streams
     * remain source-owned; the DS adaptation is only that the reloc layer has
     * already converted their pointers before this source-shaped walk. */
    root_dobj->parent_gobj->anim_frame = anim_frame;

    while (current_dobj != NULL)
    {
        if (anim_joints != NULL)
        {
            AObjEvent32 *anim_joint = *anim_joints;

            if (anim_joint != NULL)
            {
                gcAddDObjAnimJoint(current_dobj, anim_joint, anim_frame);
            }
            else
            {
                current_dobj->anim_wait = AOBJ_ANIM_NULL;
            }
            anim_joints++;
        }
        if (p_matanim_joints != NULL)
        {
            if (*p_matanim_joints != NULL)
            {
                MObj *mobj = current_dobj->mobj;
                AObjEvent32 **matanim_joints = *p_matanim_joints;

                while (mobj != NULL)
                {
                    AObjEvent32 *matanim_joint = *matanim_joints;

                    if (matanim_joint != NULL)
                    {
                        gcAddMObjMatAnimJoint(mobj, matanim_joint, anim_frame);
                    }
                    mobj = mobj->next;
                    matanim_joints++;
                }
            }
            p_matanim_joints++;
        }
        current_dobj =
            lbCommonGetTreeDObjNextFromRoot(current_dobj, root_dobj);
    }
}

/* BattleShip lbcommon.c:1199-1208, restricted to the supplied subtree. */
NDS_COLD void lbCommonPlayTreeDObjsAnim(DObj *root_dobj)
{
    DObj *current_dobj = root_dobj;

    while (current_dobj != NULL)
    {
        gcPlayDObjAnimJoint(current_dobj);
        current_dobj = lbCommonGetTreeDObjNextFromRoot(current_dobj, root_dobj);
    }
}

NDS_COLD void lbCommonSetDObjTransformsForTreeDObjs(DObj *root_dobj,
                                           DObjDesc *dobjdesc)
{
    DObj *current_dobj = root_dobj;

    /* BattleShip lbcommon.c:1211-1225. */
    while ((current_dobj != NULL) && (dobjdesc->id != DOBJ_ARRAY_MAX))
    {
        current_dobj->translate.vec.f = dobjdesc->translate;
        current_dobj->rotate.vec.f = dobjdesc->rotate;
        current_dobj->scale.vec.f = dobjdesc->scale;
        dobjdesc++;
        current_dobj =
            lbCommonGetTreeDObjNextFromRoot(current_dobj, root_dobj);
    }
}

void ftParamResetFighterColAnim(GObj *fighter_gobj)
{
    FTStruct *fp = (fighter_gobj != NULL) ? ftGetStruct(fighter_gobj) : NULL;

    if (fp != NULL)
    {
        ftParamResetColAnim(&fp->colanim);
    }
}

void ftParamResetColAnim(GMColAnim *colanim)
{
    s32 i;

    if (colanim == NULL)
    {
        return;
    }
    for (i = 0; i < ARRAY_COUNT(colanim->cs); i++)
    {
        colanim->cs[i].p_script = NULL;
        colanim->cs[i].color_event_timer = 0;
        colanim->cs[i].script_id = 0;
    }
    colanim->length = 0;
    colanim->colanim_id = 0;
    colanim->is_use_color1 = 0;
    colanim->is_use_color2 = 0;
    colanim->is_use_light = 0;
    colanim->skeleton_id = 0;
}

void ftParamResetStatUpdateColAnim(GObj *fighter_gobj)
{
    enum
    {
        /* Source-private constants: ftdonkey.h / ftsamus.h. Keep them local
         * instead of importing character-private headers into this compat TU. */
        NDS_COMPAT_DONKEY_CHARGE_MAX = 10,
        NDS_COMPAT_SAMUS_CHARGE_MAX = 7
    };
    FTStruct *fp = (fighter_gobj != NULL) ? ftGetStruct(fighter_gobj) : NULL;

    if (fp == NULL)
    {
        return;
    }

    /* BattleShip ftparam.c:1247-1323. This was once intentionally trimmed to
     * Mario/Fox when those were the only live fighters, but P2 now admits
     * Donkey and Samus. Their full-charge indicator is restored *here* after a
     * higher-priority unlocked ColAnim ends; the direct charge-max request can
     * legitimately lose arbitration while the 60-priority charging flash is
     * still active. Keep the source reset/reapply contract as the roster grows. */
    ftParamResetFighterColAnim(fighter_gobj);

    switch (ftParamGetBestHitStatusPart(fighter_gobj))
    {
    case nGMHitStatusInvincible:
        ftParamCheckSetFighterColAnimID(
            fighter_gobj, nGMColAnimFighterHitStatusInvincible, 0);
        break;
    case nGMHitStatusIntangible:
        ftParamCheckSetFighterColAnimID(
            fighter_gobj, nGMColAnimFighterHitStatusIntangible, 0);
        break;
    }
    /* Remix's gfx_routine_end table: a content's own routine replaces its
     * parent's case (Dedede's and Sheik's full charge). */
    if (ndsP4ColAnimEnd(fighter_gobj) == FALSE)
    switch (fp->fkind)
    {
    case nFTKindDonkey:
    case nFTKindNDonkey:
    case nFTKindGDonkey:
        if (fp->passive_vars.donkey.charge_level ==
            NDS_COMPAT_DONKEY_CHARGE_MAX)
        {
            ftParamCheckSetFighterColAnimID(
                fighter_gobj, nGMColAnimFighterCommonSpecialNCharge, 0);
        }
        break;
    case nFTKindSamus:
    case nFTKindNSamus:
        if (fp->passive_vars.samus.charge_level ==
            NDS_COMPAT_SAMUS_CHARGE_MAX)
        {
            ftParamCheckSetFighterColAnimID(
                fighter_gobj, nGMColAnimFighterCommonSpecialNCharge, 0);
        }
        break;
    /* ftparam.c:1285-1299: a Kirby holding a full copied charge. */
    case nFTKindKirby:
    case nFTKindNKirby:
        if ((fp->passive_vars.kirby.copy_id == nFTKindSamus) ||
            (fp->passive_vars.kirby.copy_id == nFTKindNSamus))
        {
            if (fp->passive_vars.kirby.copysamus_charge_level ==
                NDS_COMPAT_SAMUS_CHARGE_MAX)
            {
                ftParamCheckSetFighterColAnimID(
                    fighter_gobj, nGMColAnimFighterCommonSpecialNCharge, 0);
            }
        }
        if ((fp->passive_vars.kirby.copy_id == nFTKindDonkey) ||
            (fp->passive_vars.kirby.copy_id == nFTKindNDonkey) ||
            (fp->passive_vars.kirby.copy_id == nFTKindGDonkey))
        {
            if (fp->passive_vars.kirby.copydonkey_charge_level ==
                NDS_COMPAT_DONKEY_CHARGE_MAX)
            {
                ftParamCheckSetFighterColAnimID(
                    fighter_gobj, nGMColAnimFighterCommonSpecialNCharge, 0);
            }
        }
        break;
    /* ftparam.c:1300-1305: PSI Magnet's hold glow. */
    case nFTKindNess:
    case nFTKindNNess:
        if (fp->is_absorb)
        {
            ftParamCheckSetFighterColAnimID(
                fighter_gobj, nGMColAnimFighterNessSpecialLwHold, 0);
        }
        break;
    }
    if (fp->damage_heal != 0)
    {
        ftParamCheckSetFighterColAnimID(fighter_gobj,
                                        nGMColAnimFighterHeal, 0);
    }
    if (fp->star_invincible_tics != 0)
    {
        ftParamCheckSetFighterColAnimID(fighter_gobj,
                                        nGMColAnimFighterStar, 0);
    }
    if ((fp->invincible_tics != 0) || (fp->intangible_tics != 0))
    {
        ftParamCheckSetFighterColAnimID(fighter_gobj,
                                        nGMColAnimFighterNoDamage, 0);
    }
    if ((fp->status_id >= nFTCommonStatusHammerStart) &&
        (fp->status_id <= nFTCommonStatusHammerEnd))
    {
        ftParamCheckSetFighterColAnimID(fighter_gobj,
                                        nGMColAnimFighterHammer, 0);
    }
}

/* The real ones come with battleship_ftcommon_hammer.c, which imports
 * fthammer.c whole. These no-op/counter shims are why picking the Hammer up
 * started its timer and music but the fighter never entered any Hammer
 * state. */

sb32 ftCommonGroundCheckInterrupt(GObj *fighter_gobj)
{
    /* BattleShip fighter.h defines this exact ordered chain as the common
     * ground interrupt. The imported manager runs it for every live fighter;
     * it must not depend on the old scripted natural-motion proof being active. */
    return ((ftCommonSpecialNCheckInterruptCommon(fighter_gobj) != FALSE) ||
            (ftCommonSpecialHiCheckInterruptCommon(fighter_gobj) != FALSE) ||
            (ftCommonSpecialLwCheckInterruptCommon(fighter_gobj) != FALSE) ||
            (ftCommonCatchCheckInterruptCommon(fighter_gobj) != FALSE) ||
            (ftCommonAttackS4CheckInterruptCommon(fighter_gobj) != FALSE) ||
            (ftCommonAttackHi4CheckInterruptCommon(fighter_gobj) != FALSE) ||
            (ftCommonAttackLw4CheckInterruptCommon(fighter_gobj) != FALSE) ||
            (ftCommonAttackS3CheckInterruptCommon(fighter_gobj) != FALSE) ||
            (ftCommonAttackHi3CheckInterruptCommon(fighter_gobj) != FALSE) ||
            (ftCommonAttackLw3CheckInterruptCommon(fighter_gobj) != FALSE) ||
            (ftCommonAttack1CheckInterruptCommon(fighter_gobj) != FALSE) ||
            (ftCommonGuardOnCheckInterruptCommon(fighter_gobj) != FALSE) ||
            (ftCommonAppealCheckInterruptCommon(fighter_gobj) != FALSE) ||
            (ftCommonKneeBendCheckInterruptCommon(fighter_gobj) != FALSE) ||
            (ftCommonDashCheckInterruptCommon(fighter_gobj) != FALSE) ||
            (ftCommonPassCheckInterruptCommon(fighter_gobj) != FALSE) ||
            (ftCommonDokanStartCheckInterruptCommon(fighter_gobj) != FALSE) ||
            (ftCommonSquatCheckInterruptCommon(fighter_gobj) != FALSE) ||
            (ftCommonTurnCheckInterruptCommon(fighter_gobj) != FALSE) ||
            (ftCommonWalkCheckInterruptCommon(fighter_gobj) != FALSE)) ? TRUE :
                                                                         FALSE;
    return FALSE;
}

static sb32 ndsFighterWalkDeferredInterrupt(GObj *fighter_gobj)
{
    (void)fighter_gobj;
    return FALSE;
}

sb32 ftCommonSpecialHiCheckInterruptCommon(GObj *fighter_gobj)
{
    {
        extern sb32 ndsBaseFTCommonSpecialHiCheckInterruptCommon(
            GObj *fighter_gobj);

        return ndsBaseFTCommonSpecialHiCheckInterruptCommon(fighter_gobj);
    }
}

sb32 ftCommonCatchCheckInterruptCommon(GObj *fighter_gobj)
{
    return ndsBaseFTCommonCatchCheckInterruptCommon(fighter_gobj);
}

sb32 ftCommonAttackS4CheckInterruptCommon(GObj *fighter_gobj)
{
    return ndsBaseFTCommonAttackS4CheckInterruptCommon(fighter_gobj);
}

sb32 ftCommonAttackS4CheckInterruptTurn(GObj *fighter_gobj)
{
    return ndsBaseFTCommonAttackS4CheckInterruptTurn(fighter_gobj);
}

sb32 ftCommonAttackHi4CheckInterruptCommon(GObj *fighter_gobj)
{
    return ndsBaseFTCommonAttackHi4CheckInterruptCommon(fighter_gobj);
}

sb32 ftCommonAttackLw4CheckInterruptCommon(GObj *fighter_gobj)
{
    return ndsBaseFTCommonAttackLw4CheckInterruptCommon(fighter_gobj);
}

sb32 ftCommonAttackLw4CheckInterruptSquat(GObj *fighter_gobj)
{
    return ndsBaseFTCommonAttackLw4CheckInterruptSquat(fighter_gobj);
}

sb32 ftCommonAttackS3CheckInterruptCommon(GObj *fighter_gobj)
{
    return ndsBaseFTCommonAttackS3CheckInterruptCommon(fighter_gobj);
}

sb32 ftCommonAttackHi3CheckInterruptCommon(GObj *fighter_gobj)
{
    return ndsBaseFTCommonAttackHi3CheckInterruptCommon(fighter_gobj);
}

sb32 ftCommonAttackLw3CheckInterruptCommon(GObj *fighter_gobj)
{
    return ndsBaseFTCommonAttackLw3CheckInterruptCommon(fighter_gobj);
}

sb32 ftCommonAttack1CheckInterruptCommon(GObj *fighter_gobj)
{
    return ndsBaseFTCommonAttack1CheckInterruptCommon(fighter_gobj);
}

void ftCommonAttack11SetStatus(GObj *fighter_gobj)
{
    ndsBaseFTCommonAttack11SetStatus(fighter_gobj);
    return;
}

void ftCommonAttack11ProcUpdate(GObj *fighter_gobj)
{
    ndsBaseFTCommonAttack11ProcUpdate(fighter_gobj);
    return;
}

void ftCommonAttack11ProcInterrupt(GObj *fighter_gobj)
{
    ndsBaseFTCommonAttack11ProcInterrupt(fighter_gobj);
    return;
}

void ftCommonAttack12SetStatus(GObj *fighter_gobj)
{
    ndsBaseFTCommonAttack12SetStatus(fighter_gobj);
    return;
}

void ftCommonAttack12ProcUpdate(GObj *fighter_gobj)
{
    ndsBaseFTCommonAttack12ProcUpdate(fighter_gobj);
    return;
}

void ftCommonAttack12ProcInterrupt(GObj *fighter_gobj)
{
    ndsBaseFTCommonAttack12ProcInterrupt(fighter_gobj);
    return;
}

sb32 ftCommonAttack11CheckGoto(GObj *fighter_gobj)
{
    return ndsBaseFTCommonAttack11CheckGoto(fighter_gobj);
}

/* The real ones come with battleship_ftcommon_itemuse.c, which imports
 * ftcommonitemshoot.c whole. These no-ops are why a fighter holding a Ray Gun
 * or a Fire Flower pressed A and nothing came out. */

/* The real one lives in battleship_ftcommon_get.c wherever the item core is on.
 * This shim answered "no item here" unconditionally, which was correct while
 * nothing could be picked up and is a gameplay hole now that something can. */

sb32 ftCommonAttack100StartCheckInterruptCommon(GObj *fighter_gobj)
{
    return ndsBaseFTCommonAttack100StartCheckInterruptCommon(fighter_gobj);
}

NDS_COLD void ftCommonAttack100StartSetStatus(GObj *fighter_gobj)
{
    ndsBaseFTCommonAttack100StartSetStatus(fighter_gobj);
    return;
}

void ftCommonAttack100StartProcUpdate(GObj *fighter_gobj)
{
    ndsBaseFTCommonAttack100StartProcUpdate(fighter_gobj);
    return;
}

void ftCommonAttack100LoopSetStatus(GObj *fighter_gobj)
{
    ndsBaseFTCommonAttack100LoopSetStatus(fighter_gobj);
    return;
}

void ftCommonAttack100LoopProcUpdate(GObj *fighter_gobj)
{
    ndsBaseFTCommonAttack100LoopProcUpdate(fighter_gobj);
    return;
}

void ftCommonAttack100LoopProcInterrupt(GObj *fighter_gobj)
{
    ndsBaseFTCommonAttack100LoopProcInterrupt(fighter_gobj);
    return;
}

void ftCommonAttack100EndSetStatus(GObj *fighter_gobj)
{
    ndsBaseFTCommonAttack100EndSetStatus(fighter_gobj);
    return;
}

sb32 ftCommonCatchCheckInterruptAttack11(GObj *fighter_gobj)
{
    return ndsBaseFTCommonCatchCheckInterruptAttack11(fighter_gobj);
}

u16 ftParamGetMotionCount(void);
u16 ftParamGetStatUpdateCount(void);

void ftParamSetMotionID(FTStruct *fp, s32 motion_attack_id)
{
    if (fp != NULL)
    {
        /* BattleShip ftparam.c:1632-1636. `motion_count` distinguishes two
         * separate uses of the same attack for stale-move accounting; leaving
         * it unchanged makes the four-player stale queue collapse distinct
         * attacks into one old motion. */
        fp->motion_attack_id = motion_attack_id;
        fp->motion_count = ftParamGetMotionCount();
    }
}

void ftParamSetStatUpdate(FTStruct *fp, u16 flags)
{
    if (fp != NULL)
    {
        fp->stat_flags.halfword = flags;
        /* Source ftparam.c:1680-1684 assigns a fresh stat generation here.
         * Keep the DS shadow fields below as mirrors for legacy probes/custom
         * status helpers, but do not substitute them for the source counter. */
        fp->stat_count = ftParamGetStatUpdateCount();
        fp->stat_attack_id = fp->stat_flags.attack_id;
        fp->status_attack_id = fp->stat_flags.attack_id;
        fp->status_is_smash = fp->stat_flags.is_smash_attack;
        fp->status_is_projectile = fp->stat_flags.is_projectile;
    }
}

void ftParamUpdate1PGameAttackStats(FTStruct *fp, u16 flags)
{
    /* BattleShip ftparam.c:1687-1704 is statistics-only and, critically for
     * P2-2, is a complete no-op outside 1P Game.  The old shim rewrote
     * stat_flags to `flags` (normally zero) and incremented stat_count in VS,
     * changing attack identity immediately after ftParamSetStatUpdate had set
     * it.  P2-6 owns the missing 1P bonus counters; do not mutate gameplay
     * state here in VS as a substitute for those counters. */
    (void)fp;
    (void)flags;
}

__attribute__((weak)) GObj *
efManagerKirbyVulcanJabMakeEffect(Vec3f *pos, s32 lr, f32 rotate, f32 vel,
                                  f32 add)
{
    (void)pos;
    (void)lr;
    (void)rotate;
    (void)vel;
    (void)add;
    return NULL;
}

__attribute__((weak)) GObj *
efManagerSamusGrappleBeamGlowMakeEffect(GObj *fighter_gobj)
{
    (void)fighter_gobj;
    return NULL;
}

sb32 ftCommonGuardOnCheckInterruptCommon(GObj *fighter_gobj)
{
    return ndsBaseFTCommonGuardOnCheckInterruptCommon(fighter_gobj);
}


sb32 ftCommonEscapeCheckInterruptGuard(GObj *fighter_gobj)
{
    return ndsBaseFTCommonEscapeCheckInterruptGuard(fighter_gobj);
}


sb32 ftCommonGuardCheckInterruptEscape(GObj *fighter_gobj)
{
    return ndsBaseFTCommonGuardCheckInterruptEscape(fighter_gobj);
}

sb32 ftCommonCatchCheckInterruptGuard(GObj *fighter_gobj)
{
    return ndsBaseFTCommonCatchCheckInterruptGuard(fighter_gobj);
}

sb32 ftCommonGuardPassCheckInterruptGuard(GObj *fighter_gobj)
{
    return ndsBaseFTCommonGuardPassCheckInterruptGuard(fighter_gobj);
}

sb32 ftCommonAppealCheckInterruptCommon(GObj *fighter_gobj)
{
    return ndsBaseFTCommonAppealCheckInterruptCommon(fighter_gobj);
}

void ftKirbySpecialNLoseCopy(GObj *fighter_gobj);
void ftKirbySpecialNDamageCheckLoseCopy(GObj *fighter_gobj);

sb32 ftCommonKneeBendCheckInterruptCommon(GObj *fighter_gobj)
{
    return ndsBaseFTCommonKneeBendCheckInterruptCommon(fighter_gobj);
}

sb32 ftCommonDashCheckInterruptCommon(GObj *fighter_gobj)
{
    return ndsBaseFTCommonDashCheckInterruptCommon(fighter_gobj);
}

sb32 ftCommonAttackS4CheckInterruptDash(GObj *fighter_gobj)
{
    return ndsBaseFTCommonAttackS4CheckInterruptDash(fighter_gobj);
}

sb32 ftCommonEscapeCheckInterruptDash(GObj *fighter_gobj)
{
    return ndsBaseFTCommonEscapeCheckInterruptDash(fighter_gobj);
}

sb32 ftCommonCatchCheckInterruptDashRun(GObj *fighter_gobj)
{
    return ndsBaseFTCommonCatchCheckInterruptDashRun(fighter_gobj);
}

void ftCommonAttackDashSetStatus(GObj *fighter_gobj)
{
    ndsBaseFTCommonAttackDashSetStatus(fighter_gobj);
    return;
}

sb32 ftCommonAttackDashCheckInterruptCommon(GObj *fighter_gobj)
{
    return ndsBaseFTCommonAttackDashCheckInterruptCommon(fighter_gobj);
}

/* The real one comes with battleship_ftcommon_itemthrow.c, which is now gated
 * on the item core as well as Link. */

/* Same, for the Star Rod and Harisen swing (ftcommonitemswing.c:114). */

sb32 ftCommonGuardOnCheckInterruptDashRun(GObj *fighter_gobj, f32 frame)
{
    return ndsBaseFTCommonGuardOnCheckInterruptDashRun(fighter_gobj,
                                                       (s32)frame);

    return ndsFighterWalkDeferredInterrupt(fighter_gobj);
}

sb32 ftCommonKneeBendCheckInterruptRun(GObj *fighter_gobj)
{
    return ndsBaseFTCommonKneeBendCheckInterruptRun(fighter_gobj);
}

NDS_COLD sb32 ftCommonTurnRunCheckInterruptRun(GObj *fighter_gobj)
{
    return ndsBaseFTCommonTurnRunCheckInterruptRun(fighter_gobj);
}

void ftCommonTurnProcUpdate(GObj *fighter_gobj)
{
    ndsBaseFTCommonTurnProcUpdate(fighter_gobj);
    return;
}

void ftCommonTurnProcInterrupt(GObj *fighter_gobj)
{
    ndsBaseFTCommonTurnProcInterrupt(fighter_gobj);
    return;
}

void ftCommonTurnSetStatus(GObj *fighter_gobj, s32 lr_dash)
{
    ndsBaseFTCommonTurnSetStatus(fighter_gobj, lr_dash);
    return;
}

void ftCommonTurnSetStatusCenter(GObj *fighter_gobj)
{
    ndsBaseFTCommonTurnSetStatusCenter(fighter_gobj);
    return;
}

void ftCommonTurnSetStatusInvertLR(GObj *fighter_gobj)
{
    ndsBaseFTCommonTurnSetStatusInvertLR(fighter_gobj);
    return;
}

sb32 ftCommonTurnCheckInputSuccess(GObj *fighter_gobj)
{
    return ndsBaseFTCommonTurnCheckInputSuccess(fighter_gobj);
}

void ftParamSetStickLR(FTStruct *fp)
{
    if (fp != NULL)
    {
        /* BattleShip ftparam.c:251-254 deliberately treats centered X as
         * facing right. Several common attack/special entry paths call this
         * helper directly, so suppressing the zero case was a DS-only state
         * rule that could preserve an old left-facing direction. */
        fp->lr = (fp->input.pl.stick_range.x >= 0) ? 1 : -1;
    }
}

sb32 ftCommonSquatCheckInterruptCommon(GObj *fighter_gobj)
{
    return ndsBaseFTCommonSquatCheckInterruptCommon(fighter_gobj);
}

sb32 ftCommonPassCheckInterruptCommon(GObj *fighter_gobj)
{
    return ndsBaseFTCommonPassCheckInterruptCommon(fighter_gobj);
}

sb32 ftCommonPassCheckInterruptSquat(GObj *fighter_gobj)
{
    return ndsBaseFTCommonPassCheckInterruptSquat(fighter_gobj);
}

/* Mushroom Kingdom warp-pipe enter path: the source test lives in
 * src/import/battleship_ftcommon_dokan.c, which imports decomp
 * ftcommondokan.c verbatim (ftcommon.h:133-141 constants included). This TU
 * keeps only the proof-gated wrapper: the DownWaitLoop probe below defers the
 * interrupt while it runs, and non-pipe fighters still fall through to the
 * deferred interrupt. */
sb32 ndsBaseFTCommonDokanStartCheckInterruptCommon(GObj *fighter_gobj);

sb32 ftCommonDokanStartCheckInterruptCommon(GObj *fighter_gobj)
{

    /* decomp ftcommondokan.c:112-159; the fallthrough stays the deferred
     * interrupt so non-pipe fighters behave as before. */
    if (ndsBaseFTCommonDokanStartCheckInterruptCommon(fighter_gobj) != FALSE)
    {
        return TRUE;
    }
    return ndsFighterWalkDeferredInterrupt(fighter_gobj);
}

sb32 ftCommonSquatWaitCheckInterruptLanding(GObj *fighter_gobj)
{
    return ndsBaseFTCommonSquatWaitCheckInterruptLanding(fighter_gobj);
}

/* Owned by battleship_ftcommon_hammer.c wherever the item core is on -- that
 * TU imports the whole of ftcommonhammerfall.c, which is where this check
 * lives. */

sb32 ftCommonTurnCheckInterruptCommon(GObj *fighter_gobj)
{
    return ndsBaseFTCommonTurnCheckInterruptCommon(fighter_gobj);
}

void ftAnimEndSetWait(GObj *fighter_gobj)
{
    battleship_ftAnimEndSetWait(fighter_gobj);
    return;
}

void ftAnimEndSetFall(GObj *fighter_gobj)
{
    battleship_ftAnimEndSetFall(fighter_gobj);
    return;
}

sb32 ftAnimEndCheckSetStatus(GObj *fighter_gobj, void (*proc_status)(GObj*))
{
    return battleship_ftAnimEndCheckSetStatus(fighter_gobj, proc_status);
}

void ftCommonDashProcUpdate(GObj *fighter_gobj)
{
    ndsBaseFTCommonDashProcUpdate(fighter_gobj);
    return;
}

void ftCommonDashProcInterrupt(GObj *fighter_gobj)
{
    ndsBaseFTCommonDashProcInterrupt(fighter_gobj);
    return;
}

void ftCommonDashProcPhysics(GObj *fighter_gobj)
{
    ndsBaseFTCommonDashProcPhysics(fighter_gobj);
    return;
}

void ftCommonDashProcMap(GObj *fighter_gobj)
{
    ndsBaseFTCommonDashProcMap(fighter_gobj);
    return;
}

void ftCommonDashSetStatus(GObj *fighter_gobj, u32 flag)
{
    ndsBaseFTCommonDashSetStatus(fighter_gobj, flag);
    return;
}

sb32 ftCommonDashCheckTurn(GObj *fighter_gobj)
{
    return ndsBaseFTCommonDashCheckTurn(fighter_gobj);
}

void ftCommonRunProcInterrupt(GObj *fighter_gobj)
{
    ndsBaseFTCommonRunProcInterrupt(fighter_gobj);
    return;
}

void ftCommonRunSetStatus(GObj *fighter_gobj)
{
    ndsBaseFTCommonRunSetStatus(fighter_gobj);
    return;
}

sb32 ftCommonRunCheckInterruptDash(GObj *fighter_gobj)
{
    return ndsBaseFTCommonRunCheckInterruptDash(fighter_gobj);
}

void ftCommonRunBrakeProcInterrupt(GObj *fighter_gobj)
{
    ndsBaseFTCommonRunBrakeProcInterrupt(fighter_gobj);
    return;
}

void ftCommonRunBrakeProcPhysics(GObj *fighter_gobj)
{
    ndsBaseFTCommonRunBrakeProcPhysics(fighter_gobj);
    return;
}

void ftCommonRunBrakeSetStatus(GObj *fighter_gobj, u32 flag)
{
    ndsBaseFTCommonRunBrakeSetStatus(fighter_gobj, flag);
    return;
}

NDS_COLD sb32 ftCommonRunBrakeCheckInterruptRun(GObj *fighter_gobj)
{
    return ndsBaseFTCommonRunBrakeCheckInterruptRun(fighter_gobj);
}

sb32 ftCommonRunBrakeCheckInterruptTurnRun(GObj *fighter_gobj)
{
    return ndsBaseFTCommonRunBrakeCheckInterruptTurnRun(fighter_gobj);
}

void ftCommonKneeBendProcUpdate(GObj *fighter_gobj)
{
    ndsBaseFTCommonKneeBendProcUpdate(fighter_gobj);
    return;
}

void ftCommonKneeBendProcInterrupt(GObj *fighter_gobj)
{
    ndsBaseFTCommonKneeBendProcInterrupt(fighter_gobj);
    return;
}

void ftCommonKneeBendSetStatusParam(GObj *fighter_gobj, s32 status_id,
                                    s32 input_source)
{
    ndsBaseFTCommonKneeBendSetStatusParam(fighter_gobj, status_id, input_source);
    return;
}

void ftCommonKneeBendSetStatus(GObj *fighter_gobj, s32 input_source)
{
    ndsBaseFTCommonKneeBendSetStatus(fighter_gobj, input_source);
    return;
}

void ftCommonGuardKneeBendSetStatus(GObj *fighter_gobj, s32 input_source)
{
    ndsBaseFTCommonGuardKneeBendSetStatus(fighter_gobj, input_source);
    return;
}

sb32 ftCommonKneeBendCheckButtonTap(FTStruct *fp)
{
    return ndsBaseFTCommonKneeBendCheckButtonTap(fp);
}

s32 ftCommonKneeBendGetInputTypeCommon(FTStruct *fp)
{
    return ndsBaseFTCommonKneeBendGetInputTypeCommon(fp);
}

s32 ftCommonKneeBendGetInputTypeRun(FTStruct *fp)
{
    return ndsBaseFTCommonKneeBendGetInputTypeRun(fp);
}

sb32 ftCommonGuardKneeBendCheckInterruptGuard(GObj *fighter_gobj)
{
    return ndsBaseFTCommonGuardKneeBendCheckInterruptGuard(fighter_gobj);
}

sb32 ftCommonAttackHi4CheckInterruptKneeBend(GObj *fighter_gobj)
{
    return ndsBaseFTCommonAttackHi4CheckInterruptKneeBend(fighter_gobj);
}

/* Owned by battleship_ftcommon_hammer.c wherever the item core is on -- that
 * TU imports the whole of ftcommonhammerkneebend.c. */

void ftCommonJumpProcInterrupt(GObj *fighter_gobj)
{
    ndsBaseFTCommonJumpProcInterrupt(fighter_gobj);
    return;
}

NDS_COLD void ftCommonJumpGetJumpForceButton(s32 stick_range_x, s32 *jump_vel_x,
                                    s32 *jump_vel_y, sb32 is_shorthop)
{
    ndsBaseFTCommonJumpGetJumpForceButton(stick_range_x, jump_vel_x, jump_vel_y, is_shorthop);
    return;
}

void ftCommonJumpSetStatus(GObj *fighter_gobj)
{
    ndsBaseFTCommonJumpSetStatus(fighter_gobj);
    return;
}

void ftCommonFallProcInterrupt(GObj *fighter_gobj)
{
    ndsBaseFTCommonFallProcInterrupt(fighter_gobj);
    return;
}

void ftCommonFallSetStatus(GObj *fighter_gobj)
{
    ndsBaseFTCommonFallSetStatus(fighter_gobj);
    return;
}

void ftCommonOttottoProcUpdate(GObj *fighter_gobj)
{
    ndsBaseFTCommonOttottoProcUpdate(fighter_gobj);
    return;
}

void ftCommonOttottoProcInterrupt(GObj *fighter_gobj)
{
    ndsBaseFTCommonOttottoProcInterrupt(fighter_gobj);
    return;
}

void ftCommonOttottoProcMap(GObj *fighter_gobj)
{
    ndsBaseFTCommonOttottoProcMap(fighter_gobj);
    return;
}

void ftCommonOttottoWaitSetStatus(GObj *fighter_gobj)
{
    ndsBaseFTCommonOttottoWaitSetStatus(fighter_gobj);
    return;
}

void ftCommonOttottoSetStatus(GObj *fighter_gobj)
{
    ndsBaseFTCommonOttottoSetStatus(fighter_gobj);
    return;
}

/* Owned by battleship_ftcommon_hammer.c wherever the item core is on -- that
 * TU imports the whole of ftcommonhammerfall.c, which is where both of these
 * live. */

void ftCommonLandingProcInterrupt(GObj *fighter_gobj)
{
    ndsBaseFTCommonLandingProcInterrupt(fighter_gobj);
    return;
}

void ftCommonLandingSetStatusParam(GObj *fighter_gobj, s32 status_id,
                                   sb32 is_allow_interrupt,
                                   f32 anim_speed)
{
    ndsBaseFTCommonLandingSetStatusParam(fighter_gobj, status_id, is_allow_interrupt, anim_speed);
    return;
}

void ftCommonLandingSetStatus(GObj *fighter_gobj)
{
    ndsBaseFTCommonLandingSetStatus(fighter_gobj);
    return;
}

void ftCommonLandingAirNullSetStatus(GObj *fighter_gobj, f32 anim_speed)
{
    ndsBaseFTCommonLandingAirNullSetStatus(fighter_gobj, anim_speed);
    return;
}

void ftCommonLandingAirSetStatus(GObj *fighter_gobj)
{
    ndsBaseFTCommonLandingAirSetStatus(fighter_gobj);
    return;
}

void ftCommonLandingFallSpecialSetStatus(GObj *fighter_gobj,
                                         sb32 is_allow_interrupt,
                                         f32 anim_speed)
{
    ndsBaseFTCommonLandingFallSpecialSetStatus(fighter_gobj, is_allow_interrupt, anim_speed);
    return;
}

sb32 ftCommonAttackAirCheckInterruptCommon(GObj *fighter_gobj)
{
    sb32 result = ndsBaseFTCommonAttackAirCheckInterruptCommon(fighter_gobj);
    if ((gNdsFighterNaturalMovesetPhase == 9u) ||
        (gNdsFighterNaturalMovesetPhase == 10u))
    {
        if (result != FALSE)
        {
            gNdsFighterNaturalMovesetAerialFrames++;
        }
    }
    return result;
}

void ftCommonAttackAirProcMap(GObj *fighter_gobj)
{
    ndsBaseFTCommonAttackAirProcMap(fighter_gobj);
    return;
}

sb32 ftCommonJumpAerialCheckInterruptCommon(GObj *fighter_gobj)
{
    return ndsBaseFTCommonJumpAerialCheckInterruptCommon(fighter_gobj);
}


void mpCommonSetFighterAir(FTStruct *fp)
{
    if (fp != NULL)
    {
        fp->ga = nMPKineticsAir;
        fp->physics.vel_air.z = 0.0F;
        if (fp->joints[nFTPartsJointTopN] != NULL)
        {
            fp->joints[nFTPartsJointTopN]->translate.vec.f.z = 0.0F;
        }
        fp->jumps_used = 1;
    }
}

void mpCommonSetFighterProjectFloor(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    DObj *root = DObjGetStruct(fighter_gobj);

    if ((fp == NULL) || (root == NULL))
    {
        return;
    }
    fp->coll_data.p_translate = &root->translate.vec.f;
    fp->coll_data.p_lr = &fp->lr;
    if (fp->coll_data.p_map_coll == NULL)
    {
        fp->coll_data.p_map_coll = &fp->coll_data.map_coll;
    }
    mpProcessSetCollProjectFloorID(&fp->coll_data);
}

void ftPhysicsApplyGravityClampTVel(FTStruct *fp, f32 gravity, f32 tvel)
{
    if (fp == NULL)
    {
        return;
    }
    /* Remix's head of this function (PeachFloat.handle_physics_). */
    if (ndsP4Gravity(fp) != FALSE)
    {
        return;
    }
    fp->physics.vel_air.y -= gravity;
    if (fp->physics.vel_air.y < -tvel)
    {
        fp->physics.vel_air.y = -tvel;
    }
}

void ftPhysicsApplyGravityDefault(FTStruct *fp, FTAttributes *attr)
{
    if ((fp == NULL) || (attr == NULL))
    {
        return;
    }
    ftPhysicsApplyGravityClampTVel(fp, attr->gravity, attr->tvel_base);
}

void ftPhysicsApplyFastFall(FTStruct *fp, FTAttributes *attr)
{
    if ((fp == NULL) || (attr == NULL))
    {
        return;
    }
    fp->physics.vel_air.y = -attr->tvel_fast;
}

void ftPhysicsClampGroundVel(FTStruct *fp, f32 clamp)
{
    if (fp == NULL)
    {
        return;
    }
    if (fp->physics.vel_ground.x < -clamp)
    {
        fp->physics.vel_ground.x = -clamp;
    }
    else if (fp->physics.vel_ground.x > clamp)
    {
        fp->physics.vel_ground.x = clamp;
    }
}

void ftPhysicsApplyClampGroundVelStickRange(FTStruct *fp, s32 stick_x_min,
                                            f32 vel, f32 clamp)
{
    if (fp == NULL)
    {
        return;
    }
    if (ABS(fp->input.pl.stick_range.x) >= stick_x_min)
    {
        fp->physics.vel_ground.x +=
            (fp->input.pl.stick_range.x * vel * fp->lr);
        ftPhysicsClampGroundVel(fp, clamp);
    }
}

void ftPhysicsClampAirVelX(FTStruct *fp, f32 clamp)
{
    if (fp == NULL)
    {
        return;
    }
    if (fp->physics.vel_air.x < -clamp)
    {
        fp->physics.vel_air.x = -clamp;
    }
    else if (fp->physics.vel_air.x > clamp)
    {
        fp->physics.vel_air.x = clamp;
    }
}

NDS_COLD void ftPhysicsClampAirVelY(FTStruct *fp, f32 clamp)
{
    if (fp == NULL)
    {
        return;
    }
    if (fp->physics.vel_air.y > clamp)
    {
        fp->physics.vel_air.y = clamp;
    }
}

void ftPhysicsAddClampAirVelY(FTStruct *fp, f32 vel, f32 clamp)
{
    if (fp == NULL)
    {
        return;
    }
    fp->physics.vel_air.y += vel;
    ftPhysicsClampAirVelY(fp, clamp);
}

void ftPhysicsClampAirVelXMax(FTStruct *fp)
{
    if ((fp != NULL) && (fp->attr != NULL))
    {
        ftPhysicsClampAirVelX(fp, fp->attr->air_speed_max_x);
    }
}

void ftParamMakeRumble(FTStruct *fp, s32 rumble_id, s32 length)
{
    (void)fp;
}

sb32 ftPhysicsCheckClampAirVelXDec(FTStruct *fp, f32 clamp)
{
    if (fp == NULL)
    {
        return FALSE;
    }
    if (ABSF(fp->physics.vel_air.x) > clamp)
    {
        fp->physics.vel_air.x +=
            (fp->physics.vel_air.x >= 0.0F) ? -1.0F : 1.0F;
        if (ABSF(fp->physics.vel_air.x) < clamp)
        {
            fp->physics.vel_air.x =
                (fp->physics.vel_air.x >= 0.0F) ? clamp : -clamp;
        }
        return TRUE;
    }
    return FALSE;
}

sb32 ftPhysicsCheckClampAirVelXDecMax(FTStruct *fp, FTAttributes *attr)
{
    if ((fp == NULL) || (attr == NULL))
    {
        return FALSE;
    }
    return ftPhysicsCheckClampAirVelXDec(fp, attr->air_speed_max_x);
}

void ftPhysicsClampAirVelXStickRange(FTStruct *fp, s32 stick_range_min,
                                     f32 vel, f32 clamp)
{
    if (fp == NULL)
    {
        return;
    }
    if (ABS(fp->input.pl.stick_range.x) >= stick_range_min)
    {
        fp->physics.vel_air.x += fp->input.pl.stick_range.x * vel;
        ftPhysicsClampAirVelX(fp, clamp);
    }
}

void ftPhysicsClampAirVelXStickDefault(FTStruct *fp, FTAttributes *attr)
{
    if (attr != NULL)
    {
        ftPhysicsClampAirVelXStickRange(fp,
                                        FTPHYSICS_AIRDRIFT_CLAMP_RANGE_MIN,
                                        attr->air_accel,
                                        attr->air_speed_max_x);
    }
}

void ftPhysicsApplyAirVelXFriction(FTStruct *fp, FTAttributes *attr)
{
    if ((fp == NULL) || (attr == NULL))
    {
        return;
    }
    if (fp->physics.vel_air.x < 0.0F)
    {
        fp->physics.vel_air.x += attr->air_friction;
        if (fp->physics.vel_air.x > 0.0F)
        {
            fp->physics.vel_air.x = 0.0F;
        }
    }
    else if (fp->physics.vel_air.x > 0.0F)
    {
        fp->physics.vel_air.x -= attr->air_friction;
        if (fp->physics.vel_air.x < 0.0F)
        {
            fp->physics.vel_air.x = 0.0F;
        }
    }
}

void ftPhysicsCheckSetFastFall(FTStruct *fp)
{
    if ((fp != NULL) &&
        (fp->is_fastfall == FALSE) &&
        (fp->physics.vel_air.y < 0.0F) &&
        (fp->input.pl.stick_range.y <= FTCOMMON_FASTFALL_STICK_RANGE_MIN) &&
        (fp->tap_stick_y < FTCOMMON_FASTFALL_BUFFER_TICS_MAX))
    {
        fp->is_fastfall = TRUE;
        fp->tap_stick_y = FTINPUT_STICKBUFFER_TICS_MAX;
        /* The source runs the new colanim's first frame at once. */
        if (ftParamCheckSetFighterColAnimID(fp->fighter_gobj,
                                            nGMColAnimFighterFastFall, 0) != FALSE)
        {
            ftMainRunUpdateColAnim(fp->fighter_gobj);
        }
    }
}

void ftPhysicsApplyAirVelDriftFastFall(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    FTAttributes *attr = (fp != NULL) ? fp->attr : NULL;

    if ((fp == NULL) || (attr == NULL))
    {
        return;
    }
    ftPhysicsCheckSetFastFall(fp);
    if (fp->is_fastfall != FALSE)
    {
        ftPhysicsApplyFastFall(fp, attr);
    }
    else
    {
        ftPhysicsApplyGravityDefault(fp, attr);
    }
    if (ftPhysicsCheckClampAirVelXDecMax(fp, attr) == FALSE)
    {
        ftPhysicsClampAirVelXStickDefault(fp, attr);
        ftPhysicsApplyAirVelXFriction(fp, attr);
    }
}

void ftPhysicsApplyAirVelFriction(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    FTAttributes *attr = (fp != NULL) ? fp->attr : NULL;

    if ((fp == NULL) || (attr == NULL))
    {
        return;
    }
    /* The source keeps a fast fall here (ftphysics.c 0x800D91EC). */
    if (fp->is_fastfall != FALSE)
    {
        ftPhysicsApplyFastFall(fp, attr);
    }
    else ftPhysicsApplyGravityDefault(fp, attr);

    if (ftPhysicsCheckClampAirVelXDecMax(fp, attr) == FALSE)
    {
        ftPhysicsApplyAirVelXFriction(fp, attr);
    }
}

/* The source's 0x800D90E0: the drift without the fast-fall check, so an
 * aerial attack or an item throw keeps a fast fall it started with but
 * cannot start one (only ftPhysicsApplyAirVelDriftFastFall does). */
void ftPhysicsApplyAirVelDrift(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    FTAttributes *attr = (fp != NULL) ? fp->attr : NULL;

    if ((fp == NULL) || (attr == NULL))
    {
        return;
    }
    if (fp->is_fastfall != FALSE)
    {
        ftPhysicsApplyFastFall(fp, attr);
    }
    else ftPhysicsApplyGravityDefault(fp, attr);

    if (ftPhysicsCheckClampAirVelXDecMax(fp, attr) == FALSE)
    {
        ftPhysicsClampAirVelXStickDefault(fp, attr);
        ftPhysicsApplyAirVelXFriction(fp, attr);
    }
}

void mpCommonSetFighterGround(FTStruct *fp)
{
    if (fp != NULL)
    {
        fp->physics.vel_ground.x =
            fp->physics.vel_air.x * (f32)fp->lr;
        fp->ga = nMPKineticsGround;
        fp->jumps_used = 0;
        fp->stat_flags.ga = nMPKineticsGround;
        fp->vel_ground = fp->physics.vel_ground;
    }
    if ((ndsFighterMarioFoxStageMPCliffWaitDamageLoopProofEnabled() !=
            FALSE) &&
        (sNdsStageMPCliffWaitDamageLoopDamageFallMapActive != FALSE) &&
        (fp != NULL) &&
        (fp->tics_since_last_z < FTCOMMON_PASSIVE_BUFFER_TICS_MAX))
    {
        if (ABS(fp->input.pl.stick_range.x) >=
            FTCOMMON_PASSIVE_F_OR_B_RANGE)
        {
            gNdsStageMPCliffWaitDamageLoopPassiveStandGroundSetCount++;
        }
        else
        {
            gNdsStageMPCliffWaitDamageLoopPassiveGroundSetCount++;
        }
    }
}

/* Declared here rather than through <ft/ftpublic.h>: the port mirrors many decomp
 * headers under include/ but has no ftpublic.h of its own, and this is the only
 * caller. The definition is already compiled -- src/import/battleship_ftpublic.c
 * includes decomp's ftpublic.c -- it was simply --gc-sections'd out of the ROM
 * for want of any caller at all, which is why the cue never fired. */
extern void ftPublicPlayCliffReact(GObj *fighter_gobj, f32 knockback);

/* Engagement proof for the crowd's near-KO gasp. Zero after a match in which
 * someone was launched toward a blast zone and survived means the branch below
 * never ran; the cue being silent while this counts is a different fault. */
volatile u32 gNdsFtPublicCliffReactCount;

/* TRUE while the cliff-catch seam (ndsMPCommonRunFighterCliffFloorCeil-
 * Collisions) runs the landing params: Remix's ledge_flag (Peach.asm
 * ledge_patch_), which her grounded_script reads. */
static sb32 sNdsP4CliffLanding;

void mpCommonSetFighterLandingParams(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp == NULL)
    {
        return;
    }
    /* THE CROWD'S NEAR-KO GASP. This function used to clear public_knockback and
     * nothing else, and the note here called the missing branch "audio debt" --
     * so FGM 615/616/617 (GaspL/M/S) were packed, allowlisted, and unreachable.
     *
     * Source, mpcommon.c:426-440: a fighter carrying >= 100 knockback who LANDS
     * within 450 units of a blast bound gets a crowd reaction scaled by that
     * knockback, and only then is the flag cleared. Both landing seams already
     * called this function -- :9161 for a floor and :11603 for a cliff catch --
     * so only the branch itself was absent.
     *
     * ftmain.c:1821 is the other half and is already live: it disarms
     * public_knockback every frame the fighter is back between the bounds, so
     * the flag survives only while they are still out by the edge. That is what
     * makes this the "sent flying and saved it" gasp rather than a landing sound.
     *
     * The joint NULL check is a port addition; source dereferences it unguarded.
     *
     * The fkind switch below is the source's whole (mpcommon.c:441-468). It
     * held only Mario's case: Luigi's cyclone never rose again after its first
     * aerial use (is_expend_tornado), and Samus's aerial charge shot lost
     * lift for the rest of the stock (charge_recoil). Samus's case returns in
     * the source; nothing of the source's follows, so it breaks here and the
     * proof counters below still run. A P4 content takes the case Remix's
     * grounded_script row names (ndsP4GroundedKind). */
    if (fp->public_knockback != 0.0F)
    {
        if ((fp->public_knockback >= 100.0F) &&
            (fp->joints[nFTPartsJointTopN] != NULL))
        {
            f32 topn_x = fp->joints[nFTPartsJointTopN]->translate.vec.f.x;

            if ((topn_x < (gMPCollisionBounds.current.left + 450.0F)) ||
                (topn_x > (gMPCollisionBounds.current.right - 450.0F)))
            {
                ftPublicPlayCliffReact(fighter_gobj, fp->public_knockback);
                NDS_DIAG(gNdsFtPublicCliffReactCount++);
            }
        }
    }
    fp->public_knockback = 0.0F;
    switch (ndsP4GroundedKind(fp))
    {
    case nFTKindMario:
    case nFTKindMMario:
    case nFTKindNMario:
        fp->passive_vars.mario.is_expend_tornado = FALSE;
        break;

    case nFTKindSamus:
    case nFTKindNSamus:
        fp->passive_vars.samus.charge_recoil = 0;
        break;

    case nFTKindLuigi:
    case nFTKindNLuigi:
        fp->passive_vars.mario.is_expend_tornado = FALSE;
        break;

    case nFTKindCaptain:
    case nFTKindNCaptain:
        fp->passive_vars.captain.falcon_punch_unk = FALSE;
        break;

    case nFTKindPurin:
    case nFTKindNPurin:
        fp->passive_vars.purin.unk_0x0 = FALSE;
        break;

    default:
        break;
    }
    /* A content's own grounded_script routine, which Remix's row runs in
     * place of a case (Peach's); the cliff seam marks a ledge catch. */
    ndsP4OnLanding(fp, sNdsP4CliffLanding);
}

void mpCommonRunDefaultCollision(MPCollData *coll_data, GObj *gobj,
                                 u32 flags)
{
    (void)gobj;
    (void)flags;
    if (coll_data == NULL)
    {
        return;
    }
    NDS_DIAG(gNdsCollisionRuntimeDiagnostics.default_run_calls++);
    /* Floor acquisition/resolution is graduated first.  The O2R wall runner
     * does not yet populate source line/angle state, so entering those source
     * branches here would be less faithful than leaving them deferred. */
    if (mpProcessRunFloorCollisionAdjNewNULL(coll_data) != FALSE)
    {
        mpProcessSetCollideFloor(coll_data);
        if ((coll_data->mask_stat & MAP_FLAG_FLOOR) != 0u)
        {
            mpProcessRunFloorEdgeAdjust(coll_data);
        }
    }
    else
    {
        mpProcessSetCollProjectFloorID(coll_data);
    }
}

void mpCommonCopyCollDataStats(MPCollData *this_coll_data, Vec3f *pos,
                               MPCollData *other_coll_data)
{
    if ((this_coll_data == NULL) || (pos == NULL) ||
        (other_coll_data == NULL))
    {
        return;
    }
    NDS_DIAG(gNdsCollisionRuntimeDiagnostics.default_copy_calls++);
    this_coll_data->pos_prev = *pos;
    this_coll_data->p_map_coll = &other_coll_data->map_coll;
    this_coll_data->mask_curr = 0u;
    this_coll_data->mask_unk = 0u;
    this_coll_data->mask_stat = 0u;
    this_coll_data->is_coll_end = FALSE;
    this_coll_data->update_tic = other_coll_data->update_tic;
}

void mpCommonResetCollDataStats(MPCollData *coll_data)
{
    if (coll_data == NULL)
    {
        return;
    }
    NDS_DIAG(gNdsCollisionRuntimeDiagnostics.default_reset_calls++);
    coll_data->p_map_coll = &coll_data->map_coll;
    coll_data->update_tic = gMPCollisionUpdateTic;
    coll_data->mask_curr = 0u;
}

void mpCommonRunFighterCollisionDefault(GObj *fighter_gobj, Vec3f *pos,
                                        FTCollisionData *coll_data)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if ((fp != NULL) && (pos != NULL) && (coll_data != NULL) &&
        (fp->coll_data.p_translate != NULL))
    {
        NDS_DIAG(gNdsCollisionRuntimeDiagnostics.default_fighter_calls++);
        mpCommonCopyCollDataStats(&fp->coll_data, pos, coll_data);
        mpCommonRunDefaultCollision(&fp->coll_data, fighter_gobj,
                                    MAP_PROC_TYPE_DEFAULT);
        mpCommonResetCollDataStats(&fp->coll_data);
    }
}

void ftParamSetPlayerTagWait(GObj *fighter_gobj, s32 wait)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp != NULL)
    {
        fp->playertag_wait = wait;
    }
}

void ftParamSetCaptureImmuneMask(FTStruct *fp, u8 capture_immune_mask)
{
    if (fp != NULL)
    {
        fp->capture_immune_mask = capture_immune_mask;
    }
    if (sNdsFTMainSetStatusCompatReplayActive != FALSE)
    {
        return;
    }
}

void ftParamSetCatchParams(FTStruct *fp, u8 catch_mask,
                           void (*proc_catch)(GObj *),
                           void (*proc_capture)(GObj *, GObj *))
{
    if (fp != NULL)
    {
        fp->is_catchstatus = TRUE;
        fp->catch_mask = catch_mask;
        fp->proc_catch = proc_catch;
        fp->proc_capture = proc_capture;
    }
}

void ftParamSetThrowParams(FTStruct *fp, GObj *throw_gobj)
{
    if (fp != NULL)
    {
        FTStruct *throw_fp =
            (throw_gobj != NULL) ? ftGetStruct(throw_gobj) : NULL;

        fp->throw_gobj = throw_gobj;
        if (throw_fp != NULL)
        {
            fp->throw_fkind = throw_fp->fkind;
            fp->throw_team = throw_fp->team;
            fp->throw_player = throw_fp->player;
            fp->throw_player_num = throw_fp->player_num;
        }
        else
        {
            fp->throw_fkind = nFTKindNull;
            fp->throw_team = 0;
            fp->throw_player = 0;
            fp->throw_player_num = 0;
        }
    }
}


void ftCommonCatchPullProcCatch(GObj *fighter_gobj)
{
    ndsBaseFTCommonCatchPullProcCatch(fighter_gobj);
    return;
}

void ftCommonCapturePulledProcCapture(GObj *fighter_gobj,
                                      GObj *capture_gobj)
{
    if (ndsP4CustomCapture(fighter_gobj, capture_gobj) != FALSE)
    {
        return;
    }
    ndsBaseFTCommonCapturePulledProcCapture(fighter_gobj, capture_gobj);
    return;
}

static void ndsFtParamStopVoice(FTStruct *fp)
{
    if (fp == NULL)
    {
        return;
    }
    if (fp->p_voice != NULL)
    {
        if ((fp->p_voice->sfx_id != 0) && (fp->p_voice->sfx_id == fp->voice_id))
        {
            func_80026738_27338(fp->p_voice);
        }
    }
    fp->p_voice = NULL;
    fp->voice_id = 0;
}

void ftParamPlayVoice(FTStruct *fp, u16 voice_id)
{
    if (fp == NULL)
    {
        return;
    }
    fp->p_voice = func_800269C0_275C0(voice_id);
    fp->voice_id = (fp->p_voice != NULL) ? fp->p_voice->sfx_id : 0;
}

void ftParamPlayLoopSFX(FTStruct *fp, u16 sfx_id)
{
    if ((fp != NULL) && (fp->p_loop_sfx == NULL))
    {
        fp->p_loop_sfx = func_800269C0_275C0(sfx_id);
        fp->loop_sfx_id =
            (fp->p_loop_sfx != NULL) ? fp->p_loop_sfx->sfx_id : 0;
    }
}

void ftParamStopLoopSFX(FTStruct *fp)
{
    if (fp == NULL)
    {
        return;
    }
    if (fp->p_loop_sfx != NULL)
    {
        if ((fp->p_loop_sfx->sfx_id != 0) &&
            (fp->p_loop_sfx->sfx_id == fp->loop_sfx_id))
        {
            func_80026738_27338(fp->p_loop_sfx);
        }
    }
    fp->p_loop_sfx = NULL;
    fp->loop_sfx_id = 0;
}

void gmRumbleStopRumbleID(s32 player, s32 rumble_id)
{
    (void)player;
    (void)rumble_id;
}

#if !NDS_1P_GAME
void ftBossCommonUpdateDamageStats(GObj *fighter_gobj)
{
    (void)fighter_gobj;
}
#endif

__attribute__((weak)) s32 itMainGetDamageOutput(ITStruct *ip)
{
    if (ip == NULL)
    {
        return 0;
    }
    return (s32)((ip->attack_coll.damage * ip->attack_coll.stale) + 0.999F);
}

static void ndsCompatSetHitInteractStats(GMAttackRecord *records,
                                         GObj *victim_gobj, s32 attack_type,
                                         u32 group_id, u32 rehit_time)
{
    s32 i;

    if (records == NULL)
    {
        return;
    }
    for (i = 0; i < GMATTACKREC_NUM_MAX; i++)
    {
        if (records[i].victim_gobj == victim_gobj)
        {
            break;
        }
    }
    if (i == GMATTACKREC_NUM_MAX)
    {
        for (i = 0; i < GMATTACKREC_NUM_MAX; i++)
        {
            if (records[i].victim_gobj == NULL)
            {
                break;
            }
        }
        if (i == GMATTACKREC_NUM_MAX)
        {
            i = 0;
        }
        records[i].victim_gobj = victim_gobj;
    }

    switch (attack_type)
    {
    case nGMHitTypeDamage:
        records[i].victim_flags.is_interact_hurt = TRUE;
        break;

    case nGMHitTypeShield:
        records[i].victim_flags.is_interact_shield = TRUE;
        break;

    case nGMHitTypeShieldRehit:
        records[i].victim_flags.is_interact_shield = TRUE;
        records[i].victim_flags.timer_rehit = rehit_time;
        break;

    case nGMHitTypeReflect:
        records[i].victim_flags.is_interact_reflect = TRUE;
        records[i].victim_flags.timer_rehit = rehit_time;
        break;

    case nGMHitTypeAbsorb:
        records[i].victim_flags.is_interact_absorb = TRUE;
        break;

    case nGMHitTypeAttack:
        records[i].victim_flags.group_id = group_id;
        break;

    case nGMHitTypeDamageRehit:
        records[i].victim_flags.is_interact_hurt = TRUE;
        records[i].victim_flags.timer_rehit = rehit_time;
        break;

    default:
        break;
    }
}

__attribute__((weak)) void itProcessSetHitInteractStats(
    ITAttackColl *attack_coll, GObj *victim_gobj, s32 attack_type,
    u32 victim_group_id)
{
    if (attack_coll == NULL)
    {
        return;
    }
    ndsCompatSetHitInteractStats(attack_coll->attack_records, victim_gobj,
                                 attack_type, victim_group_id,
                                 ITEM_REHIT_TIME_DEFAULT);
}

/* BattleShip ftparam.c:93-118: these durations define theme priority.
 * The source intentionally pairs Star with the Hammer duration and vice versa. */
s32 ftParamGetItemMusicLength(u32 bgm_id)
{
    switch (bgm_id)
    {
    case nSYAudioBGMStar:
        return ITHAMMER_BGM_DURATION;

    case nSYAudioBGMHammer:
        return ITSTAR_BGM_DURATION;

    default:
        return 0;
    }
}

void ftParamTryPlayItemMusic(u32 bgm_id)
{
    if (ftParamGetItemMusicLength(bgm_id) >= ftParamGetItemMusicLength(gMPCollisionBGMCurrent))
    {
        syAudioPlayBGM(0, bgm_id);
        gMPCollisionBGMCurrent = bgm_id;
    }
}

void ftParamSetStarHitStatusInvincible(FTStruct *fp, s32 invincible_tics)
{
    if (fp == NULL)
    {
        return;
    }
    fp->star_hitstatus = nGMHitStatusInvincible;
    fp->star_invincible_tics = invincible_tics;
    ftParamCheckSetFighterColAnimID(fp->fighter_gobj,
                                    nGMColAnimFighterStar, 0);
}

void ftParamSetHealDamage(FTStruct *fp, s32 heal)
{
    if (fp == NULL)
    {
        return;
    }
    fp->damage_heal += heal;
    ftParamCheckSetFighterColAnimID(fp->fighter_gobj,
                                    nGMColAnimFighterHeal, 0);
}

void ftParamStopVoiceRunProcDamage(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp == NULL)
    {
        return;
    }
    /* Remix's head of this function (PeachFloat.end_float_on_hit_). */
    ndsP4OnDamage(fp);
    ndsFtParamStopVoice(fp);
    if (fp->proc_damage != NULL)
    {
        fp->proc_damage(fighter_gobj);
    }
}

#if !NDS_DONKEY
/* Compile seam for builds without Donkey Kong, where `battleship_donkey.c` --
 * and therefore the source `ftDonkeyThrowFDamageSetStatus` -- is not compiled.
 * Nothing can reach it there: ftCommonDamageUpdateCatchResist only takes this
 * arm for a Donkey Kong holding a fighter or a heavy item.  With
 * NDS_DONKEY=1 the source setter is linked and battleship_ftcommon_damage.c
 * calls it directly (P2-3r10). */
void ndsCompatFTDonkeyThrowFDamageSetStatus(GObj *fighter_gobj)
{
    ftCommonDamageGotoDamageStatus(fighter_gobj);
}
#endif

/* decomp ft/fighter.h:24-29. The source's ftSetupDropItem is a macro that
 * drops the held item where it is: itMainSetFighterDrop with a zero velocity
 * and throw_mul 1.0 runs the kind's Dropped status, unhooks the item from the
 * holder's joint and clears fp->item_gobj (itmain.c:318-375). Clearing only
 * fp->item_gobj left the item in its Hold status, still riding the holder's
 * item joint through the 0x52 matrix and never pickable again: a barrel or
 * crate knocked out of a lifting fighter's hands (ftCommonHeavyGetProcDamage)
 * followed the damage animation instead of falling, rolling or breaking. */
NDS_COLD void ftSetupDropItem(FTStruct *fp)
{
    Vec3f vel;

    if ((fp == NULL) || (fp->item_gobj == NULL))
    {
        return;
    }
    vel.x = vel.y = vel.z = 0.0F;
    itMainSetFighterDrop(fp->item_gobj, &vel, 1.0F);
}

void ftCommonThrownSetStatusDamageRelease(GObj *fighter_gobj)
{
    ndsBaseFTCommonThrownSetStatusDamageRelease(fighter_gobj);
    return;
}

void ftCommonThrownUpdateDamageStats(FTStruct *this_fp)
{
    ndsBaseFTCommonThrownUpdateDamageStats(this_fp);
    return;
}

void ftCommonThrownSetStatusNoDamageRelease(GObj *fighter_gobj)
{
    ndsBaseFTCommonThrownSetStatusNoDamageRelease(fighter_gobj);
    return;
}

void ftCommonThrownDecideFighterLoseGrip(GObj *fighter_gobj,
                                         GObj *interact_gobj)
{
    ndsBaseFTCommonThrownDecideFighterLoseGrip(fighter_gobj, interact_gobj);
    return;
}

void ftCommonThrownDecideDeadResult(GObj *fighter_gobj)
{
    ndsBaseFTCommonThrownDecideDeadResult(fighter_gobj);
    return;
}

void ftParamSetHitStatusAll(GObj *fighter_gobj, s32 hitstatus)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp != NULL)
    {
        fp->hitstatus = hitstatus;
        ndsFTParamSetHitStatusColAnim(fighter_gobj, hitstatus);
    }
}

extern void gmCollisionCopyMatrix(Mtx44f dst, Mtx44f src);


/* BUGS.md #2: this returned identity plus the joint's *local* translate, so a
 * grab placed the victim at the capturer's hand offset measured from the world
 * origin instead of from the hand -- the snap the owner sees. The contract is
 * the joint's world matrix, and ftcommoncapturepulled.c's non-US branch spells
 * the same computation out: func_ovl2_800EDBA4 to rebuild the chain, then read
 * parts->mtx_translate. That cache is live here -- ndsFTParamsInvalidateFighterParts
 * clears unk_dobjtrans_word every frame -- so use it rather than recomposing.
 * ftcommoncapturepulled.c is the only caller; scsubsysfighter.c declares this
 * but never calls it. */
void func_ovl0_800C9A38(Mtx44f mtx, DObj *dobj)
{
    FTParts *parts;
    s32 i;
    s32 j;

    for (i = 0; i < 4; i++)
    {
        for (j = 0; j < 4; j++)
        {
            mtx[i][j] = (i == j) ? 1.0F : 0.0F;
        }
    }
    if ((dobj == NULL) || (dobj->parent_gobj == NULL))
    {
        return;
    }
    func_ovl2_800EDBA4(dobj);
    parts = ftGetParts(dobj);
    if (parts != NULL)
    {
        gmCollisionCopyMatrix(mtx, parts->mtx_translate);
    }
}

/* BUGS.md "Fox's pistol model is missing": Fox's gun is not an object or an
 * effect, it is model part 13 on joint 17 (FTFOX_BLASTER_HOLD_JOINT), and its
 * geometry is file 315 at 0x3F0 -- already shipped (Makefile
 * reloc_extern_data/MiscData315) and already resolved through
 * dFoxMain_modelparts_container, whose desc[13] points at it.
 *
 * The request has been live all along: the port #includes ftmain.c verbatim
 * (battleship_ftmain.c:81) and its decoder dispatches
 * nFTMotionEventSetModelPartID at ftmain.c:575 straight into this function.
 * Discarding the arguments here is why no gun appears.
 *
 * The old shim stopped at the state mirror. That was insufficient: the DS
 * display contract captures the source DObj's live DL/flags and its draw-plan
 * memo can persist across motion events. Restore the complete source mutation
 * below and invalidate those DS caches at the same writer seam. */
void ftParamSetModelPartID(GObj *fighter_gobj, s32 joint_id,
                           s32 modelpart_id)
{
    FTStruct *fp = (fighter_gobj != NULL) ? ftGetStruct(fighter_gobj) : NULL;
    s32 slot;

    if ((fp == NULL) || (joint_id < nFTPartsJointCommonStart) ||
        ((u32)joint_id >= ARRAY_COUNT(fp->joints)))
    {
        return;
    }
    if (fp->nds_p4_kirby_copy != 0u)
    {
        modelpart_id = ndsP4KirbyModelPart(fp, joint_id, modelpart_id);
    }
    slot = joint_id - nFTPartsJointCommonStart;
    if (((u32)slot >= ARRAY_COUNT(fp->modelpart_status)) ||
        (fp->joints[joint_id] == NULL))
    {
        return;
    }
    if (fp->modelpart_status[slot].modelpart_id_curr == (s8)modelpart_id)
    {
        return;
    }
    fp->modelpart_status[slot].modelpart_id_curr = (s8)modelpart_id;
    (void)ndsFTParamApplyModelPartCurrent(fighter_gobj, fp, slot);
    fp->is_modelpart_modify = TRUE;
    NDS_DIAG(gNdsFighterModelPartSetCount++);
    if (modelpart_id >= 0)
    {
        NDS_DIAG(gNdsFighterModelPartOnCount++);
    }
    ndsFTParamInvalidateModelPartRenderer(fighter_gobj, fp);
}

sb32 ndsFTManagerLowDetailOnly(s32 player);

void ftParamSetModelPartDetailAll(GObj *fighter_gobj, u8 detail)
{
    FTStruct *fp = (fighter_gobj != NULL) ? ftGetStruct(fighter_gobj) : NULL;
    s32 i;

    /* P7 C1, owner 2026-10-19: a CPU (or a wireless guest) in a campaign or
     * VS battle of three or four keeps its low detail (battleship_ftmanager.c
     * ndsFTManagerLowDetailOnly: its high image is not loaded there). */
    if ((fp != NULL) && (detail == nFTPartsDetailHigh) &&
        (ndsFTManagerLowDetailOnly(fp->player) != FALSE))
    {
        detail = nFTPartsDetailLow;
    }

    /* Owner 2026-10-08 (S15): the KO and pause close-ups keep a content's
     * low-detail model in a battle of three or four fighters (its high
     * image is not admitted there). */
    if ((fp != NULL) && (fp->nds_p4_content != 0u) &&
        (detail == nFTPartsDetailHigh) &&
        (ndsP4ContentLowDetailOnly(fp->player) != FALSE))
    {
        detail = nFTPartsDetailLow;
    }
    /* S8: so does a Kirby holding a content's power, in any VS battle: the
     * match admits his Remix hats in the low detail only
     * (nds_kirby_hat_residency.c), whatever its fighter count. */
    if ((fp != NULL) && (fp->fkind == nFTKindKirby) && (fp->nds_p4_kirby_copy != 0u) &&
        (detail == nFTPartsDetailHigh) &&
        (gSCManagerSceneData.scene_curr == nSCKindVSBattle))
    {
        detail = nFTPartsDetailLow;
    }
    if ((fp == NULL) || (detail == fp->detail_curr))
    {
        return;
    }
    fp->detail_curr = detail;
    for (i = nFTPartsJointCommonStart; i < (s32)ARRAY_COUNT(fp->joints); i++)
    {
        if (fp->joints[i] != NULL)
        {
            s32 slot = i - nFTPartsJointCommonStart;
            s32 modelpart_id = fp->modelpart_status[slot].modelpart_id_curr;

            if (modelpart_id != -1)
            {
                fp->modelpart_status[slot].modelpart_id_curr = -1;
                ftParamSetModelPartID(fighter_gobj, i, modelpart_id);
            }
        }
    }
    ftParamInitTexturePartAll(fighter_gobj);
}

sb32 ftCommonThrowCheckInterruptCatchWait(GObj *fighter_gobj)
{
    return ndsBaseFTCommonThrowCheckInterruptCatchWait(fighter_gobj);
}

void ftCommonThrownReleaseThrownUpdateStats(GObj *fighter_gobj, s32 lr,
                                            s32 script_id,
                                            sb32 is_proc_status)
{
    ndsBaseFTCommonThrownReleaseThrownUpdateStats(fighter_gobj, lr, script_id, is_proc_status);
    return;
}

void ftCommonCaptureShoulderedSetStatus(GObj *fighter_gobj)
{
#if NDS_DONKEY
    ndsBaseFTCommonCaptureShoulderedSetStatus(fighter_gobj);
#else
    (void)fighter_gobj;
#endif
}

#if NDS_DONKEY
void ftCommonCaptureShoulderedProcInterrupt(GObj *fighter_gobj)
{
    if (ndsP4ShoulderedInterrupt(fighter_gobj) != FALSE)
    {
        return;
    }
    ndsBaseFTCommonCaptureShoulderedProcInterrupt(fighter_gobj);
}
#endif

#if !NDS_DONKEY
void ftDonkeyThrowFWaitSetStatus(GObj *fighter_gobj)
{
    (void)fighter_gobj;
}
#endif

void ftCommonThrownReleaseFighterLoseGrip(GObj *fighter_gobj)
{
    ndsBaseFTCommonThrownReleaseFighterLoseGrip(fighter_gobj);
    return;
}

/* BattleShip dFTCommonDataHandicapTable from ft/ftcommondata.c:4-78.
 * The rest of that TU is item/audio data outside this combat slice. */
static const f32 sNDSFTCommonDataHandicapTable[][2] =
{
    { 0.55F, 1.8181818F },
    { 0.6F, 15.0F / 9.0F },
    { 0.7F, 10.0F / 7.0F },
    { 0.75F, 12.0F / 9.0F },
    { 0.8F, 11.25F / 9.0F },
    { 0.9F, 10.0F / 9.0F },
    { 0.95F, 10.0F / 9.5F },
    { 1.0F, 9.0F / 9.0F },
#if defined(REGION_US)
    { 1.09F, 0.9174312F },
    { 0.5F, 45.0F / 9.0F },
    { 0.55F, 36.0F / 9.0F },
    { 0.6F, 30.0F / 9.0F },
    { 0.65F, 20.0F / 7.0F },
    { 0.7F, 22.5F / 9.0F },
    { 0.75F, 1.8181818F },
    { 0.8F, 1.6949153F },
    { 0.85F, 1.5873016F },
    { 0.9F, 1.4925373F },
    { 0.95F, 1.4084507F },
    { 0.65F, 30.0F / 9.0F },
    { 0.7F, 20.0F / 7.0F },
    { 0.74F, 22.5F / 9.0F },
    { 0.77F, 20.0F / 9.0F },
    { 0.8F, 18.0F / 9.0F },
    { 1.0F, 4.0F / 7.0F },
    { 1.05F, 0.53763443F },
    { 1.1F, 0.5128205F },
    { 1.15F, 0.4761905F },
    { 1.23F, 0.45045045F },
    { 1.05F, 0.43478262F },
    { 1.1F, 3.0F / 7.5F },
    { 1.15F, (10.0F / 3.0F) / 9.0F },
    { 1.2F, 0.35714287F },
    { 1.25F, 3.0F / 9.0F },
#else
    { 1.1F, 0.9090909F },
    { 0.6F, 45.0F / 9.0F },
    { 0.6F, 36.0F / 9.0F },
    { 0.6F, 30.0F / 9.0F },
    { 0.6F, 20.0F / 7.0F },
    { 0.6F, 22.5F / 9.0F },
    { 0.9F, 1.8181818F },
    { 0.95F, 1.6949153F },
    { 1.0F, 1.5873016F },
    { 1.0F, 1.4925373F },
    { 1.0F, 1.4084507F },
    { 0.7F, 30.0F / 9.0F },
    { 0.75F, 25.0F / 8.0F },
    { 0.8F, 20.0F / 7.0F },
    { 1.0F, 25.0F / 9.5F },
    { 0.9F, 22.5F / 9.0F },
    { 1.05F, 4.0F / 7.0F },
    { 1.09F, 0.53763443F },
    { 1.13F, 0.5128205F },
    { 1.17F, 0.4761905F },
    { 1.23F, 0.45045045F },
    { 0.95F, 0.43478262F },
    { 1.0F, 3.0F / 7.5F },
    { 1.05F, (10.0F / 3.0F) / 9.0F },
    { 1.1F, 0.35714287F },
    { 1.1F, 3.0F / 9.0F },
#endif
    { 0.9F, 9.0F / 9.0F },
    { 1.0F, 9.0F / 9.0F },
    { 1.1F, 9.0F / 9.0F },
    { 1.22F, 9.0F / 9.0F },
    { 1.5F, 9.0F / 9.0F },
#if defined(REGION_US)
    { 1.08F, (200.0F / 24.0F) / 9.0F }
#else
    { 1.0F, 9.0F / 9.0F }
#endif
};

f32 ftParamGetCommonKnockback(s32 percent_damage, s32 recent_damage,
                              s32 hit_damage, s32 knockback_weight,
                              s32 knockback_scale, s32 knockback_base,
                              f32 weight, s32 attack_handicap,
                              s32 defend_handicap)
{
    const s32 default_handicap = 9;
    const s32 handicap_min = 1;
    const s32 handicap_max =
        (s32)ARRAY_COUNT(sNDSFTCommonDataHandicapTable);
    f32 damage_ratio =
        (gSCManagerBattleState != NULL) ?
            ((f32)gSCManagerBattleState->damage_ratio * 0.01F) : 1.0F;
    f32 knockback;

    if ((attack_handicap < handicap_min) || (attack_handicap > handicap_max))
    {
        attack_handicap = default_handicap;
    }
    if ((defend_handicap < handicap_min) || (defend_handicap > handicap_max))
    {
        defend_handicap = default_handicap;
    }
    if (knockback_weight != 0)
    {
        knockback = ((((((1.0F +
            (10.0F * (f32)knockback_weight * 0.05F)) * weight * 1.4F) +
            18.0F) * ((f32)knockback_scale * 0.01F)) +
            (f32)knockback_base) * damage_ratio *
            sNDSFTCommonDataHandicapTable[attack_handicap - 1][0]) *
            sNDSFTCommonDataHandicapTable[defend_handicap - 1][1];
    }
    else
    {
        f32 damage_add = (f32)(percent_damage + recent_damage);

        knockback = ((((((((damage_add * 0.1F) +
            (damage_add * (f32)hit_damage * 0.05F)) * weight * 1.4F) +
            18.0F) * ((f32)knockback_scale * 0.01F)) +
            (f32)knockback_base) * damage_ratio *
            sNDSFTCommonDataHandicapTable[attack_handicap - 1][0]) *
            sNDSFTCommonDataHandicapTable[defend_handicap - 1][1]);
    }
    if (knockback >= 2500.0F)
    {
        knockback = 2500.0F;
    }
    if ((gSCManagerBackupData.error_flags & LBBACKUP_ERROR_RANDOMKNOCKBACK) !=
        0)
    {
        knockback = syUtilsRandFloat() * 200.0F;
    }
    return knockback;
}

s32 ftParamGetHitLag(s32 damage, s32 status_id, f32 hitlag_mul)
{
#if defined(REGION_US)
    s32 hitlag_tics =
        (s32)((s32)(((f32)damage * (1.0F / 3.0F)) + 5.0F) * hitlag_mul);
#else
    s32 hitlag_tics =
        (s32)((s32)(((f32)damage * (1.0F / 3.0F)) + 4.0F) * hitlag_mul);
#endif

    /* BattleShip ftparam.c:1511-1523 applies hitlag_mul before the crouch
     * reduction, with an integer truncation at each assignment. The old DS
     * helper moved hitlag_mul after the crouch branch, which is not algebraically
     * equivalent once those truncations are included. */
    if ((status_id == nFTCommonStatusSquat) ||
        (status_id == nFTCommonStatusSquatWait))
    {
        hitlag_tics = (s32)((f32)hitlag_tics * (2.0F / 3.0F));
    }
    return hitlag_tics;
}

void ftParamSetDamageShuffle(FTStruct *fp, sb32 is_electric, s32 damage,
                             s32 status_id, f32 hitlag_mul)
{
    static const u8 sShuffleFrameIndexMax[2] = { 4u, 3u };
    u32 index = (is_electric != FALSE) ? 1u : 0u;

    if (fp == NULL)
    {
        return;
    }
    fp->shuffle_tics =
        (s32)((f32)ftParamGetHitLag(damage, status_id, hitlag_mul) * 1.3F);
    fp->shuffle_frame_index = 0;
    fp->is_shuffle_electric = (index != 0u) ? TRUE : FALSE;
    fp->shuffle_index_max = sShuffleFrameIndexMax[index];
}

s32 ftParamGetStaledDamage(s32 player, s32 damage, s32 attack_id,
                           u16 motion_count)
{
    extern f32 ftParamGetStale(s32 player, s32 attack_id, s32 motion_count);
    f32 stale = ftParamGetStale(player, attack_id, motion_count);

    /* BattleShip ftparam.c:1608-1617. This is the actual damage-side consumer
     * of the per-player stale queue restored by P2-2. Leaving this as the old
     * identity stub meant the queue could be perfectly maintained and still
     * have zero gameplay effect. Preserve the source's +0.999F rounding before
     * the implicit integer conversion. */
    if (stale != 1.0F)
    {
        damage = (damage * stale) + 0.999F;
    }
    return damage;
}

s32 ftParamGetBestHitStatusPart(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    s32 hitstatus_base;
    s32 hitstatus_best;
    s32 i;

    if (fp == NULL)
    {
        return nGMHitStatusNone;
    }

    /* BattleShip ftparam.c:645-672. Per-hurtbox state participates in the
     * fighter's effective collision status; the old DS all-status helper read
     * only fp->hitstatus and therefore ignored part invincibility/intangibility. */
    hitstatus_base = fp->hitstatus;
    hitstatus_best = fp->damage_colls[0].hitstatus;

    if (hitstatus_best != nGMHitStatusNormal)
    {
        for (i = 1; i < (s32)ARRAY_COUNT(fp->damage_colls); i++)
        {
            FTDamageColl *damage_coll = &fp->damage_colls[i];

            if (damage_coll->hitstatus == nGMHitStatusNone)
            {
                break;
            }
            else if (hitstatus_best > damage_coll->hitstatus)
            {
                hitstatus_best = damage_coll->hitstatus;
            }
        }
    }
    if (hitstatus_base < hitstatus_best)
    {
        hitstatus_base = hitstatus_best;
    }
    return hitstatus_base;
}

s32 ftParamGetBestHitStatusAll(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    s32 hitstatus_best;

    if (fp == NULL)
    {
        return nGMHitStatusNone;
    }

    /* BattleShip ftparam.c:676-689. Respawn/star and timed special hitstatus
     * are global overrides on top of the best per-part status. Source throws,
     * captures and ground hazards all consume this aggregate result. */
    hitstatus_best = ftParamGetBestHitStatusPart(fighter_gobj);
    if (hitstatus_best < fp->star_hitstatus)
    {
        hitstatus_best = fp->star_hitstatus;
    }
    if (hitstatus_best < fp->special_hitstatus)
    {
        hitstatus_best = fp->special_hitstatus;
    }
    return hitstatus_best;
}

#ifndef FTCOMMON_DAMAGE_EFFECT_KNOCKBACK_LOW
#define FTCOMMON_DAMAGE_EFFECT_KNOCKBACK_LOW 120.0F
#define FTCOMMON_DAMAGE_EFFECT_KNOCKBACK_MID_LOW 150.0F
#define FTCOMMON_DAMAGE_EFFECT_KNOCKBACK_MID 200.0F
#define FTCOMMON_DAMAGE_EFFECT_KNOCKBACK_MID_HIGH 300.0F
#define FTCOMMON_DAMAGE_EFFECT_KNOCKBACK_HIGH 600.0F
#define FTCOMMON_DAMAGE_EFFECT_WAIT_LOW 0
#define FTCOMMON_DAMAGE_EFFECT_WAIT_MID_LOW 8
#define FTCOMMON_DAMAGE_EFFECT_WAIT_MID 5
#define FTCOMMON_DAMAGE_EFFECT_WAIT_MID_HIGH 3
#define FTCOMMON_DAMAGE_EFFECT_WAIT_HIGH 2
#define FTCOMMON_DAMAGE_EFFECT_WAIT_DEFAULT 1
#define FTCOMMON_DAMAGE_PUBLIC_REACT_GASP_ANGLE_LOW F_CLC_DTOR32(75.0F)
#define FTCOMMON_DAMAGE_PUBLIC_REACT_GASP_ANGLE_HIGH F_CLC_DTOR32(115.0F)
#define FTCOMMON_DAMAGE_PUBLIC_REACT_GASP_KNOCKBACK_MUL 0.8F
#define FTCOMMON_DAMAGE_KNOCKBACK_VERYHIGH 160.0F
#define FTCOMMON_DAMAGE_FIGHTER_FLYTOP_ANGLE_LOW F_CLC_DTOR32(70.0F)
#define FTCOMMON_DAMAGE_FIGHTER_FLYTOP_ANGLE_HIGH F_CLC_DTOR32(110.0F)
#define FTCOMMON_DAMAGE_FIGHTER_FLYROLL_DAMAGE_MIN 100
#define FTCOMMON_DAMAGE_FIGHTER_FLYROLL_RANDOM_CHANCE 0.5F
#define FTCOMMON_DAMAGE_FIGHTER_PLAYERTAG_KNOCKBACK_MIN 130.0F
#define FTCOMMON_DAMAGE_FIGHTER_PLAYERTAG_HIDE_FRAMES 10
#define FTCOMMON_WALLDAMAGE_INTANGIBLE_TIMER 15
#endif

/* The dash-run damage proof, split out of the crowd stub that used to be the
 * only body of ftPublicCommonCheck. It records; it never decided anything. The
 * real actor (battleship_ftpublic.c) calls this and then the source check, so
 * importing the crowd cannot silently retire a proof. */
void ndsFighterDashRunRecordPublicCheck(GObj *fighter_gobj, f32 knockback,
                                        sb32 is_force_curr_knockback)
{
    (void)fighter_gobj;
}

/* Declared unconditionally, defined either here or in battleship_ftpublic.c.
 * `ft/fighter.h` declares ftPublicMakeActor and not this one, so at
 * NDS_IMPORT_BATTLESHIP_FT_PUBLIC=1 the definition below disappears and the
 * call at ndsFTCommonDamageSetPublic has no prototype in scope -- which is a
 * hard error, and the reason the flag had never been compiled. */
void ftPublicCommonCheck(GObj *fighter_gobj, f32 knockback,
                         sb32 is_force_curr_knockback);


GObj *ftParamGetPlayerNumGObj(s32 player_num)
{
    return ndsFighterGetPlayerNumGObj(player_num);
}

static void ndsFTCommonDamageSetPublic(FTStruct *fp, f32 knockback,
                                       f32 angle)
{
    GObj *attacker_gobj;
    sb32 is_force_curr_knockback = FALSE;

    if (fp == NULL)
    {
        return;
    }
    fp->status_vars.common.damage.public_knockback = knockback;
    fp->public_knockback = 0.0F;
    if ((angle > FTCOMMON_DAMAGE_PUBLIC_REACT_GASP_ANGLE_LOW) &&
        (angle < FTCOMMON_DAMAGE_PUBLIC_REACT_GASP_ANGLE_HIGH))
    {
        fp->status_vars.common.damage.public_knockback *=
            FTCOMMON_DAMAGE_PUBLIC_REACT_GASP_KNOCKBACK_MUL;
    }
    attacker_gobj = ndsFighterGetPlayerNumGObj(fp->damage_player_num);
    if (attacker_gobj != NULL)
    {
        FTStruct *attacker_fp = ftGetStruct(attacker_gobj);
        if ((attacker_fp != NULL) &&
            (attacker_fp->public_knockback >=
                FTCOMMON_DAMAGE_KNOCKBACK_VERYHIGH))
        {
            is_force_curr_knockback = TRUE;
        }
    }
    ftPublicCommonCheck(fp->fighter_gobj,
                        fp->status_vars.common.damage.public_knockback,
                        is_force_curr_knockback);
}

void ftCommonDamageSetPublic(FTStruct *fp, f32 knockback, f32 angle)
{
    ndsBaseFTCommonDamageSetPublic(fp, knockback, angle);
    return;
}

void ftCommonDamageSetDustEffectInterval(FTStruct *fp)
{
    ndsBaseFTCommonDamageSetDustEffectInterval(fp);
    return;
}

NDS_COLD f32 ftCommonDamageGetKnockbackAngle(s32 angle_i, sb32 ga, f32 knockback)
{
    return ndsBaseFTCommonDamageGetKnockbackAngle(angle_i, ga, knockback);
}

s32 ftCommonDamageGetDamageLevel(f32 hitstun)
{
    return ndsBaseFTCommonDamageGetDamageLevel(hitstun);
}

sb32 ftCommonDamageCheckCatchResist(FTStruct *fp)
{
    return ndsBaseFTCommonDamageCheckCatchResist(fp);
}

sb32 ftCommonDamageCheckCaptureKeepHold(FTStruct *fp)
{
    return ndsBaseFTCommonDamageCheckCaptureKeepHold(fp);
}

sb32 ftCommonDamageCheckElementSetColAnim(GObj *fighter_gobj, s32 element,
                                          s32 damage_level)
{
    return ndsBaseFTCommonDamageCheckElementSetColAnim(fighter_gobj, element, damage_level);
}

void ifScreenFlashSetColAnimID(s32 colanim_id, s32 colanim_duration)
{
    ndsBattleShipIFScreenFlashSetColAnimID(colanim_id, colanim_duration);
}

void ftCommonDamageCheckMakeScreenFlash(f32 knockback, s32 element)
{
    ndsBaseFTCommonDamageCheckMakeScreenFlash(knockback, element);
    return;
}

void ftCommonDamageInitDamageVars(GObj *fighter_gobj, s32 status_id_replace,
                                  s32 damage, f32 knockback,
                                  s32 angle_start, s32 damage_lr,
                                  s32 damage_index, s32 element,
                                  s32 damage_player_num, s32 arg9,
                                  sb32 unk_bool, sb32 is_public)
{
    NDS_FREEZE_DIAGNOSTICS_MARK(NDS_FREEZE_BREADCRUMB_DAMAGE_ENTER);
    ndsBaseFTCommonDamageInitDamageVars(fighter_gobj, status_id_replace, damage, knockback, angle_start, damage_lr, damage_index, element, damage_player_num, arg9, unk_bool, is_public);
    return;
}

void ftCommonDamageGotoDamageStatus(GObj *fighter_gobj)
{
    ndsBaseFTCommonDamageGotoDamageStatus(fighter_gobj);
    return;
}

extern void battleship_ftMainRunUpdateColAnim(GObj *fighter_gobj);

void ftMainRunUpdateColAnim(GObj *fighter_gobj)
{
    battleship_ftMainRunUpdateColAnim(fighter_gobj);
}

void ftCommonDamageUpdateDamageColAnim(GObj *fighter_gobj, f32 knockback,
                                       s32 element)
{
    ndsBaseFTCommonDamageUpdateDamageColAnim(fighter_gobj, knockback, element);
    return;
}

NDS_COLD void ftCommonDamageSetDamageColAnim(GObj *fighter_gobj)
{
    ndsBaseFTCommonDamageSetDamageColAnim(fighter_gobj);
    return;
}

void ftCommonDamageUpdateMain(GObj *fighter_gobj)
{
    ndsBaseFTCommonDamageUpdateMain(fighter_gobj);
    return;
}

void ftParamUpdate1PGameDamageStats(FTStruct *fp, s32 damage_player,
                                    s32 damage_object_class,
                                    s32 damage_object_kind, u16 flags,
                                    u16 damage_stat_count)
{
    if (fp != NULL)
    {
        /* BattleShip ftparam.c:1757-1780. These first fields are ordinary
         * gameplay attribution despite the function's 1P-oriented name; VS
         * death/scoring code consumes damage_player later.  Keep the attacker's
         * stat metadata in damage_stat_flags -- the old shim overwrote the
         * VICTIM'S live stat_flags, which can change its own next attack. */
        fp->damage_player = damage_player;
        fp->damage_object_class = damage_object_class;
        fp->damage_object_kind = damage_object_kind;
        fp->damage_count++;
        if ((damage_stat_count == 0u) ||
            (fp->damage_stat_count != damage_stat_count))
        {
            fp->damage_stat_flags.halfword = flags;
            fp->damage_stat_count = damage_stat_count;
            /* The source's remaining body updates 1P bonus counters only.
             * Those counters belong to P2-6 and are not present in the port;
             * there is intentionally no VS-side substitute here. */
        }
    }
}

NDS_COLD void ftCommonFuraSleepSetStatus(GObj *fighter_gobj)
{
    ndsBaseFTCommonFuraSleepSetStatus(fighter_gobj);
    return;
}

NDS_COLD void ftCommonTwisterSetStatus(GObj *fighter_gobj, GObj *tornado_gobj)
{
    ndsBaseFTCommonTwisterSetStatus(fighter_gobj, tornado_gobj);
    return;
}

void ftCommonTaruCannProcPhysics(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    GObj *tarucann_gobj;
    DObj *fighter_root;
    DObj *tarucann_root;

    if (fp == NULL)
    {
        return;
    }
    tarucann_gobj = fp->status_vars.common.tarucann.tarucann_gobj;
    if (tarucann_gobj == NULL)
    {
        return;
    }
    fighter_root = DObjGetStruct(fighter_gobj);
    tarucann_root = DObjGetStruct(tarucann_gobj);
    if ((fighter_root == NULL) || (tarucann_root == NULL))
    {
        return;
    }
    fighter_root->translate.vec.f = tarucann_root->translate.vec.f;
}

#if NDS_STAGE_JUNGLE
volatile u32 gNdsTaruCannCaptureCount __attribute__((used));
volatile u32 gNdsTaruCannFireInputCount __attribute__((used));
volatile u32 gNdsTaruCannFireAutoCount __attribute__((used));
volatile u32 gNdsTaruCannLaunchCount __attribute__((used));
volatile s32 gNdsTaruCannLaunchAngle __attribute__((used));
volatile f32 gNdsTaruCannLaunchRotate __attribute__((used));
volatile f32 gNdsTaruCannLaunchKnockback __attribute__((used));
#endif

void ftCommonTaruCannSetStatus(GObj *fighter_gobj, GObj *tarucann_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp == NULL)
    {
        return;
    }

    ftParamStopVoiceRunProcDamage(fighter_gobj);
    if ((fp->item_gobj != NULL) &&
        (itGetStruct(fp->item_gobj)->weight == nITWeightHeavy))
    {
        ftSetupDropItem(fp);
    }
    if (fp->catch_gobj != NULL)
    {
        ftCommonThrownSetStatusDamageRelease(fp->catch_gobj);
        fp->catch_gobj = NULL;
    }
    else if (fp->capture_gobj != NULL)
    {
        ftCommonThrownDecideFighterLoseGrip(fp->capture_gobj, fighter_gobj);
    }

    ftMainSetStatus(fighter_gobj, nFTCommonStatusTaruCann, 0.0F, 0.0F,
                    FTSTATUS_PRESERVE_NONE);
#if NDS_STAGE_JUNGLE
    NDS_DIAG(gNdsTaruCannCaptureCount++);
#endif
    ftMainPlayAnimEventsAll(fighter_gobj);
    ftPhysicsStopVelAll(fighter_gobj);

    fp->status_vars.common.tarucann.shoot_wait = 0;
    fp->status_vars.common.tarucann.release_wait = 0;
    fp->status_vars.common.tarucann.tarucann_gobj = tarucann_gobj;

    ftParamSetHitStatusAll(fighter_gobj, nGMHitStatusIntangible);
    fp->is_invisible = TRUE;

    ftParamSetCaptureImmuneMask(fp, FTCATCHKIND_MASK_ALL);
    (void)func_800269C0_275C0(nSYAudioFGMJungleTaruCannEnter);
}
