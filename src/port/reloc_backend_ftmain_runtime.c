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

#define NDS_FTMAIN_GROUND_OBSTACLE_COUNT 2u
typedef GRObstacle NDSFTMainGroundObstacle;
#define sNdsFTMainGroundObstacles sFTMainGroundObstacles
#define sNdsFTMainGroundObstaclesNum sFTMainGroundObstaclesNum
