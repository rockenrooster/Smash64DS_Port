#define NDS_FTMOTION_ATTACK_EVENT_POS_NEW 0x1u
#define NDS_FTMOTION_ATTACK_EVENT_POS_WORLD 0x2u
#define NDS_FTMOTION_ATTACK_EVENT_POS_TRANSFER 0x4u
#define NDS_FTMOTION_ATTACK_EVENT_POS_MATRIX_RESET 0x8u
#define NDS_FTMOTION_ATTACK_EVENT_POS_JOINT_READY 0x10u
#define NDS_FTMOTION_ATTACK_EVENT_POS_WRITEBACK 0x20u
#define NDS_FTMOTION_ATTACK_EVENT_POS_INTERPOLATE 0x40u
#define NDS_FTMOTION_ATTACK_EVENT_POS_PREV_COPY 0x80u
#define NDS_FTMOTION_ATTACK_EVENT_POS_RANGE_CURR 0x100u
#define NDS_FTMOTION_ATTACK_EVENT_POS_RANGE_PREV 0x200u
#define NDS_FTMOTION_ATTACK_EVENT_POS_DAMAGE_RECT 0x400u
#define NDS_FTMOTION_ATTACK_EVENT_POS_DAMAGE_COLLIDE 0x800u
#define NDS_FTMOTION_ATTACK_EVENT_POS_DAMAGE_RECORD 0x1000u
#define NDS_FTMOTION_ATTACK_EVENT_POS_DAMAGE_HITLOG 0x2000u
#define NDS_FTMOTION_ATTACK_EVENT_POS_HIT_SFX 0x4000u
#define NDS_FTMOTION_ATTACK_EVENT_POS_HIT_STATS 0x8000u
#define NDS_FTMOTION_ATTACK_EVENT_POS_PROCPARAMS 0x10000u
#define NDS_FTMAIN_PROCPARAMS_DAMAGE_INPUT 0x1u
#define NDS_FTMAIN_PROCPARAMS_UPDATE_DAMAGE 0x2u
#define NDS_FTMAIN_PROCPARAMS_DAMAGE_STATUS 0x4u
#define NDS_FTMAIN_PROCPARAMS_SHUFFLE 0x8u
#define NDS_FTMAIN_PROCPARAMS_RUMBLE 0x10u
#define NDS_FTMAIN_PROCPARAMS_HITLAG 0x20u
#define NDS_FTMAIN_PROCPARAMS_CLEAR 0x40u
#define NDS_FTMAIN_PROCPARAMS_LAGSTART 0x80u
#define NDS_FTMAIN_PROCPARAMS_ATTACK_DAMAGE 0x100u
#define NDS_FTMAIN_PROCPARAMS_ATTACK_SHIELD_PUSH 0x200u
#define NDS_FTMAIN_PROCPARAMS_SHIELD_DAMAGE 0x400u
#define NDS_FTMAIN_PROCPARAMS_SHIELD_BREAK 0x800u
#define NDS_FTMAIN_PROCPARAMS_REFLECT_BREAK 0x1000u
#define NDS_FTMAIN_PROCPARAMS_REFLECT_HIT 0x2000u
#define NDS_FTMAIN_PROCPARAMS_REFLECT_SOUND 0x4000u
#define NDS_FTMAIN_PROCPARAMS_ABSORB 0x8000u
#define NDS_FTMAIN_PROCPARAMS_DAMAGE_STATUS_SELECT 0x10000u
#define NDS_FTMAIN_PROCPARAMS_DAMAGE_STATUS_SETUP 0x20000u
#define NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_CATCH 0x40000u
#define NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_RELEASE 0x80000u
#define NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_CAPTURE 0x100000u
#define NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_CATCH_ZERO 0x200000u
#define NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_CAPTURE_ZERO 0x400000u
#define NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_CATCH_STATS 0x800000u
#define NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_CAPTURE_STATS 0x1000000u
#define NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_CAPTURE_RELEASE 0x2000000u
#define NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_CAPTURE_NODAMAGE 0x4000000u
#define NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_CATCH_NODAMAGE 0x8000000u
#define NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_TAIL_COLANIM 0x10000000u
#define NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_TAIL_STATUS 0x20000000u
#define NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_ITEM_RESIST 0x40000000u
#define NDS_FTMAIN_PROCPARAMS_DAMAGE_UPDATE_MAIN_ITEM_DROP 0x80000000u
#define NDS_FTMAIN_PROCPARAMS_SELECTED_REQUIRED_MASK 0xfffdf3ffu
#define NDS_FTMAIN_PROCPARAMS_REBOUND_STATUS 0x1u
#define NDS_FTMAIN_PROCPARAMS_REBOUND_CALLBACKS 0x2u
#define NDS_FTMAIN_PROCPARAMS_REBOUND_VECTOR 0x4u
#define NDS_FTMAIN_PROCPARAMS_REBOUND_HITLAG 0x8u
#define NDS_FTMAIN_PROCPARAMS_REBOUND_CLEAR 0x10u
#define NDS_FTMAIN_DAMAGE_SLEEP_ELEMENT 0x1u
#define NDS_FTMAIN_DAMAGE_SLEEP_ZERO_KNOCKBACK 0x2u
#define NDS_FTMAIN_DAMAGE_SLEEP_FURA_COLANIM 0x4u
#define NDS_FTMAIN_DAMAGE_SLEEP_STATUS 0x8u
#define NDS_FTMAIN_DAMAGE_SLEEP_MOTION 0x10u
#define NDS_FTMAIN_DAMAGE_SLEEP_CLIFF_WAIT 0x20u
#define NDS_FTMAIN_DAMAGE_SLEEP_RESTORE 0x40u
#define NDS_DAMAGE_ITEM_BYPASS_LIGHT 0x1u
#define NDS_DAMAGE_ITEM_BYPASS_HEAVY_NON_DK 0x2u
#define NDS_DAMAGE_ITEM_BYPASS_TAIL_COLANIM 0x4u
#define NDS_DAMAGE_ITEM_BYPASS_KEEP_ITEM 0x8u
#define NDS_DAMAGE_ITEM_BYPASS_RESTORE 0x10u
#define NDS_DAMAGE_ITEM_HEAVY_BRANCH 0x1u
#define NDS_DAMAGE_ITEM_HEAVY_RESIST 0x2u
#define NDS_DAMAGE_ITEM_HEAVY_DROP 0x4u
#define NDS_DAMAGE_ITEM_HEAVY_RETURN 0x8u
#define NDS_DAMAGE_ITEM_HEAVY_RESTORE 0x10u
#define NDS_DAMAGE_KIND_PRESERVE_BEFORE 0x1u
#define NDS_DAMAGE_KIND_PRESERVE_AFTER 0x2u
#define NDS_DAMAGE_KIND_PRESERVE_RESTORE 0x4u
#define NDS_DAMAGE_KIND_PRESERVE_ORIGINAL_INIT 0x8u
#define NDS_DAMAGE_KIND_PRESERVE_ORIGINAL_GOTO 0x10u
#define NDS_DAMAGE_KIND_PROCPARAMS_TWISTER 0x20u
#define NDS_DAMAGE_KIND_PROCPARAMS_TRAP 0x40u
#define NDS_DAMAGE_VOICE_ATTR 0x1u
#define NDS_DAMAGE_VOICE_THRESHOLD_CALL 0x2u
#define NDS_DAMAGE_VOICE_FORCE_CALL 0x4u
#define NDS_DAMAGE_VOICE_RESTORE 0x8u
#define NDS_DAMAGE_FLYROLL_PERCENT 0x1u
#define NDS_DAMAGE_FLYROLL_ANGLE 0x2u
#define NDS_DAMAGE_FLYROLL_RNG 0x4u
#define NDS_DAMAGE_FLYROLL_STATUS 0x8u
#define NDS_DAMAGE_FLYROLL_RESTORE 0x10u
#define NDS_DAMAGE_FLYTOP_LEVEL 0x1u
#define NDS_DAMAGE_FLYTOP_ANGLE 0x2u
#define NDS_DAMAGE_FLYTOP_STATUS 0x4u
#define NDS_DAMAGE_FLYTOP_RESTORE 0x8u
#define NDS_DAMAGE_REPLACE_STATUS 0x1u
#define NDS_DAMAGE_REPLACE_ELECTRIC 0x2u
#define NDS_DAMAGE_REPLACE_PASSIVE 0x4u
#define NDS_DAMAGE_REPLACE_RESTORE 0x8u
#define NDS_DAMAGE_REPLACE_DISPATCH 0x10u
#define NDS_DAMAGE_REPLACE_HITLAG_BLOCK 0x20u
#define NDS_DAMAGE_STATUS_SELECT_LEVEL 0x1u
#define NDS_DAMAGE_STATUS_SELECT_GROUND 0x2u
#define NDS_DAMAGE_STATUS_SELECT_AIR 0x4u
#define NDS_DAMAGE_STATUS_SELECT_ELECTRIC 0x8u
#define NDS_DAMAGE_STATUS_SELECT_PARKED 0x10u
#define NDS_DAMAGE_STATUS_SETUP_INIT 0x1u
#define NDS_DAMAGE_STATUS_SETUP_STATUS 0x2u
#define NDS_DAMAGE_STATUS_SETUP_MOTION 0x4u
#define NDS_DAMAGE_STATUS_SETUP_CALLBACKS 0x8u
#define NDS_DAMAGE_STATUS_SETUP_HITSTUN 0x10u
#define NDS_DAMAGE_STATUS_SETUP_UPDATE 0x20u
#define NDS_DAMAGE_STATUS_SETUP_RESTORE 0x40u
#define NDS_DAMAGE_STATUS_SETUP_PHYSICS 0x80u
#define NDS_DAMAGE_STATUS_SETUP_INTERRUPT 0x100u
#define NDS_DAMAGE_STATUS_SETUP_EXPIRE 0x200u
#define NDS_DAMAGE_STATUS_SETUP_MAP 0x400u
#define NDS_DAMAGE_STATUS_SETUP_MAP_FLOOR 0x800u
#define NDS_DAMAGE_STATUS_SETUP_MAP_CLIFF 0x1000u
#define NDS_DAMAGE_STATUS_SETUP_FALL_PHYSICS 0x2000u
#define NDS_DAMAGE_STATUS_SETUP_FASTFALL 0x4000u
#define NDS_DAMAGE_STATUS_SETUP_AIR_MAP 0x8000u
#define NDS_DAMAGE_STATUS_SETUP_FLYROLL_PHYSICS 0x10000u
#define NDS_DAMAGE_STATUS_SETUP_KNOCKBACK_INVINCIBLE 0x20000u
#define NDS_DAMAGE_STATUS_SETUP_LAGUPDATE 0x40000u
#define NDS_DAMAGE_STATUS_SETUP_HITLAG_LIFECYCLE 0x80000u
#define NDS_DAMAGE_STATUS_SETUP_PUBLIC 0x100000u
#define NDS_DAMAGE_STATUS_SETUP_COLANIM 0x200000u
#define NDS_DAMAGE_STATUS_SETUP_SCREENFLASH 0x400000u
#define NDS_DAMAGE_STATUS_SETUP_RUMBLE 0x800000u
#define NDS_DAMAGE_STATUS_SETUP_DUST 0x1000000u
#define NDS_DAMAGE_STATUS_SETUP_PLAYERTAG 0x2000000u
#define NDS_DAMAGE_STATUS_SETUP_ATTACKER 0x4000000u
#define NDS_DAMAGE_STATUS_SETUP_PROC_PASSIVE 0x8000000u
#define NDS_DAMAGE_STATUS_SETUP_PROC_PASSIVE_TICK 0x10000000u
#define NDS_DAMAGE_STATUS_SETUP_PROC_PASSIVE_STATUS 0x20000000u
#define NDS_DAMAGE_STATUS_SETUP_SLEEP_STATUS 0x40000000u
#define NDS_DAMAGE_STATUS_SETUP_HAMMER_INTERRUPT 0x80000000u
#define NDS_DAMAGE_COMMON_PHYSICS_GROUND 0x1u
#define NDS_DAMAGE_COMMON_PHYSICS_AIR_FRICTION 0x2u
#define NDS_DAMAGE_COMMON_PHYSICS_AIR_DRIFT 0x4u
#define NDS_DAMAGE_COMMON_PHYSICS_CLEAR_ATTACK 0x8u
#define NDS_DAMAGE_COMMON_PHYSICS_RESTORE 0x10u
#define NDS_DAMAGE_COMMON_PHYSICS_ORIGINAL 0x20u
#define NDS_DAMAGE_COMMON_CALLBACK_GROUND_UPDATE 0x1u
#define NDS_DAMAGE_COMMON_CALLBACK_AIR_UPDATE 0x2u
#define NDS_DAMAGE_COMMON_CALLBACK_GROUND_INTERRUPT 0x4u
#define NDS_DAMAGE_COMMON_CALLBACK_AIR_INTERRUPT 0x8u
#define NDS_DAMAGE_COMMON_CALLBACK_HAMMER_GROUND 0x10u
#define NDS_DAMAGE_COMMON_CALLBACK_HAMMER_AIR 0x20u
#define NDS_DAMAGE_COMMON_CALLBACK_RESTORE 0x40u
#define NDS_DAMAGE_COMMON_CALLBACK_GROUND_STAY 0x80u
#define NDS_DAMAGE_COMMON_CALLBACK_AIR_STAY 0x100u
#define NDS_DAMAGE_COMMON_CALLBACK_GROUND_UPDATE_ORIGINAL 0x200u
#define NDS_DAMAGE_COMMON_CALLBACK_AIR_UPDATE_ORIGINAL 0x400u
#define NDS_DAMAGE_COMMON_CALLBACK_COMMON_INTERRUPT_ORIGINAL 0x800u
#define NDS_DAMAGE_COMMON_CALLBACK_AIR_INTERRUPT_ORIGINAL 0x1000u
#define NDS_DAMAGE_COMMON_CALLBACK_FALL_INTERRUPT 0x2000u
#define NDS_DAMAGE_AIR_MAP_WALL_COLLISION 0x1u
#define NDS_DAMAGE_AIR_MAP_WALL_HELPER 0x2u
#define NDS_DAMAGE_AIR_MAP_WALL_SHORT_CIRCUIT 0x4u
#define NDS_DAMAGE_AIR_MAP_WALL_KNOCKBACK 0x8u
#define NDS_DAMAGE_AIR_MAP_WALL_RESTORE 0x10u
#define NDS_DAMAGE_AIR_MAP_WALL_ORIGINAL 0x20u
#define NDS_DAMAGE_KNOCKBACK_ANGLE_FIXED 0x1u
#define NDS_DAMAGE_KNOCKBACK_ANGLE_AIR_361 0x2u
#define NDS_DAMAGE_KNOCKBACK_ANGLE_GROUND_LOW_361 0x4u
#define NDS_DAMAGE_KNOCKBACK_ANGLE_GROUND_HIGH_361 0x8u
#define NDS_DAMAGE_KNOCKBACK_ANGLE_GROUND_CAP_361 0x10u
#define NDS_DAMAGE_KNOCKBACK_ANGLE_ORIGINAL 0x20u
#define NDS_DAMAGE_FALL_INTERRUPT_CALL 0x1u
#define NDS_DAMAGE_FALL_INTERRUPT_SPECIAL 0x2u
#define NDS_DAMAGE_FALL_INTERRUPT_ATTACK 0x4u
#define NDS_DAMAGE_FALL_INTERRUPT_JUMP 0x8u
#define NDS_DAMAGE_FALL_INTERRUPT_HAMMER 0x10u
#define NDS_DAMAGE_FALL_INTERRUPT_RESTORE 0x20u
#define NDS_DAMAGE_DUST_LOW 0x1u
#define NDS_DAMAGE_DUST_MID_LOW 0x2u
#define NDS_DAMAGE_DUST_MID 0x4u
#define NDS_DAMAGE_DUST_MID_HIGH 0x8u
#define NDS_DAMAGE_DUST_HIGH 0x10u
#define NDS_DAMAGE_DUST_DEFAULT_AIR 0x20u
#define NDS_DAMAGE_DUST_RESTORE 0x40u
#define NDS_DAMAGE_DUST_ORIGINAL 0x80u
#define NDS_DAMAGE_DUST_UPDATE_DEC 0x1u
#define NDS_DAMAGE_DUST_UPDATE_EFFECT 0x2u
#define NDS_DAMAGE_DUST_UPDATE_RESET 0x4u
#define NDS_DAMAGE_DUST_UPDATE_RESTORE 0x8u
#define NDS_DAMAGE_DUST_UPDATE_ORIGINAL 0x10u
#define NDS_DAMAGE_HITSTUN_PUBLIC_DEC 0x1u
#define NDS_DAMAGE_HITSTUN_PUBLIC_TRANSFER 0x2u
#define NDS_DAMAGE_HITSTUN_PUBLIC_RESTORE 0x4u
#define NDS_DAMAGE_HITSTUN_PUBLIC_ORIGINAL 0x8u
#define NDS_DAMAGE_COLANIM_FIRE 0x1u
#define NDS_DAMAGE_COLANIM_ELECTRIC 0x2u
#define NDS_DAMAGE_COLANIM_FREEZE 0x4u
#define NDS_DAMAGE_COLANIM_NORMAL 0x8u
#define NDS_DAMAGE_COLANIM_RESTORE 0x10u
#define NDS_DAMAGE_COLANIM_ORIGINAL 0x20u
#define NDS_DAMAGE_COLANIM_UPDATE_DIRECT 0x1u
#define NDS_DAMAGE_COLANIM_UPDATE_SET 0x2u
#define NDS_DAMAGE_COLANIM_UPDATE_NOOP 0x4u
#define NDS_DAMAGE_COLANIM_UPDATE_RESTORE 0x8u
#define NDS_DAMAGE_COLANIM_UPDATE_ORIGINAL 0x10u
#define NDS_DAMAGE_INVINCIBLE_HITLAG_BLOCK 0x1u
#define NDS_DAMAGE_INVINCIBLE_FLAG_BLOCK 0x2u
#define NDS_DAMAGE_INVINCIBLE_SET 0x4u
#define NDS_DAMAGE_INVINCIBLE_RESTORE 0x8u
#define NDS_DAMAGE_INVINCIBLE_ORIGINAL 0x10u
#define NDS_DAMAGE_LAGUPDATE_HITLAG_BLOCK 0x1u
#define NDS_DAMAGE_LAGUPDATE_STICK_BLOCK 0x2u
#define NDS_DAMAGE_LAGUPDATE_TAP_BLOCK 0x4u
#define NDS_DAMAGE_LAGUPDATE_APPLY 0x8u
#define NDS_DAMAGE_LAGUPDATE_RESTORE 0x10u
#define NDS_DAMAGE_LAGUPDATE_ORIGINAL 0x20u
#define NDS_DAMAGE_FLASH_LOW_NOOP 0x1u
#define NDS_DAMAGE_FLASH_FIRE 0x2u
#define NDS_DAMAGE_FLASH_ELECTRIC 0x4u
#define NDS_DAMAGE_FLASH_FREEZE 0x8u
#define NDS_DAMAGE_FLASH_NORMAL 0x10u
#define NDS_DAMAGE_FLASH_RESTORE 0x20u
#define NDS_DAMAGE_FLASH_ORIGINAL 0x40u
#define NDS_DAMAGE_PUBLIC_ANGLE_REDUCE 0x1u
#define NDS_DAMAGE_PUBLIC_TARGET_RESET 0x2u
#define NDS_DAMAGE_PUBLIC_FORCE 0x4u
#define NDS_DAMAGE_PUBLIC_RESTORE 0x8u
#define NDS_DAMAGE_PUBLIC_NO_FORCE 0x10u
#define NDS_DAMAGE_PUBLIC_ORIGINAL 0x20u
#define NDS_DAMAGE_LEVEL_LOW 0x1u
#define NDS_DAMAGE_LEVEL_MID 0x2u
#define NDS_DAMAGE_LEVEL_HIGH 0x4u
#define NDS_DAMAGE_LEVEL_FLY 0x8u
#define NDS_DAMAGE_LEVEL_ORIGINAL 0x10u
#define NDS_DAMAGE_HOLD_RESIST_SLEEP_FALSE 0x1u
#define NDS_DAMAGE_HOLD_RESIST_ZERO_TRUE 0x2u
#define NDS_DAMAGE_HOLD_RESIST_PAUSED_TRUE 0x4u
#define NDS_DAMAGE_HOLD_RESIST_DONKEY_TRUE 0x8u
#define NDS_DAMAGE_HOLD_RESIST_DEFAULT_FALSE 0x10u
#define NDS_DAMAGE_HOLD_RESIST_KEEP_TRUE 0x20u
#define NDS_DAMAGE_HOLD_RESIST_KEEP_FALSE 0x40u
#define NDS_DAMAGE_HOLD_RESIST_RESTORE 0x80u
#define NDS_DAMAGE_UPDATE_CATCH_RESIST_ZERO_COLANIM 0x1u
#define NDS_DAMAGE_UPDATE_CATCH_RESIST_PAUSED_COLANIM 0x2u
#define NDS_DAMAGE_UPDATE_CATCH_RESIST_STATUS 0x4u
#define NDS_DAMAGE_UPDATE_CATCH_RESIST_RESTORE 0x8u
#define NDS_DAMAGE_UPDATE_CATCH_RESIST_ORIGINAL 0x10u
#define NDS_FTMAIN_HITLOG_NUM_MAX 10u

static u32 sNdsFighterDashRunProcParamsLagStartCount;
static FTHitLog sNdsFighterDashRunHitLogs[NDS_FTMAIN_HITLOG_NUM_MAX];
static u32 sNdsFighterDashRunHitLogID;
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
    u32 effect_count_before = gNdsStageMPLiveHitDamageLoopAttackClashEffectCount;

    battleship_ftMainUpdateAttackStatFighter(other_fp, other_hit, this_fp,
                                             this_hit, other_gobj, this_gobj);
    if ((ndsFighterMarioFoxStageMPLiveHitDamageLoopProofEnabled() != FALSE) &&
        (gNdsStageMPLiveHitDamageLoopAttackClashEffectCount ==
            effect_count_before) &&
        (other_hit != NULL) && (this_hit != NULL))
    {
        if ((this_hit->damage - 10) < other_hit->damage)
        {
            gNdsStageMPLiveHitDamageLoopAttackClashEffectCount++;
        }
        if ((other_hit->damage - 10) < this_hit->damage)
        {
            gNdsStageMPLiveHitDamageLoopAttackClashEffectCount++;
        }
    }
}

void ftMainUpdateShieldStatFighter(FTStruct *attacker_fp,
                                   FTAttackColl *attack_coll,
                                   FTStruct *victim_fp,
                                   GObj *attacker_gobj,
                                   GObj *victim_gobj)
{
    s32 shield_damage_before = (victim_fp != NULL) ?
        victim_fp->shield_damage : 0;
    s32 effect_size = (attack_coll != NULL) ? attack_coll->damage : 0;

    battleship_ftMainUpdateShieldStatFighter(attacker_fp, attack_coll,
                                             victim_fp, attacker_gobj,
                                             victim_gobj);
    if ((ndsFighterMarioFoxStageMPLiveHitDamageLoopProofEnabled() != FALSE) &&
        (attacker_fp != NULL) && (attack_coll != NULL) &&
        (victim_fp != NULL))
    {
        if (victim_fp->shield_damage >= effect_size)
        {
            gNdsStageMPLiveHitDamageLoopShieldPlayer = attacker_fp->player;
        }
        if ((victim_fp->shield_damage > shield_damage_before) ||
            (victim_fp->shield_damage_total != 0) ||
            (attacker_fp->attack_shield_push != 0))
        {
            gNdsStageMPLiveHitDamageLoopShieldEffectCount++;
            gNdsStageMPLiveHitDamageLoopShieldEffectSize = effect_size;
        }
    }
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
#if NDS_TICK_HUD
    u32 catch_start = cpuGetTiming();
#endif

    battleship_ftMainProcSearchCatch(fighter_gobj);
#if NDS_TICK_HUD
    gNdsTickHudSrcCatchTicks += cpuGetTiming() - catch_start;
#endif
}

void ftMainSearchHitItem(GObj *fighter_gobj)
{
    battleship_ftMainSearchHitItem(fighter_gobj);
    if ((fighter_gobj != NULL) &&
        (ndsFighterMarioFoxStageMPLiveHitDamageLoopProofEnabled() != FALSE))
    {
        gNdsStageMPLiveHitDamageLoopFullCollisionDeferredCount++;
    }
}

void ftMainSearchHitWeapon(GObj *fighter_gobj)
{
    battleship_ftMainSearchHitWeapon(fighter_gobj);
    if ((fighter_gobj != NULL) &&
        (ndsFighterMarioFoxStageMPLiveHitDamageLoopProofEnabled() != FALSE))
    {
        gNdsStageMPLiveHitDamageLoopFullCollisionDeferredCount++;
    }
}

void ftMainSearchGroundHit(GObj *fighter_gobj)
{
    battleship_ftMainSearchGroundHit(fighter_gobj);
    if ((fighter_gobj != NULL) &&
        (ndsFighterMarioFoxStageMPLiveHitDamageLoopProofEnabled() != FALSE))
    {
        gNdsStageMPLiveHitDamageLoopFullCollisionDeferredCount++;
    }
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
#if NDS_TICK_HUD
    u32 hit_start = cpuGetTiming();
#endif

    NDS_FREEZE_DIAGNOSTICS_MARK(NDS_FREEZE_BREADCRUMB_HIT_SEARCH);
    battleship_ftMainProcSearchHitAll(fighter_gobj);
#if NDS_TICK_HUD
    gNdsTickHudSrcHitDetectTicks += cpuGetTiming() - hit_start;
#endif
}

void ftMainProcParams(GObj *fighter_gobj)
{
    /* Cycle 86 SPRM. Fighter proc priority 0 (decomp ft/ftmanager.c:863), the
     * last of the six and the one that carries the animation/event interpreter
     * and the status/param update. The audit hook is deliberately INSIDE the
     * bracket: it is compiled out of the measuring ROM
     * (NDS_FIGHTER_ANIM_AUDIT 0) so it costs nothing there, and if it is ever
     * enabled its cost belongs to this owner rather than silently to SGCO. */
#if NDS_TICK_HUD
    u32 params_start = cpuGetTiming();
#endif

    battleship_ftMainProcParams(fighter_gobj);
#if NDS_FIGHTER_ANIM_AUDIT
    ndsFighterAnimAuditUpdate(fighter_gobj);
#endif
#if NDS_TICK_HUD
    gNdsTickHudSrcParamsTicks += cpuGetTiming() - params_start;
#endif
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
#if NDS_TASK108_SITR_CALLBACK_CENSUS
#define NDS_TASK108_SITR_ENTRY_CAPACITY 512u
#define NDS_TASK108_SITR_CONTEXT_COUNT 4u

enum
{
    NDS_TASK108_SITR_SLOT_PASSIVE = 0,
    NDS_TASK108_SITR_SLOT_UPDATE = 1,
    NDS_TASK108_SITR_SLOT_INTERRUPT = 2,
    NDS_TASK108_SITR_SLOT_COUNT = 3
};

typedef void (*NDS_TASK108_SITR_PROC)(GObj *);

typedef struct NdsTask108SitrEntry
{
    u32 fkind;
    u32 status_id;
    u32 slot;
    u32 target;
    u32 calls;
    u32 ticks;
} NdsTask108SitrEntry;

typedef struct NdsTask108SitrContext
{
    GObj *fighter_gobj;
    NDS_TASK108_SITR_PROC original[NDS_TASK108_SITR_SLOT_COUNT];
    u8 active;
    u8 in_callback;
    u8 refresh_pending;
    u8 reserved;
} NdsTask108SitrContext;

__attribute__((used)) volatile NdsTask108SitrEntry
    gNdsTask108SitrEntries[NDS_TASK108_SITR_ENTRY_CAPACITY];
__attribute__((used)) volatile u32 gNdsTask108SitrEntryCount;
__attribute__((used)) volatile u32 gNdsTask108SitrEntryOverflow;
__attribute__((used)) volatile u32 gNdsTask108SitrTotalCalls;
__attribute__((used)) volatile u32 gNdsTask108SitrTotalTicks;
__attribute__((used)) volatile u32 gNdsTask108SitrRefreshCount;

static NdsTask108SitrContext
    sNdsTask108SitrContexts[NDS_TASK108_SITR_CONTEXT_COUNT];

static void ndsTask108SitrPassive(GObj *fighter_gobj);
static void ndsTask108SitrUpdate(GObj *fighter_gobj);
static void ndsTask108SitrInterrupt(GObj *fighter_gobj);

static NdsTask108SitrContext *ndsTask108SitrContextFor(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    u32 i;

    if ((fp != NULL) && (fp->player >= 0) &&
        ((u32)fp->player < NDS_TASK108_SITR_CONTEXT_COUNT))
    {
        return &sNdsTask108SitrContexts[(u32)fp->player];
    }
    for (i = 0u; i < NDS_TASK108_SITR_CONTEXT_COUNT; i++)
    {
        if (sNdsTask108SitrContexts[i].fighter_gobj == fighter_gobj)
        {
            return &sNdsTask108SitrContexts[i];
        }
    }
    return NULL;
}

static void ndsTask108SitrRestore(NdsTask108SitrContext *context,
                                  FTStruct *fp)
{
    if ((context == NULL) || (fp == NULL))
    {
        return;
    }
    if (fp->proc_passive == ndsTask108SitrPassive)
    {
        fp->proc_passive = context->original[NDS_TASK108_SITR_SLOT_PASSIVE];
    }
    if (fp->proc_update == ndsTask108SitrUpdate)
    {
        fp->proc_update = context->original[NDS_TASK108_SITR_SLOT_UPDATE];
    }
    if (fp->proc_interrupt == ndsTask108SitrInterrupt)
    {
        fp->proc_interrupt = context->original[NDS_TASK108_SITR_SLOT_INTERRUPT];
    }
}

static void ndsTask108SitrWrap(NdsTask108SitrContext *context, FTStruct *fp)
{
    if ((context == NULL) || (fp == NULL) || (context->active == FALSE))
    {
        return;
    }
    if (fp->proc_passive != ndsTask108SitrPassive)
    {
        context->original[NDS_TASK108_SITR_SLOT_PASSIVE] = fp->proc_passive;
    }
    if (fp->proc_update != ndsTask108SitrUpdate)
    {
        context->original[NDS_TASK108_SITR_SLOT_UPDATE] = fp->proc_update;
    }
    if (fp->proc_interrupt != ndsTask108SitrInterrupt)
    {
        context->original[NDS_TASK108_SITR_SLOT_INTERRUPT] = fp->proc_interrupt;
    }
    fp->proc_passive =
        (context->original[NDS_TASK108_SITR_SLOT_PASSIVE] != NULL) ?
            ndsTask108SitrPassive : NULL;
    fp->proc_update =
        (context->original[NDS_TASK108_SITR_SLOT_UPDATE] != NULL) ?
            ndsTask108SitrUpdate : NULL;
    fp->proc_interrupt =
        (context->original[NDS_TASK108_SITR_SLOT_INTERRUPT] != NULL) ?
            ndsTask108SitrInterrupt : NULL;
}

static void ndsTask108SitrRecord(u32 fkind, u32 status_id, u32 slot,
                                 NDS_TASK108_SITR_PROC target, u32 ticks)
{
    u32 i;
    u32 target_u32 = (u32)(uintptr_t)target;

    gNdsTask108SitrTotalCalls++;
    gNdsTask108SitrTotalTicks += ticks;
    for (i = 0u; i < gNdsTask108SitrEntryCount; i++)
    {
        volatile NdsTask108SitrEntry *entry = &gNdsTask108SitrEntries[i];
        if ((entry->fkind == fkind) && (entry->status_id == status_id) &&
            (entry->slot == slot) && (entry->target == target_u32))
        {
            entry->calls++;
            entry->ticks += ticks;
            return;
        }
    }
    if (gNdsTask108SitrEntryCount >= NDS_TASK108_SITR_ENTRY_CAPACITY)
    {
        gNdsTask108SitrEntryOverflow++;
        return;
    }
    i = gNdsTask108SitrEntryCount++;
    gNdsTask108SitrEntries[i].fkind = fkind;
    gNdsTask108SitrEntries[i].status_id = status_id;
    gNdsTask108SitrEntries[i].slot = slot;
    gNdsTask108SitrEntries[i].target = target_u32;
    gNdsTask108SitrEntries[i].calls = 1u;
    gNdsTask108SitrEntries[i].ticks = ticks;
}

static void ndsTask108SitrDispatch(GObj *fighter_gobj, u32 slot)
{
    NdsTask108SitrContext *context = ndsTask108SitrContextFor(fighter_gobj);
    FTStruct *fp = ftGetStruct(fighter_gobj);
    NDS_TASK108_SITR_PROC target;
    u32 fkind;
    u32 status_id;
    u32 start;
    u32 ticks;

    if ((context == NULL) || (fp == NULL) || (slot >= NDS_TASK108_SITR_SLOT_COUNT))
    {
        return;
    }
    target = context->original[slot];
    if (target == NULL)
    {
        return;
    }
    fkind = (u32)fp->fkind;
    status_id = (u32)fp->status_id;

    /* The interposition is observational. Restore all three original pointers
     * while source code executes so callback-pointer identity remains exact. */
    ndsTask108SitrRestore(context, fp);
    context->in_callback = TRUE;
    context->refresh_pending = FALSE;
    start = cpuGetTiming();
    target(fighter_gobj);
    ticks = cpuGetTiming() - start;
    context->in_callback = FALSE;
    ndsTask108SitrRecord(fkind, status_id, slot, target, ticks);

    fp = ftGetStruct(fighter_gobj);
    if ((fp != NULL) && (context->active != FALSE))
    {
        ndsTask108SitrWrap(context, fp);
    }
}

static void ndsTask108SitrPassive(GObj *fighter_gobj)
{
    ndsTask108SitrDispatch(fighter_gobj, NDS_TASK108_SITR_SLOT_PASSIVE);
}

static void ndsTask108SitrUpdate(GObj *fighter_gobj)
{
    ndsTask108SitrDispatch(fighter_gobj, NDS_TASK108_SITR_SLOT_UPDATE);
}

static void ndsTask108SitrInterrupt(GObj *fighter_gobj)
{
    ndsTask108SitrDispatch(fighter_gobj, NDS_TASK108_SITR_SLOT_INTERRUPT);
}

void ndsTask108SitrRefreshCallbacks(GObj *fighter_gobj)
{
    NdsTask108SitrContext *context = ndsTask108SitrContextFor(fighter_gobj);
    FTStruct *fp;

    if ((context == NULL) || (context->active == FALSE) ||
        (context->fighter_gobj != fighter_gobj))
    {
        return;
    }
    if (context->in_callback != FALSE)
    {
        context->refresh_pending = TRUE;
        return;
    }
    fp = ftGetStruct(fighter_gobj);
    if (fp != NULL)
    {
        gNdsTask108SitrRefreshCount++;
        ndsTask108SitrWrap(context, fp);
    }
}

static void ndsTask108SitrBegin(GObj *fighter_gobj)
{
    NdsTask108SitrContext *context = ndsTask108SitrContextFor(fighter_gobj);
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if ((context == NULL) || (fp == NULL) ||
        ((fp->fkind != nFTKindMario) && (fp->fkind != nFTKindFox)))
    {
        return;
    }
    context->fighter_gobj = fighter_gobj;
    context->active = TRUE;
    context->in_callback = FALSE;
    context->refresh_pending = FALSE;
    ndsTask108SitrWrap(context, fp);
}

static void ndsTask108SitrEnd(GObj *fighter_gobj)
{
    NdsTask108SitrContext *context = ndsTask108SitrContextFor(fighter_gobj);
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if ((context == NULL) || (context->fighter_gobj != fighter_gobj))
    {
        return;
    }
    ndsTask108SitrRestore(context, fp);
    context->active = FALSE;
    context->in_callback = FALSE;
    context->refresh_pending = FALSE;
}
#endif

void ftMainProcUpdateInterrupt(GObj *fighter_gobj)
{
    /* SINT. Fighter proc priority 5, the first to run. The level-3 CPU AI is
     * nested INSIDE this proc (ftmain.c:1269, case nFTPlayerKindCom) and is
     * already bracketed as SCPU, so this span deliberately includes it and the
     * analyzer subtracts: SITR = SINT - SCPU. That keeps SCPU's banked series
     * unchanged and makes SITR's non-negativity the proof the nesting is real. */
#if NDS_TICK_HUD
    u32 interrupt_start = cpuGetTiming();
#endif

#if NDS_TASK108_SITR_CALLBACK_CENSUS
    ndsTask108SitrBegin(fighter_gobj);
#endif
    battleship_ftMainProcUpdateInterrupt(fighter_gobj);
#if NDS_TASK108_SITR_CALLBACK_CENSUS
    ndsTask108SitrEnd(fighter_gobj);
#endif
#if NDS_TICK_HUD
    gNdsTickHudSrcInterruptTicks += cpuGetTiming() - interrupt_start;
#endif
}

void ftMainProcPhysicsMapDefault(GObj *fighter_gobj)
{
    /* SPHD. Fighter proc priority 4. This and SPHC are mutually exclusive arms
     * of the same capture predicate (ftmain.c:1918-1937): each calls the shared
     * ftMainProcPhysicsMap only when its own condition holds, so bracketing both
     * outer procs is disjoint and exhaustive over the shared worker. In a match
     * without grabs SPHD does the work and SPHC is a two-field early-out, which
     * is the prediction these two are checked against. */
#if NDS_TICK_HUD
    u32 physics_default_start = cpuGetTiming();
#endif

    battleship_ftMainProcPhysicsMapDefault(fighter_gobj);
#if NDS_TICK_HUD
    gNdsTickHudSrcPhysicsDefaultTicks +=
        cpuGetTiming() - physics_default_start;
#endif
}

void ftMainProcPhysicsMapCapture(GObj *fighter_gobj)
{
    /* SPHC. Fighter proc priority 3, the captured arm of the pair above. */
#if NDS_TICK_HUD
    u32 physics_capture_start = cpuGetTiming();
#endif

    battleship_ftMainProcPhysicsMapCapture(fighter_gobj);
#if NDS_TICK_HUD
    gNdsTickHudSrcPhysicsCaptureTicks +=
        cpuGetTiming() - physics_capture_start;
#endif
}

#define NDS_FTMAIN_GROUND_OBSTACLE_COUNT 2u
typedef GRObstacle NDSFTMainGroundObstacle;
#define sNdsFTMainGroundObstacles sFTMainGroundObstacles
#define sNdsFTMainGroundObstaclesNum sFTMainGroundObstaclesNum
