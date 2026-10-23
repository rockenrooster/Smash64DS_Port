/* The source-named ftmain procs and hit-pipeline entry points: the imported
 * battleship_ftmain.c renames its own (battleship_ftMain*), and these call
 * them, some bracketed for the tick HUD's spans (SHDT hit detection, SCAT
 * the catch search, SGCO), plus ftParamGetCapturedDamage.
 * sNdsFighterDashRunHitCollisionFGMs is parsed by
 * scripts/sfx/check-fgm-pack-coverage.py. A slice of reloc_backend.c. */


static const u16 sNdsFighterDashRunHitCollisionFGMs[8][3] = {
    { 40u, 38u, 37u },    /* Punch: S/M/L */
    { 34u, 32u, 31u },    /* Kick: S/M/L */
    { 216u, 216u, 216u }, /* Coin */
    { 28u, 27u, 25u },    /* Burn: S/M/L */
    { 24u, 23u, 22u },    /* Shock: S/M/L */
    { 263u, 262u, 261u }, /* Slash: S/M/L */
    { 51u, 51u, 51u },    /* Fan / slap */
    { 38u, 37u, 52u }     /* Bat */
};

s32 ftParamGetCapturedDamage(FTStruct *fp, s32 damage)
{
    if (fp == NULL)
    {
        return 0;
    }
    if (fp->capture_gobj != NULL)
    {
        damage = (s32)((damage * 0.5F) + 0.999F);
    }
    return (s32)((damage * fp->damage_mul) + 0.999F);
}

static sb32 sNdsStageMPLiveHitDamageLoopShieldStatProofActive = FALSE;

extern sb32 battleship_ftMainCheckGetUpdateDamage(FTStruct *fp,
                                                  s32 *damage);
extern void battleship_ftMainPlayHitSFX(FTStruct *fp,
                                        FTAttackColl *attack_coll);
extern void battleship_ftMainUpdateDamageStatFighter(
    FTStruct *attacker_fp, FTAttackColl *attack_coll, FTStruct *victim_fp,
    FTDamageColl *damage_coll, GObj *attacker_gobj, GObj *victim_gobj);
extern void battleship_ftMainSetHitRebound(GObj *attacker_gobj,
                                           FTStruct *fp,
                                           FTAttackColl *attack_coll,
                                           GObj *victim_gobj);
extern void battleship_ftMainUpdateAttackStatFighter(
    FTStruct *other_fp, FTAttackColl *other_hit, FTStruct *this_fp,
    FTAttackColl *this_hit, GObj *other_gobj, GObj *this_gobj);
extern void battleship_ftMainUpdateShieldStatFighter(
    FTStruct *attacker_fp, FTAttackColl *attack_coll, FTStruct *victim_fp,
    GObj *attacker_gobj, GObj *victim_gobj);
extern void battleship_ftMainUpdateCatchStatFighter(
    FTStruct *attacker_fp, FTAttackColl *attack_coll, FTStruct *victim_fp,
    GObj *attacker_gobj, GObj *victim_gobj);
extern void battleship_ftMainProcessHitCollisionStatsMain(
    GObj *fighter_gobj);
extern sb32 battleship_ftMainCheckAddGroundObstacle(
    GObj *gobj, sb32 (*proc_update)(GObj *, GObj *, s32 *));
extern void battleship_ftMainClearGroundObstacle(GObj *gobj);
extern void battleship_ftMainSetHitHazard(GObj *gobj, GObj *fighter_gobj,
                                          FTStruct *fp, s32 kind);
extern void battleship_ftMainSearchHitHazard(GObj *fighter_gobj);
extern void battleship_ftMainSearchHitFighter(GObj *this_gobj);
extern void battleship_ftMainSearchFighterCatch(GObj *this_gobj);
extern void battleship_ftMainProcSearchCatch(GObj *fighter_gobj);
extern void battleship_ftMainSearchHitItem(GObj *fighter_gobj);
extern void battleship_ftMainSearchHitWeapon(GObj *fighter_gobj);
extern void battleship_ftMainSearchGroundHit(GObj *fighter_gobj);
extern void battleship_ftMainProcSearchHitAll(GObj *fighter_gobj);
extern void battleship_ftMainProcParams(GObj *fighter_gobj);
/* Cycle 92 SGCO split: the other three of the six per-fighter procs. */
extern void battleship_ftMainProcUpdateInterrupt(GObj *fighter_gobj);
extern void battleship_ftMainProcPhysicsMapDefault(GObj *fighter_gobj);
extern void battleship_ftMainProcPhysicsMapCapture(GObj *fighter_gobj);

sb32 ftMainCheckGetUpdateDamage(FTStruct *fp, s32 *damage)
{
    return battleship_ftMainCheckGetUpdateDamage(fp, damage);
}

void ftMainPlayHitSFX(FTStruct *fp, FTAttackColl *attack_coll)
{
    battleship_ftMainPlayHitSFX(fp, attack_coll);
}

void ftMainUpdateDamageStatFighter(FTStruct *attacker_fp,
                                   FTAttackColl *attack_coll,
                                   FTStruct *victim_fp,
                                   FTDamageColl *damage_coll,
                                   GObj *attacker_gobj,
                                   GObj *victim_gobj)
{
    battleship_ftMainUpdateDamageStatFighter(attacker_fp, attack_coll,
                                             victim_fp, damage_coll,
                                             attacker_gobj, victim_gobj);
}

void ftMainSetHitRebound(GObj *attacker_gobj, FTStruct *fp,
                         FTAttackColl *attack_coll, GObj *victim_gobj)
{
    battleship_ftMainSetHitRebound(attacker_gobj, fp, attack_coll,
                                   victim_gobj);
}

void ftMainUpdateAttackStatFighter(FTStruct *other_fp,
                                   FTAttackColl *other_hit,
                                   FTStruct *this_fp,
                                   FTAttackColl *this_hit,
                                   GObj *other_gobj,
                                   GObj *this_gobj)
{
    battleship_ftMainUpdateAttackStatFighter(other_fp, other_hit, this_fp,
                                             this_hit, other_gobj, this_gobj);
}

void ftMainUpdateShieldStatFighter(FTStruct *attacker_fp,
                                   FTAttackColl *attack_coll,
                                   FTStruct *victim_fp,
                                   GObj *attacker_gobj,
                                   GObj *victim_gobj)
{
    battleship_ftMainUpdateShieldStatFighter(attacker_fp, attack_coll,
                                             victim_fp, attacker_gobj,
                                             victim_gobj);
}

void ftMainUpdateCatchStatFighter(FTStruct *attacker_fp,
                                  FTAttackColl *attack_coll,
                                  FTStruct *victim_fp,
                                  GObj *attacker_gobj,
                                  GObj *victim_gobj)
{
    battleship_ftMainUpdateCatchStatFighter(attacker_fp, attack_coll,
                                            victim_fp, attacker_gobj,
                                            victim_gobj);
}

void ftMainProcessHitCollisionStatsMain(GObj *fighter_gobj)
{
    battleship_ftMainProcessHitCollisionStatsMain(fighter_gobj);
}

sb32 ftMainCheckAddGroundObstacle(GObj *gobj,
                                  sb32 (*proc_update)(GObj *, GObj *, s32 *))
{
    return battleship_ftMainCheckAddGroundObstacle(gobj, proc_update);
}

void ftMainClearGroundObstacle(GObj *gobj)
{
    battleship_ftMainClearGroundObstacle(gobj);
}

void ftMainSetHitHazard(GObj *gobj, GObj *fighter_gobj, FTStruct *fp,
                        s32 kind)
{
    battleship_ftMainSetHitHazard(gobj, fighter_gobj, fp, kind);
}

void ftMainSearchHitHazard(GObj *fighter_gobj)
{
    battleship_ftMainSearchHitHazard(fighter_gobj);
}

void ftMainSearchHitFighter(GObj *this_gobj)
{
    FTStruct *this_fp = (this_gobj != NULL) ? ftGetStruct(this_gobj) : NULL;
    s32 shield_damage_before = (this_fp != NULL) ? this_fp->shield_damage : 0;
    s32 shield_damage_total_before =
        (this_fp != NULL) ? this_fp->shield_damage_total : 0;
    sb32 is_shield_before =
        (this_fp != NULL) ? this_fp->is_shield : FALSE;

    battleship_ftMainSearchHitFighter(this_gobj);
    if ((sNdsStageMPLiveHitDamageLoopShieldStatProofActive != FALSE) &&
        (this_fp != NULL) && (is_shield_before != FALSE) &&
        ((this_fp->shield_damage != shield_damage_before) ||
         (this_fp->shield_damage_total != shield_damage_total_before)))
    {
        gNdsStageMPLiveHitDamageLoopShieldPlayer = this_fp->shield_player;
        gNdsStageMPLiveHitDamageLoopShieldEffectCount++;
        gNdsStageMPLiveHitDamageLoopShieldEffectSize =
            this_fp->shield_damage;
    }
    else if ((sNdsStageMPLiveHitDamageLoopShieldStatProofActive != FALSE) &&
        (this_fp != NULL) && (is_shield_before != FALSE) &&
        (this_fp->shield_damage == shield_damage_before) &&
        (this_fp->shield_damage_total == shield_damage_total_before))
    {
        GObj *attacker_gobj = gGCCommonLinks[nGCCommonLinkIDFighter];

        while (attacker_gobj != NULL)
        {
            FTStruct *attacker_fp = ftGetStruct(attacker_gobj);
            u32 i;

            if ((attacker_gobj == this_gobj) || (attacker_fp == NULL))
            {
                attacker_gobj = attacker_gobj->link_next;
                continue;
            }
            for (i = 0u; i < FTATTACKCOLL_NUM_MAX; i++)
            {
                FTAttackColl *attack_coll = &attacker_fp->attack_colls[i];

                if (attack_coll->attack_state == nGMAttackStateOff)
                {
                    continue;
                }
                if (ndsFighterMarioFoxStageMPLiveHitCanDetectDamage(
                        attack_coll, this_gobj) == FALSE)
                {
                    continue;
                }
                if (!(((this_fp->ga == nMPKineticsAir) &&
                       (attack_coll->is_hit_air != FALSE)) ||
                      ((this_fp->ga == nMPKineticsGround) &&
                       (attack_coll->is_hit_ground != FALSE))))
                {
                    continue;
                }
                {
                    ftMainUpdateShieldStatFighter(attacker_fp, attack_coll,
                                                  this_fp, attacker_gobj,
                                                  this_gobj);
                    return;
                }
            }
            attacker_gobj = attacker_gobj->link_next;
        }
    }
}

void ftMainSearchFighterCatch(GObj *this_gobj)
{
    battleship_ftMainSearchFighterCatch(this_gobj);
}

void ftMainProcSearchCatch(GObj *fighter_gobj)
{
    /* Cycle 86 SCAT. Grab/catch search, fighter proc priority 2 (decomp
     * ft/ftmanager.c:861) -- the sequential sibling of SHDT at priority 1, and
     * the other search population that can switch on. Nested inside GCRA and
     * disjoint from every other bracketed proc. */

    battleship_ftMainProcSearchCatch(fighter_gobj);
}

void ftMainSearchHitItem(GObj *fighter_gobj)
{
    battleship_ftMainSearchHitItem(fighter_gobj);
}

void ftMainSearchHitWeapon(GObj *fighter_gobj)
{
    battleship_ftMainSearchHitWeapon(fighter_gobj);
}

void ftMainSearchGroundHit(GObj *fighter_gobj)
{
    battleship_ftMainSearchGroundHit(fighter_gobj);
}

void ftMainProcSearchHitAll(GObj *fighter_gobj)
{
    /* Cycle 85 SHDT. Live-hitbox hit detection, bracketed here on the port-side
     * wrapper so no decomp/ edit is needed. This span is nested inside the SRC
     * bracket in ndsRunMarioFoxProofUpdate; cpuGetTiming is a read of the
     * free-running timer pair, never a reset, so nesting it inside SRC's own
     * bracket is safe. R2-03 E35 measured this population as the owner of the
     * SRC excursion but did so on a 128-frame window; this is the whole-match
     * re-measurement. */

    NDS_FREEZE_DIAGNOSTICS_MARK(NDS_FREEZE_BREADCRUMB_HIT_SEARCH);
    battleship_ftMainProcSearchHitAll(fighter_gobj);
}

void ftMainProcParams(GObj *fighter_gobj)
{
    /* Cycle 86 SPRM. Fighter proc priority 0 (decomp ft/ftmanager.c:863), the
     * last of the six and the one that carries the animation/event interpreter
     * and the status/param update. The audit hook is deliberately INSIDE the
     * bracket: it is compiled out of the measuring ROM
     * (NDS_FIGHTER_ANIM_AUDIT 0) so it costs nothing there, and if it is ever
     * enabled its cost belongs to this owner rather than silently to SGCO. */

    battleship_ftMainProcParams(fighter_gobj);
}

/* Cycle 92 SGCO split. The remaining three of the six per-fighter procs
 * (decomp ft/ftmanager.c:858-860, priorities 5/4/3). Cycle 86 left these
 * unbracketed because linker/nds_hot_text.ld pinned them into ITCM by their
 * decomp symbol names; that is resolved by renaming them in
 * src/import/battleship_ftmain.c and moving the three .ld pins to the new names
 * IN PLACE in the same commit, so the curated 8 KiB hot list keeps its size,
 * its membership and its ORDER. These wrappers themselves land in .main like
 * the three above them, which is the same indirection SCAT/SHDT/SPRM already
 * pay.
 *
 * With these three bracketed, SGCO stops being a residual:
 *   SGCO = (SINT - SCPU) + SPHD + SPHC + SOBJ
 * where SOBJ is the analyzer-derived non-fighter remainder (camera, effects,
 * items, weapons, interface, and gcRunAll's own two dispatch loops). */

void ftMainProcUpdateInterrupt(GObj *fighter_gobj)
{
    /* SINT. Fighter proc priority 5, the first to run. The level-3 CPU AI is
     * nested INSIDE this proc (ftmain.c:1269, case nFTPlayerKindCom) and is
     * already bracketed as SCPU, so this span deliberately includes it and the
     * analyzer subtracts: SITR = SINT - SCPU. That keeps SCPU's banked series
     * unchanged and makes SITR's non-negativity the proof the nesting is real. */

    battleship_ftMainProcUpdateInterrupt(fighter_gobj);
}

void ftMainProcPhysicsMapDefault(GObj *fighter_gobj)
{
    /* SPHD. Fighter proc priority 4. This and SPHC are mutually exclusive arms
     * of the same capture predicate (ftmain.c:1918-1937): each calls the shared
     * ftMainProcPhysicsMap only when its own condition holds, so bracketing both
     * outer procs is disjoint and exhaustive over the shared worker. In a match
     * without grabs SPHD does the work and SPHC is a two-field early-out, which
     * is the prediction these two are checked against. */

    battleship_ftMainProcPhysicsMapDefault(fighter_gobj);
}

void ftMainProcPhysicsMapCapture(GObj *fighter_gobj)
{
    /* SPHC. Fighter proc priority 3, the captured arm of the pair above. */

    battleship_ftMainProcPhysicsMapCapture(fighter_gobj);
}

typedef GRObstacle NDSFTMainGroundObstacle;
