/* Source-function shims, fighter effects and setup: the barrel cannon and
 * captures, the dust and sparkle effects, the visual damage scale, animation
 * locks and the parts' figatree and setup. A slice of reloc_backend.c. */

#if NDS_P2_STAGE_JUNGLE
/* P2-4s3. The fighter half of Congo Jungle's barrel cannon, transcribed from
 * decomp ft/ftcommon/ftcommontarucann.c:8-48 and :97-116. SetStatus and
 * ProcPhysics above were already here; these three are what made a captured
 * fighter sit in the cannon forever, because the status arm assigned
 * proc_update and proc_interrupt NULL for want of them.
 *
 * Constants are the source's own (ft/ftcommon.h:160-166): 180 frames before
 * the cannon fires on its own, 10 frames from armed to release, 16 frames
 * before it can be used again.
 */
#define NDS_FTCOMMON_TARUCANN_RELEASE_WAIT 180
#define NDS_FTCOMMON_TARUCANN_SHOOT_WAIT 10
#define NDS_FTCOMMON_TARUCANN_PICKUP_WAIT 16

/* decomp ft/ftparam.c:1478-1501, imported here because nothing in the port had
 * it and two stages need it: this cannon and Hyrule Castle's tornado. The
 * knockback_weight != 0 arm is the one a ground hazard takes -- its damage is
 * fixed rather than accumulated -- and the random-knockback debug arm is
 * transcribed with it rather than dropped, because gSCManagerBackupData's
 * error flags are real state this port already carries. */
f32 ftParamGetGroundHazardKnockback(s32 percent_damage, s32 recent_damage,
                                    s32 hit_damage, s32 knockback_weight,
                                    s32 knockback_scale, s32 knockback_base,
                                    f32 weight, s32 attack_handicap,
                                    s32 defend_handicap)
{
    f32 knockback;

    if (knockback_weight != 0)
    {
        knockback = ((((((1.0F + (10.0F * knockback_weight * 0.05F)) *
                         weight * 1.4F) + 18.0F) *
                       (knockback_scale * 0.01F)) + knockback_base) *
                     sNDSFTCommonDataHandicapTable[attack_handicap - 1][0]) *
                    sNDSFTCommonDataHandicapTable[defend_handicap - 1][1];
    }
    else
    {
        f32 damage_add = (f32)(percent_damage + recent_damage);

        knockback = (((((((damage_add * 0.1F) +
                          (damage_add * hit_damage * 0.05F)) *
                         weight * 1.4F) + 18.0F) *
                       (knockback_scale * 0.01F)) + knockback_base) *
                     sNDSFTCommonDataHandicapTable[attack_handicap - 1][0]) *
                    sNDSFTCommonDataHandicapTable[defend_handicap - 1][1];
    }
    if (knockback >= 2500.0F)
    {
        knockback = 2500.0F;
    }
    if ((gSCManagerBackupData.error_flags & LBBACKUP_ERROR_RANDOMKNOCKBACK) !=
        0u)
    {
        knockback = syUtilsRandFloat() * 200.0F;
    }
    return knockback;
}

/* decomp ftcommontarucann.c:97-116. The throw descriptor is read out of the
 * live ground data rather than from a port constant: map base plus the map
 * head offset gives the file, and llGRJungleMapTaruCannThrowHitDesc (0xBC in
 * decomp reloc_data.us.h:3933) is where the cannon's damage, angle and
 * knockback live inside it. */
void ftCommonTaruCannShootFighter(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    FTThrowHitDesc *tarucann;
    DObj *fighter_root;
    f32 knockback;
    f32 rotate;
    s32 angle;

    if ((fp == NULL) || (gMPCollisionGroundData == NULL))
    {
        return;
    }
    tarucann = (FTThrowHitDesc *)(((uintptr_t)gMPCollisionGroundData -
                                   (uintptr_t)0x14u) + (uintptr_t)0xbcu);

    fighter_root = DObjGetStruct(fighter_gobj);
    if (fighter_root != NULL)
    {
        fighter_root->translate.vec.f.z = 0.0F;
    }

    knockback = ftParamGetGroundHazardKnockback(
        fp->percent_damage, tarucann->damage, tarucann->damage,
        tarucann->knockback_weight, tarucann->knockback_scale,
        tarucann->knockback_base, fp->attr->weight, 9, 9);

    rotate = grJungleTaruCannGetRotate();
    angle = (s32)((F_CLC_RTOD32(rotate) * -fp->lr) + 90.0F);
    angle -= (angle / 360) * 360;

    ftCommonDamageInitDamageVars(fighter_gobj, nFTCommonStatusDamageFlyRoll,
                                 tarucann->damage, knockback, angle, fp->lr, 0,
                                 tarucann->element, 0, TRUE, TRUE, FALSE);
    NDS_DIAG(gNdsTaruCannLaunchCount++);
    gNdsTaruCannLaunchAngle = angle;
    gNdsTaruCannLaunchRotate = rotate;
    gNdsTaruCannLaunchKnockback = knockback;
    ftParamUpdate1PGameDamageStats(fp, GMCOMMON_PLAYERS_MAX,
                                   nFTHitLogObjectGround,
                                   nGMHitEnvironmentTaruCann, 0, 0);

    fp->playertag_wait = 0;
    fp->tarucann_wait = NDS_FTCOMMON_TARUCANN_PICKUP_WAIT;
}

/* decomp ftcommontarucann.c:8-33. */
void ftCommonTaruCannProcUpdate(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp == NULL)
    {
        return;
    }
    if (fp->status_vars.common.tarucann.shoot_wait != 0)
    {
        fp->status_vars.common.tarucann.shoot_wait--;

        if (fp->status_vars.common.tarucann.shoot_wait ==
            (NDS_FTCOMMON_TARUCANN_SHOOT_WAIT / 2))
        {
            (void)func_800269C0_275C0(nSYAudioFGMJungleTaruCannShoot);
        }
        if (fp->status_vars.common.tarucann.shoot_wait == 0)
        {
            ftCommonTaruCannShootFighter(fighter_gobj);
            return;
        }
    }
    fp->status_vars.common.tarucann.release_wait++;

    if ((fp->status_vars.common.tarucann.release_wait >=
         NDS_FTCOMMON_TARUCANN_RELEASE_WAIT) &&
        (fp->status_vars.common.tarucann.shoot_wait == 0))
    {
        NDS_DIAG(gNdsTaruCannFireAutoCount++);
        fp->status_vars.common.tarucann.shoot_wait =
            NDS_FTCOMMON_TARUCANN_SHOOT_WAIT;

        grJungleTaruCannAddAnimShoot(
            fp->status_vars.common.tarucann.tarucann_gobj);
    }
}

/* decomp ftcommontarucann.c:37-47. Either face button fires early. */
void ftCommonTaruCannProcInterrupt(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp == NULL)
    {
        return;
    }
    if ((fp->status_vars.common.tarucann.shoot_wait == 0) &&
        ((fp->input.pl.button_tap &
          (fp->input.button_mask_a | fp->input.button_mask_b)) != 0))
    {
        NDS_DIAG(gNdsTaruCannFireInputCount++);
        fp->status_vars.common.tarucann.shoot_wait =
            NDS_FTCOMMON_TARUCANN_SHOOT_WAIT;

        grJungleTaruCannAddAnimShoot(
            fp->status_vars.common.tarucann.tarucann_gobj);
    }
}
#endif /* NDS_P2_STAGE_JUNGLE */

void ftCommonCaptureTrappedInitBreakoutVars(FTStruct *fp, s32 breakout_wait)
{
    if (fp != NULL)
    {
        fp->breakout_wait = breakout_wait;
        fp->breakout_lr = 0;
        fp->breakout_ud = 0;
    }
}

sb32 ftCommonCaptureTrappedUpdateBreakoutVars(FTStruct *fp)
{
    sb32 is_mash = FALSE;
    s32 breakout_lr_prev;
    s32 breakout_ud_prev;

    if (fp == NULL)
    {
        return FALSE;
    }

    if (((fp->input.pl.button_tap & fp->input.button_mask_a) != 0) ||
        ((fp->input.pl.button_tap & fp->input.button_mask_b) != 0) ||
        ((fp->input.pl.button_tap & fp->input.button_mask_z) != 0))
    {
        is_mash = TRUE;
        fp->breakout_wait--;
    }

    breakout_lr_prev = fp->breakout_lr;
    breakout_ud_prev = fp->breakout_ud;

    if (fp->input.pl.stick_range.x < -FTCOMMON_CAPTURE_MASH_STICK_RANGE_MIN)
    {
        fp->breakout_lr = -1;
    }
    if (fp->input.pl.stick_range.x > FTCOMMON_CAPTURE_MASH_STICK_RANGE_MIN)
    {
        fp->breakout_lr = +1;
    }
    if (fp->input.pl.stick_range.y < -FTCOMMON_CAPTURE_MASH_STICK_RANGE_MIN)
    {
        fp->breakout_ud = -1;
    }
    if (fp->input.pl.stick_range.y > FTCOMMON_CAPTURE_MASH_STICK_RANGE_MIN)
    {
        fp->breakout_ud = +1;
    }
    if ((fp->breakout_lr != breakout_lr_prev) ||
        (fp->breakout_ud != breakout_ud_prev))
    {
        is_mash = TRUE;
        fp->breakout_wait--;
    }
    return is_mash;
}

extern s32 syUtilsRandIntRange(s32 range);
extern sb32 itMainCheckShootNoAmmo(GObj *item_gobj);

void ftParamUpdateDamage(FTStruct *fp, s32 damage)
{
    if (fp != NULL)
    {
        s32 percent_before;
        /* Remix hooks this function's head on a character id (Marth's
         * counter, MarthDSP.detection_patch_). */
        const NDSP4Overrides *p4 = ndsP4Overrides(fp);

        if ((p4 != NULL) && (p4->update_damage != NULL))
        {
            damage = p4->update_damage(fp, damage);
        }
        percent_before = fp->percent_damage;
        if (((gNdsFighterNaturalMovesetPhase == 13u) ||
             (gNdsFighterNaturalMovesetPhase == 14u)) &&
            (gNdsFighterNaturalMovesetThrowDamageAfter == 0u) &&
            (gNdsFighterNaturalMovesetThrowDamageBefore == 0u))
        {
            gNdsFighterNaturalMovesetThrowDamageBefore =
                (u32)percent_before;
        }
        fp->percent_damage += damage;
        if (gSCManagerBattleState != NULL)
        {
            gSCManagerBattleState->players[fp->player].total_damage_all +=
                damage;
            if (fp->percent_damage > 999)
            {
                fp->percent_damage = 999;
            }
            gSCManagerBattleState->players[fp->player].stock_damage_all =
                fp->percent_damage;
        }
        if (((gNdsFighterNaturalMovesetPhase == 13u) ||
             (gNdsFighterNaturalMovesetPhase == 14u)) &&
            (damage > 0) &&
            ((u32)fp->percent_damage >
                gNdsFighterNaturalMovesetThrowDamageAfter))
        {
            gNdsFighterNaturalMovesetThrowDamageAfter =
                (u32)fp->percent_damage;
        }
        /* decomp ft/ftparam.c:1539-1553: a knockback hit knocks the held
         * item loose with probability damage/60 (a spent shooter on a coin
         * flip as well); Donkey Kong keeps a heavy item, which his damage
         * status settles itself (ftcommondamage.c:816-830). */
        if (fp->item_gobj != NULL)
        {
            if ((fp->damage_knockback != 0.0F) &&
                ((fp->hitlag_tics == 0) || !(fp->is_knockback_paused) ||
                 !(fp->damage_knockback <
                   (fp->damage_knockback_stack + 30.0F))))
            {
                ITStruct *ip = itGetStruct(fp->item_gobj);

                if ((ip->weight != nITWeightHeavy) ||
                    ((fp->fkind != nFTKindDonkey) &&
                     (fp->fkind != nFTKindNDonkey) &&
                     (fp->fkind != nFTKindGDonkey)))
                {
                    if ((damage > syUtilsRandIntRange(60)) ||
                        ((itMainCheckShootNoAmmo(fp->item_gobj) != FALSE) &&
                         (syUtilsRandIntRange(2) == 0)))
                    {
                        ftSetupDropItem(fp);
                    }
                }
            }
        }
    }
}

void ftParamUpdatePlayerBattleStats(s32 attack_player, s32 defend_player,
                                    s32 attack_damage)
{
    if ((gSCManagerBattleState != NULL) &&
        (attack_player != GMCOMMON_PLAYERS_MAX) &&
        (attack_player != defend_player))
    {
        gSCManagerBattleState->players[attack_player].total_damage_given +=
            attack_damage;
        gSCManagerBattleState->players[defend_player].
            total_damage_players[attack_player] += attack_damage;
        gSCManagerBattleState->players[defend_player].combo_damage_foe +=
            attack_damage;
        gSCManagerBattleState->players[defend_player].combo_count_foe++;
    }
}

void ftParamUpdateStaleQueue(s32 attack_player, s32 defend_player,
                             s32 attack_id, u16 motion_count)
{
    if ((gSCManagerBattleState != NULL) &&
        (attack_player != (s32)ARRAY_COUNT(gSCManagerBattleState->players)) &&
        (attack_player != defend_player) &&
        (attack_player >= 0) &&
        (attack_player < (s32)ARRAY_COUNT(gSCManagerBattleState->players)))
    {
        s32 i;
        s32 stale_id = gSCManagerBattleState->players[attack_player].stale_id;

        /* BattleShip ftparam.c:1639-1665. The queue is per ATTACKER and scans
         * the whole source-sized history before inserting, so four-way hits
         * must not share a two-fighter/global approximation. */
        for (i = 0;
             i < (s32)ARRAY_COUNT(
                 gSCManagerBattleState->players[attack_player].stale_info);
             i++)
        {
            if ((attack_id == gSCManagerBattleState->players[attack_player]
                                  .stale_info[i].attack_id) &&
                (motion_count == gSCManagerBattleState->players[attack_player]
                                     .stale_info[i].motion_count))
            {
                goto proof_record;
            }
        }
        gSCManagerBattleState->players[attack_player]
            .stale_info[stale_id].attack_id = attack_id;
        gSCManagerBattleState->players[attack_player]
            .stale_info[stale_id].motion_count = motion_count;

        if (stale_id ==
            ((s32)ARRAY_COUNT(
                 gSCManagerBattleState->players[attack_player].stale_info) - 1))
        {
            gSCManagerBattleState->players[attack_player].stale_id = 0;
        }
        else
        {
            gSCManagerBattleState->players[attack_player].stale_id = stale_id + 1;
        }
    }

proof_record:
}

void ftParamVelDamageTransferGround(FTStruct *fp)
{
    /* BattleShip ftparam.c:1426-1448.  This is gameplay state, not a proof
     * recorder: DownBounce and the passive/tech statuses call this after
     * becoming grounded so the existing air damage velocity is projected onto
     * the current floor tangent.  The old DS seam kept only the diagnostics
     * below, which left vel_damage_ground at zero and changed subsequent
     * grounded knockback for every live fighter. */
    if (fp->ga == nMPKineticsGround)
    {
        Vec3f *floor_angle = &fp->coll_data.floor_angle;

        if (fp->physics.vel_damage_ground == 0.0F)
        {
            fp->physics.vel_damage_ground = fp->physics.vel_damage_air.x;

            if (fp->physics.vel_damage_ground > 250.0F)
            {
                fp->physics.vel_damage_ground = 250.0F;
            }
            if (fp->physics.vel_damage_ground < -250.0F)
            {
                fp->physics.vel_damage_ground = -250.0F;
            }
            fp->physics.vel_damage_air.x =
                floor_angle->y * fp->physics.vel_damage_ground;
            fp->physics.vel_damage_air.y =
                -floor_angle->x * fp->physics.vel_damage_ground;
        }
    }

}

static void ndsFTParamGetVisualPosition(GObj *fighter_gobj, s32 joint_id,
                                        Vec3f *effect_pos,
                                        Vec3f *effect_scatter,
                                        sb32 is_scale_pos, Vec3f *pos)
{
    FTStruct *fp = (fighter_gobj != NULL) ? ftGetStruct(fighter_gobj) : NULL;
    DObj *root = (fighter_gobj != NULL) ? DObjGetStruct(fighter_gobj) : NULL;

    pos->x = (root != NULL) ? root->translate.vec.f.x : 0.0F;
    pos->y = (root != NULL) ? root->translate.vec.f.y : 0.0F;
    pos->z = (root != NULL) ? root->translate.vec.f.z : 0.0F;
    if ((fp == NULL) || (joint_id < 0) ||
        (joint_id >= (s32)ARRAY_COUNT(fp->joints)) ||
        (fp->joints[joint_id] == NULL))
    {
        if (effect_pos != NULL)
        {
            pos->x += effect_pos->x;
            pos->y += effect_pos->y;
            pos->z += effect_pos->z;
        }
        return;
    }
    if (effect_pos != NULL)
    {
        *pos = *effect_pos;
    }
    else
    {
        pos->x = pos->y = pos->z = 0.0F;
    }
    if (effect_scatter != NULL)
    {
        if (effect_scatter->x != 0.0F)
        {
            pos->x += (syUtilsRandFloat() - 0.5F) *
                      (effect_scatter->x * 2.0F);
        }
        if (effect_scatter->y != 0.0F)
        {
            pos->y += (syUtilsRandFloat() - 0.5F) *
                      (effect_scatter->y * 2.0F);
        }
        if (effect_scatter->z != 0.0F)
        {
            pos->z += (syUtilsRandFloat() - 0.5F) *
                      (effect_scatter->z * 2.0F);
        }
    }
    if ((is_scale_pos != FALSE) && (fp->attr != NULL) &&
        (fp->attr->size != 0.0F))
    {
        f32 inverse_size = 1.0F / fp->attr->size;

        pos->x *= inverse_size;
        pos->y *= inverse_size;
        pos->z *= inverse_size;
    }
    gmCollisionGetFighterPartsWorldPosition(fp->joints[joint_id], pos);
}

/* Verbatim from decomp/src/ef/efmanager.h, which this TU does not include.
 * Each is defined by the decomp efmanager.c that src/import/battleship_efmanager.c
 * compiles, unwrapped -- see ndsFTParamMakeSourceEffect below. */
extern LBParticle *efManagerDustLightMakeEffect(Vec3f *pos, s32 lr,
                                                f32 f_index);
extern LBParticle *efManagerDustHeavyMakeEffect(Vec3f *pos, s32 lr);
extern LBParticle *efManagerDustHeavyDoubleMakeEffect(Vec3f *pos, s32 lr,
                                                      f32 f_index);
extern LBParticle *efManagerDustExpandLargeMakeEffect(Vec3f *pos);
extern LBParticle *efManagerDustDashMakeEffect(Vec3f *pos, s32 lr, f32 scale);
extern LBParticle *efManagerSparkleWhiteMultiExplodeMakeEffect(Vec3f *pos);
extern LBParticle *efManagerFuraSparkleMakeEffect(Vec3f *pos);
extern LBParticle *efManagerMusicNoteMakeEffect(Vec3f *pos);
/* These five ALSO have weak stand-in definitions further down this file, which
 * NDS_R2_SOURCE_EFFECTS_PARTICLE overrides from battleship_efmanager.c. The
 * attribute has to appear here, ahead of the calls below -- a plain
 * declaration followed by a weak definition after first use is unspecified
 * behaviour, and GCC says so. */
extern __attribute__((weak)) LBParticle *
efManagerDustExpandSmallMakeEffect(Vec3f *pos, f32 f_index);
extern __attribute__((weak)) LBParticle *
efManagerSparkleWhiteMakeEffect(Vec3f *pos);
extern __attribute__((weak)) LBParticle *
efManagerSparkleWhiteScaleMakeEffect(Vec3f *pos, f32 scale);
extern LBParticle *ndsEFManagerChargeSparkleMakeEffect(Vec3f *pos);
extern __attribute__((weak)) LBParticle *
efManagerFlashMiddleMakeEffect(Vec3f *pos);
/* The source makers of the kinds that used to fall to a stand-in below (BUGS.md
 * 1P, owner 2026-10-06: "Generic yellow/green/green ring VFX still played for
 * some effects. Need to audit against source"). Every one starts an efcommon
 * bank script that the generator has packed since 2026-10-04, when it began
 * refusing an unlisted public maker (scripts/generate_nds_particle_banks.py). */
extern LBParticle *efManagerFlashSmallMakeEffect(Vec3f *pos);
extern LBParticle *efManagerFlashLargeMakeEffect(Vec3f *pos);
extern LBGenerator *efManagerRippleMakeEffect(Vec3f *pos);
extern LBParticle *efManagerPsionicMakeEffect(Vec3f *pos);
extern LBGenerator *efManagerKirbyStarMakeEffect(Vec3f *pos);
extern LBParticle *efManagerSparkleWhiteMultiMakeEffect(Vec3f *pos);
extern LBParticle *efManagerHealSparklesMakeEffect(Vec3f *pos);
extern LBParticle *efManagerEggBreakMakeEffect(Vec3f *pos);
extern __attribute__((weak)) LBParticle *
efManagerDamageNormalLightMakeEffect(Vec3f *pos, s32 player, s32 size,
                                     sb32 is_static);

/* THE PARTICLE-ONLY HALF OF THE SOURCE DISPATCH (ftparam.c:1892-2112).
 *
 * ftParamMakeEffect is the seam every motion-script effect command arrives at
 * -- ftmain.c:447 for nFTMotionEventEffect, ftmain.c:1139 for the collision
 * variant -- and it is how a fighter asks for running dust, landing dust,
 * flashes, sparkles and flame. The port's own switch below answers all of them
 * with one of seven untextured DS primitives, which is the whole of BUGS.md
 * "Correct VFX isn't played for various things (running foot dust VFX, ...)".
 * Mario and Fox request effects from it 178 times across their two motion
 * files and 106 of those are dust
 * (scripts/check-battle-playable-attack-effects.ps1).
 *
 * The real makers were here the entire time. Every one of them is defined in
 * the decomp efmanager.c that src/import/battleship_efmanager.c includes, and
 * unlike the thirty in that file's rename block they were never wrapped, so
 * they compile with their source bodies and --gc-sections was dropping them
 * only because nothing called them.
 *
 * WHAT IS NOT HERE IS DELIBERATE, for two separate reasons.
 *
 * These fall through because their source maker builds a DObj tree whose
 * geometry goes out as source effect display-list links, which the battle
 * hardware path does not submit -- routing them would trade a visible
 * primitive for nothing at all:
 *   ShockSmall, DamageFlyOrbs/Sparks/MDust, FireSpark, StarRodSpark
 * Same seam that kept the respawn platform invisible. Move a kind up here when
 * that path lands, not before.
 *
 * IMPACTWAVE LEFT THIS LIST ON 2026-08-02, and how that was settled matters
 * more than the kind itself. The exclusion rule was applied to it by reading
 * the SOURCE maker, which does build a DObj tree over effect DL links. But this
 * port OVERRIDES efManagerImpactWaveMakeEffect with a substitute drawing
 * nNDSVisualEffectImpactWave, a DL-link-18 visual-effect template the hardware
 * path does submit -- so the rule's premise was false for this kind, and
 * falling through is what cost the hard landing its shockwave while its dust
 * played. Check for a port override before assuming a kind is blocked here.
 *
 * ROUTING IT THEN "FAILED" THE DETAIL GATE, AND THAT WAS NOISE. The first run
 * threw "Horizontal detail region left_bush variation 22.379% is below required
 * 40.000%", which reads exactly like a 162-unit opaque disc flattening scenery,
 * and it was nearly reverted on that single arm. The owner asked whether their
 * own interaction with the emulator window had caused it. Re-running the SAME
 * change passed. assert-melonds-horizontal-detail samples a screenshot region,
 * and capture runs on an interactive desktop (docs/VERIFYING.md), so a
 * foregrounded window or a fighter standing in that region perturbs it. One
 * failing arm of a screenshot gate is a measurement, not a verdict: re-run the
 * candidate before believing it, exactly as an A/B would.
 *
 * And these fall through because P1 cannot request them, while linking their
 * maker would still cost .text -- which the taskman arena charges one-for-one
 * against the general heap, the scarcest thing this change set spends:
 *   DamageNormal, FlameLR/Random/Static, SparkleWhiteMulti, ChargeSparkle,
 *   HealSparkles, FlashSmall/Large, Psionic, ThunderAmp
 * Mario's and Fox's two motion files request seventeen distinct kinds
 * (scripts/check-battle-playable-attack-effects.ps1) and none of those are
 * among them; they belong to Ness, Samus, Kirby, Pikachu and Mewtwo. Each is
 * three lines from ftparam.c when its fighter lands. Adding one back also
 * needs ftParamGetEffectJointPosition (ftparam.c:1783) for the four that
 * rotate through attr->effect_joint_ids.
 *
 * Position is already resolved by ndsFTParamGetVisualPosition, which is the
 * source's own scatter / inverse-size / gmCollisionGetFighterPartsWorldPosition
 * arithmetic, so these cases are the source's right-hand sides verbatim. */
/* BUGS.md fire burn, second half. These live only inside the imported
 * efmanager.c (2512/2579/2643) and are declared in no port header, so they are
 * declared here at their single point of use. Until now nothing referenced
 * them and --gc-sections dropped them entirely -- the linked ELF carried no
 * Flame symbol at all -- which is why routing to them is what makes them
 * exist, not merely what makes them run. */
extern LBParticle *efManagerFlameLRMakeEffect(Vec3f *pos, s32 lr);
extern LBParticle *efManagerFlameRandomMakeEffect(Vec3f *pos);
extern LBParticle *efManagerFlameStaticMakeEffect(Vec3f *pos);


static sb32 ndsFTParamMakeSourceEffect(s32 effect_id, s32 lr, Vec3f *pos,
                                       void **effect)
{
    switch (effect_id)
    {
    /* The fighter fire burn. The colanim scripts restored above issue FlameLR
     * and FlameRandom; taking the real makers here is what stops them
     * collapsing onto the generic HitFire burst in the substitute switch
     * below. Bank script 0x12 (FlameLR) and 0x55 (FlameRandom/Static) are
    * packed for exactly this. */
    case nEFKindFlameLR:
        *effect = efManagerFlameLRMakeEffect(pos, lr);
        return TRUE;
    case nEFKindFlameRandom:
        *effect = efManagerFlameRandomMakeEffect(pos);
        return TRUE;
    case nEFKindFlameStatic:
        *effect = efManagerFlameStaticMakeEffect(pos);
        return TRUE;
    case nEFKindDustLight:
        *effect = efManagerDustLightMakeEffect(pos, lr, 1.0F);
        return TRUE;
    case nEFKindDustLightRapid:
        *effect = efManagerDustLightMakeEffect(pos, lr, 2.0F);
        return TRUE;
    case nEFKindDustHeavyDouble:
        *effect = efManagerDustHeavyDoubleMakeEffect(pos, lr, 1.0F);
        return TRUE;
    case nEFKindDustHeavyDoubleRapid:
        *effect = efManagerDustHeavyDoubleMakeEffect(pos, lr, 1.7F);
        return TRUE;
    /* The hard landing's shockwave -- see the ImpactWave paragraph above for
     * why this is not blocked on the DL-links seam. The source branches on
     * ground vs air and passes the floor angle so the ring lies along the
     * slope (ftparam.c:1966); this seam has no `fp`, so both arms take the
     * substitute at rotate 0. Dream Land's surfaces under a hard landing are
     * flat, so the angle term is zero there; thread it when a sloped stage
     * lands rather than widening this signature now. */
    case nEFKindImpactWave:
        *effect = efManagerImpactWaveMakeEffect(pos, 4, 0.0F);
        return TRUE;
    /* Pikachu's Thunder self-hit burst. efdef.h names the kind outright --
     * "Pikachu's Thunder self-hit" -- and the source Hit motion script
     * dPikachuMainMotion_GettingThundered_0x1668 issues it when the descending
     * head reaches Pikachu. ftparam.c answers it with this maker, which starts
     * bank script 0x74.
     *
     * The substitute switch below used to swallow it into the generic
     * HitElectric sprite shared with ShockSmall and Psionic, which is the whole
     * of BUGS.md "Down-B blue self-hit explosion missing": the burst was not
     * absent, it was replaced by a small generic spark. With no caller the
     * maker was also dropped by --gc-sections, and the particle-bank seam
     * derivation -- which reads efmanager.c for reachable makers -- never
     * seeded 0x74, so the script was UNREACHABLE as well. Both halves are
     * needed; generate_nds_particle_banks.py carries the other. */
    case nEFKindThunderAmp:
        *effect = efManagerThunderAmpMakeEffect(pos);
        return TRUE;
    case nEFKindDustHeavy:
        *effect = efManagerDustHeavyMakeEffect(pos, lr);
        return TRUE;
    case nEFKindDustHeavyReverse:
        *effect = efManagerDustHeavyMakeEffect(pos, -lr);
        return TRUE;
    case nEFKindDustExpandLarge:
        pos->x += ((syUtilsRandFloat() * 160.0F) - 80.0F);
        pos->y += ((syUtilsRandFloat() * 160.0F) - 80.0F);
        *effect = efManagerDustExpandLargeMakeEffect(pos);
        return TRUE;
    case nEFKindDustExpandSmall:
        *effect = efManagerDustExpandSmallMakeEffect(pos, 1.0F);
        return TRUE;
    case nEFKindDustDashSmall:
        *effect = efManagerDustDashMakeEffect(pos, lr, 1.0F);
        return TRUE;
    case nEFKindDustDashLarge:
        *effect = efManagerDustDashMakeEffect(pos, lr, 1.5F);
        return TRUE;
    case nEFKindSparkleWhite:
        *effect = efManagerSparkleWhiteMakeEffect(pos);
        return TRUE;
    case nEFKindSparkleWhiteMultiExplode:
        *effect = efManagerSparkleWhiteMultiExplodeMakeEffect(pos);
        return TRUE;
    case nEFKindSparkleWhiteScale:
        *effect = efManagerSparkleWhiteScaleMakeEffect(pos, 1.0F);
        return TRUE;
    case nEFKindChargeSparkle:
        *effect = ndsEFManagerChargeSparkleMakeEffect(pos);
        return TRUE;
    case nEFKindFuraSparkle:
        *effect = efManagerFuraSparkleMakeEffect(pos);
        return TRUE;
    case nEFKindFlashMiddle:
        *effect = efManagerFlashMiddleMakeEffect(pos);
        return TRUE;
    /* The source's expanding green circles (efdef.h: FlashSmall "plays when
     * teching", FlashLarge "large expanding green circle") and the Ripple of
     * Pikachu's Quick Attack and Ness's PSI Magnet. All three were answered by
     * the generic green/yellow Wave ring below -- the owner's "generic ring"
     * report (2026-10-06). The real makers draw from the global random stream
     * like every particle script, as on N64, so a replay that reaches one of
     * these kinds re-baselines its digest. */
    case nEFKindFlashSmall:
        *effect = efManagerFlashSmallMakeEffect(pos);
        return TRUE;
    case nEFKindFlashLarge:
        *effect = efManagerFlashLargeMakeEffect(pos);
        return TRUE;
    case nEFKindRipple:
        *effect = efManagerRippleMakeEffect(pos);
        return TRUE;
    /* Same audit: Ness's PSI sparkles (was the electric sprite), Kirby's
     * landing star (was nothing at all), the white multi-sparkle and the heal
     * sparkles (were the generic star), the egg shell (was a dust puff). */
    case nEFKindPsionic:
        *effect = efManagerPsionicMakeEffect(pos);
        return TRUE;
    case nEFKindKirbyStar:
        *effect = efManagerKirbyStarMakeEffect(pos);
        return TRUE;
    case nEFKindSparkleWhiteMulti:
        *effect = efManagerSparkleWhiteMultiMakeEffect(pos);
        return TRUE;
    case nEFKindHealSparkles:
        *effect = efManagerHealSparklesMakeEffect(pos);
        return TRUE;
    case nEFKindEggBreak:
        efManagerEggBreakMakeEffect(pos);
        *effect = NULL;
        return TRUE;
    case nEFKindMusicNote:
        efManagerMusicNoteMakeEffect(pos);
        *effect = NULL;
        return TRUE;
    default:
        return FALSE;
    }
}

/* BUGS.md "Fighter burn flames spawn at the wrong places on the damaged
 * fighter." ftparam.c:1899-1912 has all three Flame kinds DISCARD the generic
 * effect position and call ftParamGetEffectJointPosition (ftparam.c:1783),
 * which advances fp->effect_joint_array_id, wraps at
 * ARRAY_COUNT(attr->effect_joint_ids), and resolves local {0,0,0} at the
 * selected joint. The burn is MEANT to walk the body; flames sharing one origin
 * is the defect, not the fix.
 *
 * This port had no such call, and the measurement (c131, 2026-08-12) found the
 * damage worse than a missing rotation. The position the shipped code actually
 * used read (1039.04, 0.000000, 0.000000) on every emission of a burn -- Y and Z
 * EXACTLY zero -- because ndsFTParamGetVisualPosition takes its early return
 * when the colanim's joint_id does not resolve and hands back the fighter's
 * root translate. Every flame spawned on the stage plane at the victim's feet.
 * The five source joints measured healthy over the same burn: ids 15, 20, 25, 9
 * and 12, Y from 110 to 338 and Z from -104 to +128.
 *
 * Source-exact, and deliberately placed in ftParamMakeEffect rather than in the
 * maker switch: ftParamMakeEffect already holds fighter_gobj, so no signature
 * widens and no global is needed, and the override then also covers the
 * NDS_R2_SOURCE_EFFECTS_PARTICLE=0 substitute arm -- which the source's single
 * path implies and which a fix inside ndsFTParamMakeSourceEffect would miss. */
static void ndsFTParamGetEffectJointPosition(FTStruct *fp, Vec3f *pos)
{
    FTAttributes *attr = fp->attr;
    s32 joint_id;

    fp->effect_joint_array_id++;

    if (fp->effect_joint_array_id == ARRAY_COUNT(attr->effect_joint_ids))
    {
        fp->effect_joint_array_id = 0;
    }
    pos->x = pos->y = pos->z = 0.0F;

    /* The source indexes fp->joints[] unguarded. This port keeps the range
     * check because an attribute table that has not been relocated yet reads as
     * a wild id here, and the failure that produces is a wild pointer inside
     * gmCollisionGetFighterPartsWorldPosition rather than a visible one. On a
     * relocated fighter the check never fires and the behaviour is the
     * source's. */
    joint_id = attr->effect_joint_ids[fp->effect_joint_array_id];
    if ((joint_id < 0) || (joint_id >= (s32)ARRAY_COUNT(fp->joints)) ||
        (fp->joints[joint_id] == NULL))
    {
        return;
    }
    gmCollisionGetFighterPartsWorldPosition(fp->joints[joint_id], pos);
}


void *ftParamMakeEffect(GObj *fighter_gobj, s32 effect_id, s32 joint_id,
                        Vec3f *effect_pos, Vec3f *effect_scatter, s32 lr,
                        sb32 is_scale_pos, u32 arg7)
{
    Vec3f pos;
    Vec3f charge_effect_pos;
    GObj *effect_gobj = NULL;

    (void)arg7;
    /* BUGS.md "Missing fire burn effects": decide whether the Flame family is
     * reachable in P1 before adding it to P1_PARTICLE_SEAMS. The alias below
     * collapses FlameLR/FlameRandom/FlameStatic AND FireSpark onto one
     * substitute, and only FireSpark is currently a listed seam, so a single
     * combined counter could not tell "the burn is requested and substituted"
     * from "only FireSpark ever arrives". Count them apart. */
    /* Enumerate WHICH effect kinds the fire path actually requests, instead of
     * testing one hypothesis per build. 128 bits as four readable scalars. */
    if (((u32)effect_id) < 128u)
    {
        u32 bit = 1u << (((u32)effect_id) & 31u);
        switch (((u32)effect_id) >> 5)
        {
        case 0u: gNdsFighterEffectKindMask0 |= bit; break;
        case 1u: gNdsFighterEffectKindMask1 |= bit; break;
        case 2u: gNdsFighterEffectKindMask2 |= bit; break;
        default: gNdsFighterEffectKindMask3 |= bit; break;
        }
    }
    if ((gNdsFighterSpecialsProofPhase == 3u) &&
        (effect_id == nEFKindDustLight))
    {
        gNdsFighterSpecialsMarioLwDustEffectCount++;
    }
    /* BattleShip ftparam.c:1812-1854 gives the common full-charge sparkle a
     * fighter-specific hand joint and local X offset before the generic effect
     * position resolver runs. The colanim script intentionally names joint 0;
     * this override is the source contract that moves the sparkle to the hand. */
    if ((effect_id == nEFKindChargeSparkle) && (fighter_gobj != NULL))
    {
        FTStruct *charge_fp = ftGetStruct(fighter_gobj);

        if (charge_fp != NULL)
        {
            charge_effect_pos.y = 0.0F;
            charge_effect_pos.z = 0.0F;
            switch (charge_fp->fkind)
            {
            case nFTKindSamus:
                joint_id = 16;
                charge_effect_pos.x = 180.0F;
                effect_pos = &charge_effect_pos;
                break;
            case nFTKindDonkey:
            case nFTKindGDonkey:
                joint_id = 16;
                charge_effect_pos.x = 100.0F;
                effect_pos = &charge_effect_pos;
                break;
            case nFTKindKirby:
                joint_id = 16;
                charge_effect_pos.x = 50.0F;
                effect_pos = &charge_effect_pos;
                break;
            default:
                break;
            }
        }
    }
    ndsFTParamGetVisualPosition(fighter_gobj, joint_id, effect_pos,
                                effect_scatter, is_scale_pos, &pos);
    /* The source's Flame override, ahead of BOTH dispatch arms. See
     * ndsFTParamGetEffectJointPosition for why it lives here and what the
     * generic position it replaces was measured to be. */
    if ((effect_id == nEFKindFlameLR) || (effect_id == nEFKindFlameRandom) ||
        (effect_id == nEFKindFlameStatic))
    {
        FTStruct *flame_fp =
            (fighter_gobj != NULL) ? ftGetStruct(fighter_gobj) : NULL;

        if ((flame_fp != NULL) && (flame_fp->attr != NULL))
        {
            ndsFTParamGetEffectJointPosition(flame_fp, &pos);
        }
    }
    /* ftparam.c:1894: the normal hit spark at the source's fixed size 10. Its
     * bank script is picked by the requesting fighter's player
     * (dEFManagerDamageNormalLightIDs), which the seam below does not carry;
     * the generic HitNormal star stood in for it. */
    if ((effect_id == nEFKindDamageNormal) && (fighter_gobj != NULL))
    {
        return efManagerDamageNormalLightMakeEffect(
            &pos, ftGetStruct(fighter_gobj)->player, 10, FALSE);
    }
    {
        void *source_effect = NULL;

        if (ndsFTParamMakeSourceEffect(effect_id, lr, &pos,
                                       &source_effect) != FALSE)
        {
            return source_effect;
        }
    }
    switch (effect_id)
    {
    case nEFKindDamageNormal:
        effect_gobj = ndsEFManagerMakeVisualEffect(
            nNDSVisualEffectHitNormal, &pos, 0.8F, lr, NULL);
        break;
    /* The three Flame kinds now take their real makers above and never reach
     * here at NDS_R2_SOURCE_EFFECTS_PARTICLE=1. They are NOT dead: the flag is
     * switchable to 0 for attribution A/Bs, and that arm has no other route to
     * a fire burn. Deleting them would silently drop the burn from the arm
     * least likely to be looked at. */
    case nEFKindFlameLR:
    case nEFKindFlameRandom:
    case nEFKindFlameStatic:
    case nEFKindFireSpark:
        effect_gobj = ndsEFManagerMakeVisualEffect(
            nNDSVisualEffectHitFire, &pos, 0.7F, lr, NULL);
        break;
    case nEFKindShockSmall:
    case nEFKindPsionic:
    /* ThunderAmp takes its real maker above at
     * NDS_R2_SOURCE_EFFECTS_PARTICLE=1 and never reaches here. Kept for the
     * same reason the Flame kinds are: the flag is switchable to 0 for
     * attribution A/Bs, and that arm has no other route to a self-hit burst. */
    case nEFKindThunderAmp:
        effect_gobj = ndsEFManagerMakeVisualEffect(
            nNDSVisualEffectHitElectric, &pos, 0.7F, lr, NULL);
        break;
    case nEFKindDustLight:
    case nEFKindDustLightRapid:
        effect_gobj = ndsEFManagerMakeVisualEffect(
            nNDSVisualEffectDust, &pos, 0.65F, lr, NULL);
        break;
    case nEFKindDustHeavyDouble:
    case nEFKindDustHeavyDoubleRapid:
        effect_gobj = ndsEFManagerMakeVisualEffect(
            nNDSVisualEffectDust, &pos, 1.0F, lr, NULL);
        pos.x += (f32)lr * 110.0F;
        (void)ndsEFManagerMakeVisualEffect(nNDSVisualEffectDust, &pos,
                                           0.8F, -lr, NULL);
        break;
    case nEFKindDustHeavy:
    case nEFKindDustHeavyReverse:
        effect_gobj = ndsEFManagerMakeVisualEffect(
            nNDSVisualEffectDust, &pos, 1.0F,
            (effect_id == nEFKindDustHeavyReverse) ? -lr : lr, NULL);
        break;
    case nEFKindDustExpandLarge:
        effect_gobj = ndsEFManagerMakeVisualEffect(
            nNDSVisualEffectDust, &pos, 1.35F, lr, NULL);
        break;
    case nEFKindDustExpandSmall:
        effect_gobj = ndsEFManagerMakeVisualEffect(
            nNDSVisualEffectDust, &pos, 0.7F, lr, NULL);
        break;
    case nEFKindDustDashSmall:
    case nEFKindDustDashLarge:
        effect_gobj = ndsEFManagerMakeVisualEffect(
            nNDSVisualEffectDust, &pos,
            (effect_id == nEFKindDustDashLarge) ? 1.25F : 0.9F,
            lr, NULL);
        break;
    case nEFKindDamageFlyOrbs:
    case nEFKindStarRodSpark:
    case nEFKindDamageFlySparks:
    case nEFKindDamageFlySparksReverse:
        effect_gobj = ndsEFManagerMakeVisualEffect(
            nNDSVisualEffectSparkle, &pos, 0.65F, lr, NULL);
        break;
    case nEFKindDamageFlyMDust:
    case nEFKindDamageFlyMDustReverse:
        effect_gobj = ndsEFManagerMakeVisualEffect(
            nNDSVisualEffectDust, &pos, 0.55F, lr, NULL);
        break;
    case nEFKindImpactWave:
    case nEFKindRipple:
        effect_gobj = ndsEFManagerMakeVisualEffect(
            nNDSVisualEffectImpactWave, &pos, 0.9F, lr, NULL);
        break;
    case nEFKindSparkleWhite:
    case nEFKindSparkleWhiteMultiExplode:
    case nEFKindSparkleWhiteMulti:
    case nEFKindSparkleWhiteScale:
    case nEFKindFuraSparkle:
    case nEFKindChargeSparkle:
    case nEFKindHealSparkles:
        effect_gobj = ndsEFManagerMakeVisualEffect(
            nNDSVisualEffectSparkle, &pos, 0.75F, lr, NULL);
        break;
    case nEFKindFlashSmall:
    case nEFKindFlashMiddle:
    case nEFKindFlashLarge:
        effect_gobj = ndsEFManagerMakeVisualEffect(
            nNDSVisualEffectImpactWave, &pos,
            (effect_id == nEFKindFlashLarge) ? 1.3F : 0.8F, lr, NULL);
        break;
    case nEFKindMusicNote:
        effect_gobj = ndsEFManagerMakeVisualEffect(
            nNDSVisualEffectCoin, &pos, 0.65F, lr, NULL);
        break;
    case nEFKindEggBreak:
        effect_gobj = ndsEFManagerMakeVisualEffect(
            nNDSVisualEffectDust, &pos, 0.8F, lr, NULL);
        break;
    case nEFKindYoshiEggEscape:
        {
            FTStruct *fp = (fighter_gobj != NULL) ?
                ftGetStruct(fighter_gobj) : NULL;

            /* BattleShip ftparam.c:2100-2105. RollF/RollB request the egg that
             * deliberately replaces Yoshi's hidden body. Keep the source
             * maker as the owner; the compat switch previously dropped this
             * kind entirely, which let the linker collect the constructor.
             * A P4 content aliasing Yoshi makes none (Remix's
             * yoshi_shield_fix_5 adds J Yoshi only). */
            if ((fp != NULL) &&
                (fp->fkind == NDS_P4_PARENT_KIND(fp, nFTKindYoshi)))
            {
                (void)efManagerYoshiEggEscapeMakeEffect(fighter_gobj);
            }
        }
        break;
    case nEFKindQuakeMag0:
        return efManagerQuakeMakeEffect(0);
    case nEFKindQuakeMag1:
        return efManagerQuakeMakeEffect(1);
    case nEFKindQuakeMag2:
        return efManagerQuakeMakeEffect(2);
    default:
        break;
    }
    return effect_gobj;
}

static f32 ndsVisualDamageScale(s32 size, f32 base, f32 step)
{
    if (size < 0)
    {
        size = 0;
    }
    else if (size > 40)
    {
        size = 40;
    }
    return base + ((f32)size * step);
}

__attribute__((weak)) LBParticle *efManagerFlashMiddleMakeEffect(Vec3f *pos)
{
    (void)ndsEFManagerMakeVisualEffect(nNDSVisualEffectImpactWave, pos,
                                       0.8F, 1, NULL);
    return NULL;
}

__attribute__((weak)) LBParticle *
efManagerSparkleWhiteScaleMakeEffect(Vec3f *pos, f32 scale)
{
    (void)ndsEFManagerMakeVisualEffect(nNDSVisualEffectSparkle, pos,
                                       scale, 1, NULL);
    return NULL;
}

__attribute__((weak)) LBParticle *
efManagerDustExpandSmallMakeEffect(Vec3f *pos, f32 f_index)
{
    (void)ndsEFManagerMakeVisualEffect(nNDSVisualEffectDust, pos,
                                       0.65F * f_index, 1, NULL);
    return NULL;
}

__attribute__((weak)) LBParticle *efManagerFireGrindMakeEffect(Vec3f *pos)
{
    (void)ndsEFManagerMakeVisualEffect(nNDSVisualEffectHitFire, pos,
                                       0.55F, 1, NULL);
    return NULL;
}

__attribute__((weak)) LBParticle *efManagerSparkleWhiteMakeEffect(Vec3f *pos)
{
    (void)ndsEFManagerMakeVisualEffect(nNDSVisualEffectSparkle, pos,
                                       0.75F, 1, NULL);
    return NULL;
}

__attribute__((weak)) LBParticle *
efManagerDamageNormalLightMakeEffect(Vec3f *pos, s32 player, s32 size,
                                     sb32 is_static)
{
    NDS_FREEZE_DIAGNOSTICS_MARK(NDS_FREEZE_BREADCRUMB_EFFECT_SPAWN);
    ndsTask39EffectCensusRecord(
        NDS_TASK39_EFFECT_EF_MANAGER_DAMAGE_NORMAL_LIGHT_MAKE_EFFECT,
        NDS_TASK39_EFFECT_SUBSTITUTE);
    ndsTask39HitSparkSpawn(pos, player, size, is_static, FALSE);
    return NULL;
}

__attribute__((weak)) LBParticle *
efManagerDamageNormalHeavyMakeEffect(Vec3f *pos, s32 player, s32 size)
{
    NDS_FREEZE_DIAGNOSTICS_MARK(NDS_FREEZE_BREADCRUMB_EFFECT_SPAWN);
    ndsTask39EffectCensusRecord(
        NDS_TASK39_EFFECT_EF_MANAGER_DAMAGE_NORMAL_HEAVY_MAKE_EFFECT,
        NDS_TASK39_EFFECT_SUBSTITUTE);
    ndsTask39HitSparkSpawn(pos, player, size, FALSE, TRUE);
    return NULL;
}

/* NOT the owner's "orange ball", and the correction is worth keeping because it
 * was asserted from memory and refuted by the source in one lookup. This seam
 * was briefly halved on the claim that Mario's forward smash is a flame attack.
 * It is not: nothing under src/ft/ftchar/ftmario sets an element field at all,
 * and Mario's ONLY fire-element source is Special N, whose fireball is a WEAPON
 * whose element comes from attr->element (wpmanager.c:197) and which reaches
 * this function through the weapon switch at ftmain.c:2770, not the fighter one
 * at ftmain.c:2715. A side-A attack is normal-element and never arrives here.
 *
 * The scale left alone at the source-derived value. The owner filed an orange
 * ball on side-A hits; normal-element hits go to ndsTask39HitSparkSpawn under
 * NDS_TASK39_FX_SPRITES, which is where that row has to be answered. */
__attribute__((weak)) LBParticle *efManagerDamageFireMakeEffect(Vec3f *pos,
                                                               s32 size)
{
    (void)ndsEFManagerMakeVisualEffect(
        nNDSVisualEffectHitFire, pos,
        ndsVisualDamageScale(size, 0.60F, 0.03F), 1, NULL);
    return NULL;
}

__attribute__((weak)) LBParticle *
efManagerDamageElectricMakeEffect(Vec3f *pos, s32 size)
{
    (void)ndsEFManagerMakeVisualEffect(
        nNDSVisualEffectHitElectric, pos,
        ndsVisualDamageScale(size, 0.55F, 0.03F), 1, NULL);
    return NULL;
}

__attribute__((weak)) LBParticle *efManagerDamageCoinMakeEffect(Vec3f *pos)
{
    (void)ndsEFManagerMakeVisualEffect(nNDSVisualEffectCoin, pos,
                                       0.75F, 1, NULL);
    return NULL;
}

__attribute__((weak)) LBParticle *efManagerSetOffMakeEffect(Vec3f *pos,
                                                           s32 size)
{
    (void)ndsEFManagerMakeVisualEffect(
        nNDSVisualEffectImpactWave, pos,
        ndsVisualDamageScale(size, 0.55F, 0.03F), 1, NULL);
    return NULL;
}

/* The real one comes with battleship_ftcommon_hammer.c, which imports
 * fthammer.c whole. */


void ftParamKirbyTryMakeMapStarEffect(GObj *fighter_gobj)
{
    (void)fighter_gobj;
}

void ftParamSetAnimLocks(FTStruct *fp)
{
    u32 flags0;
    u32 flags1;
    u32 current_flags;
    FTParts *parts;
    u32 *animlock;
    s32 i;

    if ((fp == NULL) || (fp->attr == NULL) || (fp->attr->animlock == NULL))
    {
        return;
    }
    animlock = fp->attr->animlock;
    flags0 = animlock[0];
    flags1 = animlock[1];

    for (i = nFTPartsJointCommonStart; ((flags0 != 0) || (flags1 != 0)); i++)
    {
        current_flags =
            (i < (ARRAY_COUNT(fp->joints) - 1)) ? flags0 : flags1;
        if ((current_flags & (1u << 31)) && (fp->joints[i] != NULL))
        {
            parts = fp->joints[i]->user_data.p;
            if (parts != NULL)
            {
                gmCollisionTransformMatrixAll(fp->joints[i], parts,
                                              parts->unk_dobjtrans_0x10);
                parts->transform_update_mode = 3;
                if (fp->joints[i]->xobjs[0] != NULL)
                {
                    fp->joints[i]->xobjs[0]->unk05 = 1;
                }
            }
        }
        if (i < (ARRAY_COUNT(fp->joints) - 1))
        {
            flags0 <<= 1;
        }
        else
        {
            flags1 <<= 1;
        }
    }
}

void ftParamClearAnimLocks(FTStruct *fp)
{
    FTParts *parts;
    s32 i;

    if (fp == NULL)
    {
        return;
    }
    for (i = 0; i < ARRAY_COUNT(fp->joints); i++)
    {
        if (fp->joints[i] != NULL)
        {
            parts = fp->joints[i]->user_data.p;
            if ((parts != NULL) && (parts->transform_update_mode == 3))
            {
                parts->transform_update_mode = 0;
                if (fp->joints[i]->xobjs[0] != NULL)
                {
                    fp->joints[i]->xobjs[0]->unk05 = 0;
                }
            }
        }
    }
}

void ftParamMoveDLLink(GObj *fighter_gobj, u8 dl_link)
{
    FTStruct *fp;

    if (fighter_gobj == NULL)
    {
        return;
    }
    gcMoveGObjDL(fighter_gobj, dl_link, GOBJ_PRIORITY_DEFAULT);
    fp = ftGetStruct(fighter_gobj);
    if (fp != NULL)
    {
        fp->dl_link = dl_link;
    }
}

extern void gcAddDObjAnimJoint(DObj *dobj, AObjEvent32 *anim_joint,
                               f32 anim_frame);
extern void *ndsRelocResolvePointerFromFileBase(const void *file_base,
                                                const void *ptr,
                                                size_t size);
extern void *ndsRelocResolveAuthoritativeForceFile(void *file);

static DObj *ndsLBCommonGetTreeDObjNextFromRoot(DObj *dobj, DObj *root_dobj)
{
    if (dobj == NULL)
    {
        return NULL;
    }
    if (dobj->child != NULL)
    {
        return dobj->child;
    }
    /* decomp lb/lbcommon.c:757 -- a CHILDLESS ROOT terminates the walk. Without
     * this the walk leaves root_dobj's subtree through the root's own sibling
     * (and then through its parent's), which is how a fighter's TransN, parked
     * as XRotN's sibling by ftMainSetStatus' TransN reparent
     * (ft/ftmain.c:4710-4721), could be handed another subtree's script. Every
     * root used today has children, so this restores the guard rather than
     * changing an observed behaviour. */
    if (dobj == root_dobj)
    {
        return NULL;
    }
    if (dobj->sib_next != NULL)
    {
        return dobj->sib_next;
    }
    while (dobj != NULL)
    {
        if (dobj->parent == root_dobj)
        {
            return NULL;
        }
        if ((dobj->parent != NULL) && (dobj->parent->sib_next != NULL))
        {
            return dobj->parent->sib_next;
        }
        dobj = dobj->parent;
    }
    return NULL;
}

void ndsR2AnimZeroCopyNoteBind(GObj *gobj, const void *figatree);

void lbCommonAddFighterPartsFigatree(DObj *root_dobj, void *figatree,
                                     f32 anim_frame)
{
    figatree = ndsRelocResolveAuthoritativeForceFile(figatree);
    /* A zero-copy cache clip is now this fighter's binding (see
     * sNdsR2AnimPins in reloc_backend_assets.c). */
    if ((root_dobj != NULL) && (root_dobj->parent_gobj != NULL))
    {
        ndsR2AnimZeroCopyNoteBind(root_dobj->parent_gobj, figatree);
    }
    void **figatree_entries = figatree;
    DObj *current_dobj = root_dobj;
    /* P2-2p6: the fighter pose engine takes the attach when it can hold the
     * walk. Counted first, because the engine's state is carved from the arena
     * once per fighter at BattleShip's fixed fp->joints[] topology bound. That
     * includes hidden parts ftMainSetStatus can materialise later; an attach it
     * cannot hold must fall back to the generic bind as a whole -- never half a
     * fighter. */
    u32 pose_entries = 0u;
    sb32 pose_engine;
    sb32 bind_anim_joint = FALSE;

    if ((root_dobj != NULL) && (root_dobj->parent_gobj != NULL))
    {
        root_dobj->parent_gobj->anim_frame = anim_frame;
    }
    if (NDS_FT_POSE != 0)
    {
        DObj *walk = root_dobj;

        while (walk != NULL)
        {
            pose_entries++;
            walk = ndsLBCommonGetTreeDObjNextFromRoot(walk, root_dobj);
        }
    }
    /* BIND TO THE ENGINE THAT WILL ACTUALLY PLAY THIS MOTION.
     *
     * `ftMainPlayAnim` routes a motion to the pose engine only when
     * `fp->anim_desc.flags.is_anim_joint` is clear -- an event32 animjoint
     * status keeps the generic player. This attach did not ask the same
     * question, so an `is_anim_joint` motion was bound INTO the pose engine and
     * then played by the generic one: the pose engine holds the only live
     * script, the generic walk finds `AOBJ_ANIM_NULL` on every joint, and
     * nothing advances or publishes `parent_gobj->anim_frame`.
     *
     * That is what broke the entry. Mario's Appear (motion 197,
     * `nFTMarioMotionAppearR`) is an `is_anim_joint` motion -- measured,
     * `aj=1` with `joints[0]->child->anim_wait = AOBJ_ANIM_NULL` and a GObj
     * clock frozen for the whole status -- so `ftCommonAppearProcUpdate`'s
     * `anim_frame <= 0.0F` was true on the tick the status began and both
     * fighters left their entry immediately while their own effect played on.
     * The owner saw that as "the pipe only renders its rim".
     *
     * The two sites now ask the identical question, which is the property that
     * matters: whichever engine binds a motion is the engine that plays it. */
    {
        FTStruct *bind_fp = (root_dobj != NULL) ?
            ftGetStruct(root_dobj->parent_gobj) : NULL;

        bind_anim_joint =
            ((bind_fp != NULL) && (bind_fp->anim_desc.flags.is_anim_joint)) ?
                TRUE : FALSE;
        pose_engine = ((figatree_entries != NULL) &&
                       (bind_anim_joint == FALSE)) ?
            ndsFtPoseBindBegin(root_dobj, pose_entries) : FALSE;
        if ((bind_anim_joint != FALSE) && (root_dobj != NULL) &&
            (root_dobj->parent != NULL))
        {
            /* HAND-OVER FROM THE TRACK ENGINE. An event32 motion is played by
             * the generic walker over every joint from TopN down, TopN
             * included, while this attach walks TopN's child subtree only
             * (ftmain.c:4704). The figatree motion before it ran on the R2
             * track engine, whose AObjs sit on the live joints in Q form with
             * no interpolate descriptor -- the descriptor lives in the track
             * block. A leftover TraI track on TopN therefore reached
             * syInterpCubic(NULL) the moment Mario's Appear bound after Planet
             * Zebes' 56-frame entry pan (2026-09-07). BattleShip never sees
             * this: its leftovers keep a valid descriptor. Retire TopN's
             * tracks the way gcAddDObjAnimJoint retires a scripted joint's
             * (objanim.c:141); the unscripted joints below get the same in
             * their arm. */
            AObj *stale = root_dobj->parent->aobj;

            while (stale != NULL)
            {
                stale->kind = nGCAnimKindNone;
                stale = stale->next;
            }
        }
    }
    pose_entries = 0u;
    while ((current_dobj != NULL) && (figatree_entries != NULL))
    {
        AObjEvent32 *anim_joint = *figatree_entries;
        FTParts *parts = ftGetParts(current_dobj);
        /* EXTRA's static part (nds_p4.h static_parts): unless a hidden part
         * of the motion is rooted at it, it has no track in the figatree and
         * takes no entry (StaticPart.asm skip_static_parts_). */
        const sb32 static_skip =
            ((parts != NULL) && (root_dobj->parent_gobj != NULL) &&
             (ndsP4StaticPartSkipsAnim(ftGetStruct(root_dobj->parent_gobj),
                                       parts->joint_id) != FALSE)) ?
                TRUE : FALSE;

        anim_joint = (static_skip != FALSE) ? NULL :
            ndsRelocResolvePointerFromFileBase(figatree, anim_joint,
                                               sizeof(*anim_joint));
        if (anim_joint != NULL)
        {
            if (pose_engine != FALSE)
            {
                ndsFtPoseBindEntry(pose_entries, current_dobj,
                                   (AObjEvent16 *)anim_joint, anim_frame);
            }
            else
            {
                gcAddDObjAnimJoint(current_dobj, anim_joint, anim_frame);
            }
            NDS_DIAG(gNdsFighterNaturalMotionFigatreeAttachCount++);
            if (parts != NULL)
            {
                parts->is_have_anim = TRUE;
            }
        }
        else
        {
            if (anim_joint == NULL)
            {
                NDS_DIAG(gNdsFighterNaturalMotionFigatreeNullCount++);
            }
            else
            {
                gNdsFighterNaturalMotionFigatreeAnimInvalidCount++;
                gNdsFighterNaturalMotionUnsafeCount++;
            }
            if (pose_engine != FALSE)
            {
                ndsFtPoseBindEntry(pose_entries, current_dobj, NULL,
                                   anim_frame);
            }
            else if (bind_anim_joint != FALSE)
            {
                /* Same hand-over as TopN above: an unscripted joint keeps its
                 * AObjs, and the generic walker would evaluate the engine's
                 * Q-form leftovers. */
                AObj *stale = current_dobj->aobj;

                while (stale != NULL)
                {
                    stale->kind = nGCAnimKindNone;
                    stale = stale->next;
                }
            }
            current_dobj->anim_wait = AOBJ_ANIM_NULL;
            if (parts != NULL)
            {
                parts->is_have_anim = FALSE;
            }
        }
        if (static_skip == FALSE)
        {
            figatree_entries++;
        }
        pose_entries++;
        current_dobj =
            ndsLBCommonGetTreeDObjNextFromRoot(current_dobj, root_dobj);
    }
    if (pose_engine != FALSE)
    {
        ndsFtPoseBindEnd(pose_entries);
    }
}

/* decomp lb/lbcommon.c:785, which this port has never had a body for -- it was
 * an empty stub (`bx lr` at 0x02052eac in build-c132-stress) until 2026-08-13.
 *
 * ftCommonGuardInitJoints (ft/ftcommon/ftcommonguard1.c:275-282) sets
 * fp->anim_desc.flags.is_anim_joint = TRUE and calls this in the same breath,
 * because this is what makes the flag TRUE: it replaces every joint in
 * root_dobj's subtree with an event32 shield anim joint, or stops the joint
 * where the table entry is NULL. With no body the flag was set while every
 * joint still held the GuardOn FIGATREE that ftMainSetStatus installed
 * (ft/ftmain.c:4704), so ftParamUpdateAnimKeys' per-fighter dispatch
 * (ft/ftparam.c:386) ran the 32-bit parser over 16-bit commands -- the misread
 * attributed in artifacts/performance/2026-08-13_c-anim-anomalies/ANOMALIES.md,
 * whose `default:` drops the joint's animation entirely. The flag's clear is
 * source-exact and was never missing: ftMainSetStatus re-derives the WHOLE word
 * from the motion descriptor at ft/ftmain.c:4633
 * (`fp->anim_desc.word = motion_desc->anim_desc.word`), which a grep for the
 * field name cannot see because FTAnimDesc is a union.
 *
 * The entries are resolved exactly as lbCommonAddFighterPartsFigatree resolves
 * the figatree's: absolute pointers into a loaded file pass through, file-
 * relative ones are rebased, and anything that resolves to nothing is treated
 * as the NULL entry rather than handed to the parser. */
void lbCommonAddDObjAnimJointAll(DObj *dobj, AObjEvent32 **anim_joint,
                                 f32 anim_frame)
{
    const void *file_base = anim_joint;
    DObj *current_dobj = dobj;

    NDS_DIAG(gNdsShieldAnimJointInstallCalls++);
    if ((dobj == NULL) || (anim_joint == NULL))
    {
        return;
    }
    /* P2-2p6: an event32 attach re-targets these joints; the pose engine
     * steps aside until the next figatree attach. */
    ndsFtPoseUnbind(dobj->parent_gobj);
    if (dobj->parent_gobj != NULL)
    {
        dobj->parent_gobj->anim_frame = anim_frame;
    }
    while (current_dobj != NULL)
    {
        AObjEvent32 *script;

        /* The guard's walk passes over EXTRA's static parts: no animation,
         * no entry (StaticPart.asm lbCommonAddDObjAnimJointAll_extended_). */
        if (ndsP4StaticPartDObj(current_dobj) != FALSE)
        {
            current_dobj->anim_wait = AOBJ_ANIM_NULL;
            current_dobj =
                ndsLBCommonGetTreeDObjNextFromRoot(current_dobj, dobj);
            continue;
        }
        script = ndsRelocResolvePointerFromFileBase(file_base, *anim_joint,
                                                    sizeof(AObjEvent32));

        if (script != NULL)
        {
            gcAddDObjAnimJoint(current_dobj, script, anim_frame);
            NDS_DIAG(gNdsShieldAnimJointAttachCount++);
        }
        else
        {
            current_dobj->anim_wait = AOBJ_ANIM_NULL;
            NDS_DIAG(gNdsShieldAnimJointNullCount++);
        }
        anim_joint++;
        current_dobj =
            ndsLBCommonGetTreeDObjNextFromRoot(current_dobj, dobj);
    }
}

void lbCommonInitDObj(DObj *dobj, u8 tk1, u8 tk2, u8 tk3, u8 arg4)
{
    if (dobj == NULL)
    {
        return;
    }
    if (tk1 != nGCMatrixKindNull)
    {
        gcAddXObjForDObjFixed(dobj, tk1, arg4);
    }
    if (tk2 != nGCMatrixKindNull)
    {
        gcAddXObjForDObjFixed(dobj, tk2, arg4);
    }
    if (tk3 != nGCMatrixKindNull)
    {
        gcAddXObjForDObjFixed(dobj, tk3, arg4);
    }
    dobj->translate.vec.f.x = 0.0F;
    dobj->translate.vec.f.y = 0.0F;
    dobj->translate.vec.f.z = 0.0F;
    dobj->rotate.vec.f.x = 0.0F;
    dobj->rotate.vec.f.y = 0.0F;
    dobj->rotate.vec.f.z = 0.0F;
    dobj->scale.vec.f.x = 1.0F;
    dobj->scale.vec.f.y = 1.0F;
    dobj->scale.vec.f.z = 1.0F;
}

void lbCommonInitDObj3Transforms(DObj *dobj, u8 tk1, u8 tk2, u8 tk3)
{
    if (dobj == NULL)
    {
        return;
    }
    if (tk1 != nGCMatrixKindNull)
    {
        gcAddXObjForDObjFixed(dobj, tk1, 0);
    }
    if (tk2 != nGCMatrixKindNull)
    {
        gcAddXObjForDObjFixed(dobj, tk2, 0);
    }
    if (tk3 != nGCMatrixKindNull)
    {
        gcAddXObjForDObjFixed(dobj, tk3, 0);
    }
    dobj->translate.vec.f.x = 0.0F;
    dobj->translate.vec.f.y = 0.0F;
    dobj->translate.vec.f.z = 0.0F;
    dobj->rotate.vec.f.x = 0.0F;
    dobj->rotate.vec.f.y = 0.0F;
    dobj->rotate.vec.f.z = 0.0F;
    dobj->scale.vec.f.x = 1.0F;
    dobj->scale.vec.f.y = 1.0F;
    dobj->scale.vec.f.z = 1.0F;
}

extern void gcDecideDObj3TransformsKind(DObj *dobj, u8 tk1, u8 tk2,
                                         u8 tk3, s32 flags);

/* P2-3f50 parts-setup guard telemetry, read by
 * scripts/probe-battle-progress.ps1 -ExtraGlobals. */
__attribute__((used)) volatile u32 gNdsFTPartsSetupBadIdCount;
/* P2-2p8 joint-cap lab arm. The limit is a RUNTIME variable, not an `#if`, for
 * two reasons that both bit on the first attempt. One: a counter written only
 * inside `#if NDS_LAB_JOINT_CAP` has no compiled writer in the control, so
 * --gc-sections drops it and the harness refuses the run -- `used` does not
 * save it, because devkitARM ignores the `retain` attribute. Two: with the
 * branch always compiled, control and arm are the SAME ROM with byte-identical
 * .text, so the comparison cannot be contaminated by layout or by the
 * instrument's own cost, which is the only A/B form this campaign trusts.
 * 0 disables the cap. Seeded from NDS_LAB_JOINT_CAP so a build can pin it, and
 * pokeable so one binary runs the whole ladder. */
__attribute__((used)) volatile u32 gNdsLabJointCapLimit = NDS_LAB_JOINT_CAP;
__attribute__((used)) volatile u32 gNdsLabJointCapPrunedCount;
__attribute__((used)) volatile u32 gNdsLabJointCapKeptCount;
__attribute__((used)) volatile u32 gNdsFTPartsSetupBadId;
__attribute__((used)) volatile u32 gNdsFTPartsSetupBadStep;
__attribute__((used)) volatile u32 gNdsFTPartsSetupBadDescId;
__attribute__((used)) volatile u32 gNdsFTPartsSetupBadDesc;
__attribute__((used)) volatile u32 gNdsFTPartsSetupBadContainer;
__attribute__((used)) volatile u32 gNdsFTPartsSetupBadDetail;

void lbCommonSetupFighterPartsDObjs(DObj *root_dobj,
                                    FTCommonPartContainer *commonparts_container,
                                    s32 detail_curr, DObj **dobjs,
                                    u32 *setup_parts, u8 tk1, u8 tk2,
                                    u8 tk3, f32 anim_frame, u8 arg9)
{
    DObj *array_dobjs[DOBJ_ARRAY_MAX];
    DObjDesc *dobjdesc;
    u32 flags0;
    u32 flags1;
    s32 i;

    if ((root_dobj == NULL) || (commonparts_container == NULL) ||
        (setup_parts == NULL))
    {
        return;
    }
    dobjdesc = commonparts_container
                   ->commonparts[detail_curr - nFTPartsDetailStart]
                   .dobjdesc;
    if (dobjdesc == NULL)
    {
        return;
    }
    for (i = 0; i < DOBJ_ARRAY_MAX; i++)
    {
        array_dobjs[i] = NULL;
    }
    flags0 = setup_parts[0];
    flags1 = setup_parts[1];
    for (i = 0; ((flags0 != 0u) || (flags1 != 0u)) &&
                (dobjdesc->id != DOBJ_ARRAY_MAX);
         i++, dobjdesc++)
    {
        const u32 current_flags = (i < 32) ? flags0 : flags1;
        if (current_flags & (1u << 31))
        {
            const s32 id = dobjdesc->id & 0xFFF;
            s32 detail_id = nFTPartsDetailHigh - nFTPartsDetailStart;
            DObj *parent;
            DObj *current_dobj;

            /* GUARD, not a fix (P2-3f50). The source indexes
             * array_dobjs[id] with whatever the descriptor holds, so a
             * descriptor the loader left unrelocated writes outside this
             * frame and the NEXT fighter's setup faults in its own
             * prologue -- which is how Purin's abort presented, three
             * frames from its cause. Publish the first bad index and stop
             * this tree instead of smashing the stack. Valid data never
             * reaches this branch. */
            if ((u32)id >= (u32)DOBJ_ARRAY_MAX)
            {
                if (gNdsFTPartsSetupBadIdCount == 0u)
                {
                    gNdsFTPartsSetupBadId = (u32)id;
                    gNdsFTPartsSetupBadStep = (u32)i;
                    gNdsFTPartsSetupBadDescId = (u32)dobjdesc->id;
                    gNdsFTPartsSetupBadDesc = (u32)(uintptr_t)dobjdesc;
                    gNdsFTPartsSetupBadContainer =
                        (u32)(uintptr_t)commonparts_container;
                    gNdsFTPartsSetupBadDetail = (u32)detail_curr;
                }
                gNdsFTPartsSetupBadIdCount++;
                break;
            }
            /* The prune is here and not in a per-joint loop on purpose. Leaving
             * the DObj uncreated removes it from EVERY consumer at once -- the
             * pose player, ftParamUpdateAnimKeys, the invalidate walk, the
             * matrix build and the draw traversal all reach parts through this
             * tree -- so the arm prices a smaller skeleton rather than one
             * subsystem declining to look at a skeleton that still exists.
             * `array_dobjs[id]` stays NULL, so every descendant hits the
             * `parent == NULL` test below and prunes with its ancestor.
             *
             * Known: any nonzero cap this roster can reach ABORTS in the CPU
             * AI. ndsBaseFTComputerSetFighterDamageDetectSize dereferences the
             * NULL this leaves in fp->joints[] (decomp ft/ftcomputer.c:7970),
             * because damage_coll_descs names joints by id. That is the arm's
             * result, not a fault in it: a smaller skeleton is a per-fighter
             * data re-derivation. Kept live so the ladder is reproducible. */
            {
                const u32 joint_cap = gNdsLabJointCapLimit;

                if ((joint_cap != 0u) && ((u32)id >= joint_cap))
                {
                    NDS_DIAG(gNdsLabJointCapPrunedCount++);
                    goto advance_flags;
                }
                NDS_DIAG(gNdsLabJointCapKeptCount++);
            }
            parent = (id != 0) ? array_dobjs[id - 1] : root_dobj;

            if ((detail_curr != nFTPartsDetailHigh) &&
                (commonparts_container
                     ->commonparts[nFTPartsDetailLow - nFTPartsDetailStart]
                     .dobjdesc != NULL) &&
                (commonparts_container
                     ->commonparts[nFTPartsDetailLow - nFTPartsDetailStart]
                     .dobjdesc[i]
                     .dl != NULL))
            {
                detail_id = nFTPartsDetailLow - nFTPartsDetailStart;
            }
            if (parent == NULL)
            {
                goto advance_flags;
            }
            current_dobj = gcAddChildForDObj(
                parent,
                commonparts_container->commonparts[detail_id].dobjdesc[i].dl);
            if (current_dobj == NULL)
            {
                goto advance_flags;
            }
            array_dobjs[id] = current_dobj;
            if ((dobjdesc->id & 0x8000) != 0)
            {
                gcDecideDObj3TransformsKind(current_dobj, tk1, tk2, tk3,
                                             0x8000);
            }
            else
            {
                lbCommonInitDObj(current_dobj, tk1, tk2, tk3, arg9);
            }
            current_dobj->translate.vec.f = dobjdesc->translate;
            current_dobj->rotate.vec.f = dobjdesc->rotate;
            current_dobj->scale.vec.f = dobjdesc->scale;
            lbCommonAddMObjForFighterPartsDObj(
                current_dobj,
                (commonparts_container->commonparts[detail_id].p_mobjsubs !=
                 NULL)
                    ? commonparts_container->commonparts[detail_id]
                          .p_mobjsubs[i]
                    : NULL,
                (commonparts_container->commonparts[detail_id]
                     .p_costume_matanim_joints != NULL)
                    ? commonparts_container->commonparts[detail_id]
                          .p_costume_matanim_joints[i]
                    : NULL,
                NULL, anim_frame);
            if (dobjs != NULL)
            {
                *dobjs = current_dobj;
            }
        }
advance_flags:
        if (dobjs != NULL)
        {
            dobjs++;
        }
        if (i < 32)
        {
            flags0 <<= 1;
        }
        else
        {
            flags1 <<= 1;
        }
    }
}
