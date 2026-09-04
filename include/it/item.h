#ifndef SSB64_NDS_ITEM_H
#define SSB64_NDS_ITEM_H

#include <ft/fighter.h>
#include <PR/ultratypes.h>

#ifndef ITEM_ATKCOLL_NUM_MAX
#define ITEM_ATKCOLL_NUM_MAX 2
#endif

#define ITSTAR_INVINCIBLE_TIME 600
#define ITSTAR_WARN_BEGIN_FRAME (ITSTAR_INVINCIBLE_TIME - 480)
#define ITEM_REHIT_TIME_DEFAULT 16

/* BattleShip it/itvars.h:24-28, :30-34, :36-47 and :81-90, verbatim. The
 * two Star lines above predate this block and are the same values; the
 * rest arrive with the P2-5 batch that consumes them. Every literal is the
 * source's own -- ITSTAR_BGM_ID 0x2D and ITHAMMER_BGM_ID 0x2E are music
 * ids, not the gmMusicID enum, and are passed through as the source does. */
#define ITTOMATO_DAMAGE_HEAL 100
#define ITTOMATO_GRAVITY 1.2F
#define ITTOMATO_TVEL 100.0F
#define ITTOMATO_MAP_REBOUND_COMMON 0.3F
#define ITTOMATO_MAP_REBOUND_GROUND 0.5F

#define ITHEART_DAMAGE_HEAL 999
#define ITHEART_GRAVITY 0.25F
#define ITHEART_TVEL 30.0F
#define ITHEART_MAP_REBOUND_COMMON 0.1F
#define ITHEART_MAP_REBOUND_GROUND 0.0F

#define ITSTAR_COLANIM_ID 0x4A
#define ITSTAR_COLANIM_LENGTH 0
#define ITSTAR_BGM_ID 0x2D
#define ITSTAR_BGM_DURATION 10
#define ITSTAR_INTERACT_DELAY 16
#define ITSTAR_GRAVITY 1.2F
#define ITSTAR_TVEL 100.0F
#define ITSTAR_MAP_REBOUND_COMMON 1.0F
#define ITSTAR_VEL_X 8.0F
#define ITSTAR_BOUNCE_Y 50.0F

#define ITHAMMER_TIME 720
#define ITHAMMER_BGM_ID 0x2E
#define ITHAMMER_BGM_DURATION 20
#define ITHAMMER_WEAR_COLANIM_ID 0x4E
#define ITHAMMER_WEAR_COLANIM_LENGTH 0
#define ITHAMMER_WARN_BEGIN_FRAME (ITHAMMER_TIME - 600)
#define ITHAMMER_GRAVITY 1.5F
#define ITHAMMER_TVEL 120.0F
#define ITHAMMER_MAP_REBOUND_COMMON 0.5F
#define ITHAMMER_MAP_REBOUND_GROUND 0.2F

#define ITSWORD_GRAVITY 1.2F
#define ITSWORD_TVEL 100.0F
#define ITSWORD_MAP_REBOUND_COMMON 0.2F
#define ITSWORD_MAP_REBOUND_GROUND 0.5F

#define ITBAT_GRAVITY 1.5F
#define ITBAT_TVEL 120.0F
#define ITBAT_MAP_REBOUND_COMMON 0.2F
#define ITBAT_MAP_REBOUND_GROUND 0.5F

#define ITHARISEN_GRAVITY 1.0F
#define ITHARISEN_TVEL 80.0F
#define ITHARISEN_MAP_REBOUND_COMMON 0.0F
#define ITHARISEN_MAP_REBOUND_GROUND 0.3F

/* BattleShip it/itvars.h:9-22, :151-167 and :392-397, verbatim. The
 * containers and the pieces they burst into. ITCONTAINER_EFFECT_COUNT is a
 * line-continued 7 in the source (:151-153); it is 7 here. */
#define ITCONTAINER_EFFECT_COUNT 7
#define ITCONTAINER_GFX_LIFETIME 90

#define ITBOX_EXPLODE_FRAME_END 8
#define ITBOX_HEALTH_MAX 15
#define ITBOX_EXPLODE_SCALE 1.4F
#define ITBOX_GRAVITY 4.0F
#define ITBOX_TVEL 120.0F
#define ITBOX_MAP_REBOUND_COMMON 0.2F
#define ITBOX_MAP_REBOUND_GROUND 0.5F

#define ITCAPSULE_EXPLODE_FRAME_END 6
#define ITCAPSULE_GRAVITY 1.2F
#define ITCAPSULE_TVEL 100.0F
#define ITCAPSULE_MAP_REBOUND_COMMON 0.2F
#define ITCAPSULE_MAP_REBOUND_GROUND 0.4F
#define ITCAPSULE_EXPLODE_SCALE 1.4F

#define ITTARU_LIFETIME 360
#define ITTARU_EXPLODE_LIFETIME 8
#define ITTARU_DESPAWN_FLASH_START 60
#define ITTARU_HEALTH_MAX 10
#define ITTARU_MUL_VEL_X 1.4F
#define ITTARU_VEL_MIN 0.1F
#define ITTARU_ROLL_ROTATE_MUL 0.0045F
#define ITTARU_EXPLODE_SCALE 1.4F
#define ITTARU_GRAVITY 4.0F
#define ITTARU_TVEL 90.0F
#define ITTARU_MAP_REBOUND_COMMON 0.5F
#define ITTARU_MAP_REBOUND_GROUND 0.2F

#define ITEGG_EXPLODE_EFFECT_WAIT 8
#define ITEGG_EXPLODE_EFFECT_SCALE 1.4F
#define ITEGG_GRAVITY 1.2F
#define ITEGG_MAP_REBOUND_COMMON 0.2F
#define ITEGG_MAP_REBOUND_GROUND 0.5F
#define ITEGG_TVEL 100.0F

/* EVERY item's tuning constants, transcribed wholesale from BattleShip
 * it/itvars.h rather than a handful at a time.
 *
 * Landing items in batches meant discovering the same missing-macro build
 * failure once per batch, and each round trip cost a full rebuild to learn
 * a fact the source states plainly. The whole set is here in the source's
 * own order, so a constant referring to another (ITSTAR_WARN_BEGIN_FRAME
 * against ITSTAR_INVINCIBLE_TIME) still resolves. Values are the source's;
 * nothing is computed or rounded. */
#define ITLGUN_AMMO_MAX 16
#define ITLGUN_GRAVITY 1.5F
#define ITLGUN_TVEL 130.0F
#define ITLGUN_MAP_REBOUND_COMMON 0.2F
#define ITLGUN_MAP_REBOUND_GROUND 0.1F
#define ITLGUN_AMMO_VEL_X 300.0F
#define ITLGUN_AMMO_STEP_SCALE_X 10.0F
#define ITLGUN_AMMO_CLAMP_SCALE_X (160.0F / 3.0F)
#define ITFFLOWER_AMMO_MAX 60
#define ITFFLOWER_AMMO_LIFETIME 30
#define ITFFLOWER_AMMO_VEL 30.0F
#define ITFFLOWER_GRAVITY 1.2F
#define ITFFLOWER_TVEL 100.0F
#define ITFFLOWER_MAP_REBOUND_COMMON 0.0F
#define ITFFLOWER_MAP_REBOUND_GROUND 0.5F
#define ITMSBOMB_EXPLODE_LIFETIME 16
#define ITMSBOMB_DETECT_FIGHTER_DELAY 100
#define ITMSBOMB_DETECT_FIGHTER_RADIUS SQUARE(400.0F)
#define ITMSBOMB_GRAVITY 1.5F
#define ITMSBOMB_TVEL 80.0F
#define ITMSBOMB_MAP_REBOUND_COMMON 0.4F
#define ITMSBOMB_MAP_REBOUND_GROUND 0.3F
#define ITMSBOMB_COLL_SIZE 30.0F
#define ITMSBOMB_EXPLODE_SCALE 1.2F
#define ITBOMBHEI_EXPLODE_COLANIM_ID 0x4D
#define ITBOMBHEI_EXPLODE_COLANIM_DURATION 90
#define ITBOMBHEI_EXPLODE_LIFETIME 6
#define ITBOMBHEI_WALK_WAIT 180
#define ITBOMBHEI_FLASH_WAIT 480
#define ITBOMBHEI_SMOKE_WAIT 4
#define ITBOMBHEI_EXPLODE_WAIT 90.0F
#define ITBOMBHEI_WALK_VEL_X 24.0F
#define ITBOMBHEI_GRAVITY 1.2F
#define ITBOMBHEI_TVEL 100.0F
#define ITBOMBHEI_MAP_REBOUND_COMMON 0.4F
#define ITBOMBHEI_MAP_REBOUND_GROUND 0.3F
#define ITBOMBHEI_EXPLODE_SCALE 1.4F
#define ITSTARROD_AMMO_MAX 20
#define ITSTARROD_GRAVITY 1.2F
#define ITSTARROD_TVEL 100.0F
#define ITSTARROD_MAP_REBOUND_COMMON 0.2F
#define ITSTARROD_MAP_REBOUND_GROUND 0.5F
#define ITSTARROD_AMMO_SMASH_VEL_X 120.0F
#define ITSTARROD_AMMO_TILTVEL_X 80.0F
#define ITSTARROD_AMMO_SMASH_LIFETIME 30.0F
#define ITSTARROD_AMMO_TILT_LIFETIME 30.0F
#define ITGSHELL_LIFETIME 240
#define ITGSHELL_HEALTH_MAX 4
#define ITGSHELL_EFFECT_SPAWN_INT 8
#define ITGSHELL_DAMAGE_ALL_WAIT 32
#define ITGSHELL_CLAMP_VEL_X 90.0F
#define ITGSHELL_REBOUND_MUL_X 0.125F
#define ITGSHELL_REBOUND_VEL_Y 38.0F
#define ITGSHELL_STOP_VEL_X 12.0F
#define ITGSHELL_DAMAGE_MUL_NORMAL 8.0F
#define ITGSHELL_DAMAGE_MUL_ADD 3.0F
#define ITGSHELL_GRAVITY 1.2F
#define ITGSHELL_TVEL 100.0F
#define ITGSHELL_MAP_REBOUND_COMMON 0.2F
#define ITGSHELL_MAP_REBOUND_GROUND 0.5F
#define ITRSHELL_INTERACT_MAX 24
#define ITRSHELL_LIFETIME 480
#define ITRSHELL_HEALTH_MAX 4
#define ITRSHELL_EFFECT_SPAWN_INT 8
#define ITRSHELL_DAMAGE_ALL_WAIT 16
#define ITRSHELL_CLAMP_VEL_X 70.0F
#define ITRSHELL_CLAMP_AIR_X 90.0F
#define ITRSHELL_HIT_INITVEL_X 8.0F
#define ITRSHELL_MUL_VEL_X 1.2F
#define ITRSHELL_STOP_VEL_X 8.0F
#define ITRSHELL_ADD_VEL_X 60.0F
#define ITRSHELL_RECOIL_VEL_X (-8.0F)
#define ITRSHELL_RECOIL_MUL_X 0.7F
#define ITRSHELL_DAMAGE_MUL_NORMAL 10.0F
#define ITRSHELL_GRAVITY 1.2F
#define ITRSHELL_TVEL 100.0F
#define ITRSHELL_MAP_REBOUND_COMMON 0.25F
#define ITRSHELL_MAP_REBOUND_GROUND 0.5F
#define ITGLUCKY_EGG_SPAWN_COUNT 1
#define ITGLUCKY_EGG_SPAWN_OFF_X 200.0F
#define ITGLUCKY_EGG_SPAWN_OFF_Y 200.0F
#define ITGLUCKY_EGG_SPAWN_MUL 8.0F
#define ITGLUCKY_EGG_SPAWN_ADD_X 8.0F
#define ITGLUCKY_EGG_SPAWN_ADD_Y 30.0F
#define ITGLUCKY_EGG_SPAWN_BEGIN 80.0F
#define ITGLUCKY_EGG_SPAWN_END 85.0F
#define ITGLUCKY_HIT_ROTATE_Z 0.10471976F
#define ITGLUCKY_GRAVITY 1.2F
#define ITGLUCKY_NDAMAGE_KNOCKBACK_MIN 100.0F
#define ITGLUCKY_TVEL 100.0F
#define ITMBALL_SPAWN_WAIT 30
#define ITMBALL_GRAVITY 1.5F
#define ITMBALL_TVEL 120.0F
#define ITMBALL_MAP_REBOUND_COMMON 0.2F
#define ITMBALL_MAP_REBOUND_GROUND 0.2F
#define ITMONSTER_RISE_STOP_WAIT 22
#define ITMONSTER_RISE_VEL_Y 16.0F
#define ITPAKKUN_APPEAR_WAIT 180
#define ITPAKKUN_REBIRTH_WAIT 1200
#define ITPAKKUN_APPEAR_OFF_Y 245.0F
#define ITPAKKUN_CLAMP_OFF_Y 360.0F
#define ITPAKKUN_HURT_SIZE_MUL_Y 0.5F
#define ITPAKKUN_DETECT_SIZE_WIDTH 600.0F
#define ITPAKKUN_DETECT_SIZE_BOTTOM (-300.0F)
#define ITPAKKUN_DETECT_SIZE_TOP 700.0F
#define ITPAKKUN_NDAMAGE_KNOCKBACK_MIN 100.0F
#define ITPAKKUN_GRAVITY 1.5F
#define ITPAKKUN_TVEL 100.0F
#define ITIWARK_FLY_WAIT 30
#define ITIWARK_MODEL_ROTATE_WAIT 6
#define ITIWARK_ROCK_RUMBLE_WAIT 18
#define ITIWARK_ROCK_SPAWN_COUNT_RANDOM 9
#define ITIWARK_ROCK_SPAWN_COUNT_MIN 8
#define ITIWARK_ROCK_SPAWN_WAIT_MAX 30
#define ITIWARK_ROCK_SPAWN_WAIT_MIN 15
#define ITIWARK_ROCK_SPAWN_OFF_X_MUL 2000.0F
#define ITIWARK_ROCK_SPAWN_OFF_X_ADD (-1000.0F)
#define ITIWARK_FLY_VEL_Y 80.0F
#define ITIWARK_FLY_STOP_Y 200.0F
#define ITIWARK_IWARK_ADD_POS_Y (-660.0F)
#define ITIWARK_OTHER_ADD_POS_Y (-100.0F)
#define ITKABIGON_EFFECT_SPAWN_INT 4
#define ITKABIGON_DROP_WAIT 60
#define ITKABIGON_RUMBLE_WAIT 18
#define ITKABIGON_DROP_VEL_Y (-220.0F)
#define ITKABIGON_DROP_OFF_X_MUL 2000.0F
#define ITKABIGON_DROP_OFF_X_ADD (-1000.0F)
#define ITKABIGON_DROP_SIZE_KABIGON 4.0F
#define ITKABIGON_DROP_SIZE_OTHER 5.2F
#define ITKABIGON_JUMP_VEL_Y 80.0F
#define ITKABIGON_JUMP_GFX_MUL_OFF 200.0F
#define ITKABIGON_JUMP_GFX_SUB_OFF 100.0F
#define ITKABIGON_MAP_OFF_Y 200.0F
#define ITTOSAKINTO_LIFETIME 360
#define ITTOSAKINTO_FLAP_VEL_X 10.0F
#define ITTOSAKINTO_FLAP_VEL_Y 60.0F
#define ITTOSAKINTO_GRAVITY 6.0F
#define ITTOSAKINTO_TVEL 90.0F
#define ITMEW_LIFETIME 480
#define ITMEW_EFFECT_SPAWN_INT 3
#define ITMEW_STARTVEL_X 8.0F
#define ITMEW_STARTVEL_Y -20.0F
#define ITMEW_FLY_ADD_VEL_Y 0.8F
#define ITNYARS_LIFETIME 240
#define ITNYARS_MODEL_ROTATE_WAIT 30
#define ITNYARS_COIN_SPAWN_MAX 4
#define ITNYARS_COIN_LIFETIME 10
#define ITNYARS_COIN_SPAWN_WAIT 8
#define ITNYARS_COIN_ANGLE_STEP 13.0F
#define ITNYARS_COIN_ANGLE_DIFF 90.0F
#define ITNYARS_COIN_VEL_X 130.0F
#define ITLIZARDON_LIFETIME 480
#define ITLIZARDON_FLAME_LIFETIME 30
#define ITLIZARDON_FLAME_SPAWN_WAIT 8
#define ITLIZARDON_TURN_WAIT 26
#define ITLIZARDON_FLAME_ANGLE F_CLC_DTOR32(-15.0F)
#define ITLIZARDON_FLAME_VEL 50.0F
#define ITLIZARDON_LIZARDON_FLAME_OFF_X 180.0F
#define ITLIZARDON_LIZARDON_FLAME_OFF_Y 120.0F
#define ITLIZARDON_OTHER_FLAME_OFF_X 100.0F
#define ITLIZARDON_DUST_OFF_X (-400.0F)
#define ITLIZARDON_GRAVITY 1.0F
#define ITLIZARDON_TVEL 40.0F
#define ITLIZARDON_MAP_REBOUND_COMMON 0.2F
#define ITLIZARDON_MAP_REBOUND_GROUND 1.0F
#define ITSPEAR_SPAWN_COUNT 16
#define ITSPEAR_SPAWN_WAIT_CONST 12
#define ITSPEAR_SPAWN_WAIT_RANDOM 9
#define ITSPEAR_SPAWN_OFF_Y_MUL 1800.0F
#define ITSPEAR_SPAWN_OFF_Y_ADD (-800.0F)
#define ITSPEAR_SWARM_FLY_VEL_X 130.0F
#define ITSPEAR_SWARM_CALL_VEL_X 6.0F
#define ITSPEAR_SWARM_CALL_VEL_Y 60.0F
#define ITSPEAR_SWARM_CALL_OFF_X 500.0F
#define ITSPEAR_SWARM_CALL_WAIT 51.0F
#define ITSPEAR_GRAVITY 1.0F
#define ITSPEAR_TVEL 90.0F
#define ITKAMEX_LIFETIME 360
#define ITKAMEX_HYDRO_LIFETIME 20
#define ITKAMEX_HYDRO_SPAWN_WAIT_CONST 30
#define ITKAMEX_HYDRO_SPAWN_WAIT_RANDOM 1
#define ITKAMEX_KAMEX_HYDRO_SPAWN_OFF_X 360.0F
#define ITKAMEX_KAMEX_HYDRO_SPAWN_OFF_Y 100.0F
#define ITKAMEX_OTHER_HYDRO_SPAWN_OFF_X 100.0F
#define ITKAMEX_DUST_SPAWN_OFF_X (-150.0F)
#define ITKAMEX_COLL_SIZE 341.0F
#define ITKAMEX_PUSH_VEL_X 2.3F
#define ITKAMEX_CONSTVEL_X 38.0F
#define ITKAMEX_GRAVITY 1.0F
#define ITKAMEX_TVEL 40.0F
#define ITMLUCKY_LIFETIME 90
#define ITMLUCKY_EGG_SPAWN_WAIT_ADD 4
#define ITMLUCKY_EGG_SPAWN_COUNT 3
#define ITMLUCKY_EGG_SPAWN_WAIT_CONST 30
#define ITMLUCKY_EGG_SPAWN_BASE_VEL 8.0F
#define ITMLUCKY_EGG_SPAWN_ADD_VEL_X 7.0F
#define ITMLUCKY_EGG_SPAWN_ADD_VEL_Y 40.0F
#define ITMLUCKY_GRAVITY 1.0F
#define ITMLUCKY_TVEL 40.0F
#define ITSTARMIE_LIFETIME 240
#define ITSTARMIE_SWIFT_LIFETIME 30
#define ITSTARMIE_SWIFT_SPAWN_WAIT_CONST 12
#define ITSTARMIE_SWIFT_SPAWN_WAIT_RANDOM 1
#define ITSTARMIE_SWIFTVEL_X 150.0F
#define ITSTARMIE_STARMIE_SWIFT_SPAWN_OFF_X 200.0F
#define ITSTARMIE_STARMIE_SWIFT_SPAWN_OFF_Y 100.0F
#define ITSTARMIE_OTHER_SWIFT_SPAWN_OFF_X 100.0F
#define ITSTARMIE_TARGET_POS_OFF_X 400.0F
#define ITSTARMIE_TARGET_POS_OFF_Y 250.0F
#define ITSTARMIE_FOLLOW_VEL_X 20.0F
#define ITSTARMIE_ADD_VEL_X 10.0F
#define ITSTARMIE_PUSH_VEL_X 70.0F
#define ITSAWAMURA_LIFETIME 600
#define ITSAWAMURA_KICK_WAIT 40
#define ITSAWAMURA_TARGET_POS_OFF_Y 500.0F
#define ITSAWAMURA_DESPAWN_OFF_X 500.0F
#define ITSAWAMURA_KICK_SIZE 300.0F
#define ITSAWAMURA_KICK_VEL_X 400.0F
#define ITSAWAMURA_GRAVITY 2.4F
#define ITSAWAMURA_TVEL 100.0F
#define ITDOGAS_DESPAWN_WAIT 90
#define ITDOGAS_SMOG_SPAWN_WAIT 8
#define ITDOGAS_SMOG_SPAWN_COUNT 32
#define ITDOGAS_SMOG_LIFETIME 30
#define ITDOGAS_SMOG_VEL 18.0F
#define ITDOGAS_SMOG_MUL_OFF_X 400.0F
#define ITDOGAS_SMOG_SUB_OFF_X 200.0F
#define ITDOGAS_SMOG_MUL_OFF_Y 800.0F
#define ITDGOAS_SMOG_SUB_OFF_Y 400.0F
#define ITMARUMINE_EXPLODE_LIFETIME 6
#define ITMARUMINE_EXPLODE_EFFECT_SCALE 1.4F
#define ITPORYGON_SHAKE_STOP_WAIT 32
#define ITHITOKAGE_FLAME_LIFETIME 20
#define ITHITOKAGE_FLAME_SPAWN_WAIT 8
#define ITHITOKAGE_FLAME_SPAWN_ANGLE F_CLC_DTOR32(-12.0F)
#define ITHITOKAGE_FLAME_VEL_BASE 45.0F
#define ITHITOKAGE_FLAME_SPAWN_BEGIN 40.0F
#define ITHITOKAGE_FLAME_SPAWN_END 120.0F
#define ITHITOKAGE_FLAME_SPAWN_OFF_X (-250.0F)
#define ITHITOKAGE_HIT_ROTATE_Z F_CLC_DTOR32(6.0F)
#define ITHITOKAGE_NDAMAGE_KNOCKBACK_MIN 100.0F
#define ITHITOKAGE_GRAVITY 1.2F
#define ITHITOKAGE_TVEL 100.0F
#define ITFUSHIGIBANA_RETURN_WAIT 128
#define ITFUSHIGIBANA_RAZOR_LIFETIME 24
#define ITFUSHIGIBANA_RAZOR_SPAWN_WAIT 16
#define ITFUSHIGIBANA_RAZOR_VEL_X (-100.0F)
#define ITFUSHIGIBANA_RAZOR_ADD_VEL_X 5.0F
#define ITFUSHIGIBANA_RAZOR_SPAWN_BEGIN 40.0F
#define ITFUSHIGIBANA_RAZOR_SPAWN_END 120.0F
#define ITFUSHIGIBANA_RAZOR_SPAWN_OFF_X (-540.0F)
#define ITTARUBOMB_EFFECT_COUNT 7
#define ITTARUBOMB_GFX_LIFETIME 90
#define ITTARUBOMB_HEALTH_MAX 10
#define ITTARUBOMB_EXPLODE_LIFETIME 6
#define ITTARUBOMB_EXPLODE_EFFECT_SCALE 1.4F
#define ITTARUBOMB_MUL_VEL_X 1.4F
#define ITTARUBOMB_VEL_MIN 0.1F
#define ITTARUBOMB_ROLL_ROTATE_MUL 0.0045F
#define ITTARUBOMB_GRAVITY 4.0F
#define ITTARUBOMB_TVEL 90.0F
#define ITTARUBOMB_MAP_REBOUND_COMMON 0.5F
#define ITPKFIRE_LIFETIME 100
#define ITPKFIRE_HURT_DAMAGE_MUL 3
#define ITPKFIRE_GRAVITY 0.45F
#define ITPKFIRE_TVEL 55.0F
#define ITPKFIRE_GRAVITY 0.4F
#define ITPKFIRE_TVEL 50.0F
#define ITPKFIRE_MAP_REBOUND_COMMON 0.2F
#define ITPKFIRE_MAP_REBOUND_GROUND 0.5F
#define ITLINKBOMB_HEALTH 7
#define ITLINKBOMB_LIFETIME 300
#define ITLINKBOMB_EXPLODE_LIFETIME 6
#define ITLINKBOMB_BLOAT_COLANIM_ID 0x4F
#define ITLINKBOMB_BLOAT_COLANIM_LENGTH 96
#define ITLINKBOMB_SCALE_INDEX_MAX 10
#define ITLINKBOMB_SCALE_INDEX_REWIND (ITLINKBOMB_SCALE_INDEX_MAX / 2)
#define ITLINKBOMB_SCALE_INT 4
#define ITLINKBOMB_DAMAGE_RECOIL_VEL_X 20.0F
#define ITLINKBOMB_DAMAGE_RECOIL_VEL_Y 18.0F
#define ITLINKBOMB_EXPLODE_THRESHOLD_VEL_X 36.0F
#define ITLINKBOMB_EXPLODE_THRESHOLD_VEL_Y 25.0F
#define ITLINKBOMB_BLOAT_BEGIN 96.0F
#define ITLINKBOMB_HIT_RECOIL_VEL_X 8.0F
#define ITLINKBOMB_HIT_RECOIL_VEL_Y 20.0F
#define ITLINKBOMB_EXPLODE_EFFECT_SCALE 1.3F
#define ITLINKBOMB_GRAVITY 1.2F
#define ITLINKBOMB_TVEL 100.0F
#define ITLINKBOMB_MAP_REBOUND_COMMON 0.4F
#define ITLINKBOMB_MAP_REBOUND_GROUND 0.3F

/* BattleShip it/itvars.h:196-216. GBumper tuning. The landed slice consumes
 * HIT_ANIM_LENGTH + HIT_SCALE (proc update/hit) and CASTLE_KNOCKBACK +
 * CASTLE_ANGLE (maker Castle override); the rest (lifetime/despawn/gravity/
 * rebound) travels with the constants so later slices do not re-derive them. */
#define ITBUMPER_CASTLE_KNOCKBACK 300
#define ITBUMPER_CASTLE_ANGLE 361
#define ITBUMPER_LIFETIME 360
#define ITBUMPER_DESPAWN_TIMER 60
#define ITBUMPER_STOPVEL_WAIT 4
#define ITBUMPER_DAMAGE_ALL_WAIT 16
#define ITBUMPER_HIT_SCALE 10
#define ITBUMPER_HIT_ANIM_LENGTH 3
#define ITBUMPER_COLL_SIZE 120.0F
#define ITBUMPER_REBOUND_VEL_X (-100.0F)
#define ITBUMPER_REBOUND_AIR_X (-400.0F)
#define ITBUMPER_REBOUND_AIR_Y 200.0F
#define ITBUMPER_GRAVITY_NORMAL 1.4F
#define ITBUMPER_GRAVITY_HIT 4.0F
#define ITBUMPER_TVEL 80.0F
#define ITBUMPER_MAP_REBOUND_COMMON 0.8F
#define ITBUMPER_MAP_REBOUND_GROUND 0.8F

/* BattleShip itdef.h. P2-3 Link graduates the shared item owner with LinkBomb
 * as its first live client; keep the source capacities/physics constants here
 * so later P2-5 items join the same owner instead of growing fighter-local
 * article forks. */
#define ITEM_ALLOC_MAX 16
#define ITEM_FLAG_COLLPROJECT (1u << 31)
#define ITEM_FLAG_PARENT_FIGHTER 0
#define ITEM_FLAG_PARENT_GROUND 1
#define ITEM_FLAG_PARENT_WEAPON 2
#define ITEM_FLAG_PARENT_ITEM 3
#define ITEM_FLAG_PARENT_DEFAULT 4
#define ITEM_MASK_PARENT 0xFu
#define ITEM_TEAM_DEFAULT 4
#define ITEM_PORT_DEFAULT GMCOMMON_PLAYERS_MAX
#define ITEM_HANDICAP_DEFAULT 9
#define ITEM_THROW_DEFAULT 1.0F
#define ITEM_PICKUP_WAIT_DEFAULT 1400
#define ITEM_REFLECT_MAX_DEFAULT 100
#define ITEM_REFLECT_MUL_DEFAULT 1.8F
#define ITEM_REFLECT_ADD_DEFAULT 0.99F
#define ITEM_DESPAWN_FLASH_BEGIN_DEFAULT 180U
#define ITEM_ARROW_FLASH_INT_DEFAULT 45
#define ITEM_HOP_ANGLE_DEFAULT F_CST_DTOR32(135.0F)
#define ITEM_SPIN_SPEED_COMMON F_CST_DTOR32(18.0F)
#define ITEM_SPIN_SPEED_APPEAR_SLOW F_CLC_DTOR32(10.0F)
#define ITEM_SPIN_SPEED_APPEAR_FAST F_CST_DTOR32(16.0F)
#define ITEM_SPIN_SPEED_SMASH_THROW F_CST_DTOR32(-21.0F)
#define ITEM_SPIN_SPEED_NORMAL_THROW F_CLC_DTOR32(-10.0F)
#define ITEM_THROW_NUM_MAX 4
#define ITEM_THROW_DESPAWN_RANDOM 4
#define ITEM_LANDING_DESPAWN_CHECK 1
#define ITEM_LANDING_NUM_MAX 2
#ifndef GMCOMMON_PERCENT_DAMAGE_MAX
#define GMCOMMON_PERCENT_DAMAGE_MAX 999
#endif

#ifndef SSB64_NDS_IT_ATTACK_COLL_DECLARED
#define SSB64_NDS_IT_ATTACK_COLL_DECLARED
typedef struct ITAttackPos {
    Vec3f pos_curr;
    Vec3f pos_prev;
    sb32 unk_ithitpos_0x18;
    Mtx44f mtx;
    f32 unk_ithitpos_0x5C;
} ITAttackPos;

typedef struct ITAttackColl {
    s32 attack_state;
    s32 damage;
    f32 throw_mul;
    f32 stale;
    s32 element;
    Vec3f offsets[ITEM_ATKCOLL_NUM_MAX];
    f32 size;
    s32 angle;
    u32 knockback_scale;
    u32 knockback_weight;
    u32 knockback_base;
    s32 shield_damage;
    s32 priority;
    u8 interact_mask;
    u16 fgm_id;
    ub32 can_setoff : 1;
    ub32 can_rehit_item : 1;
    ub32 can_rehit_fighter : 1;
    ub32 can_rehit_shield : 1;
    ub32 can_hop : 1;
    ub32 can_reflect : 1;
    ub32 can_shield : 1;
    u32 motion_attack_id : 6;
    u16 motion_count;
    GMStatFlags stat_flags;
    u16 stat_count;
    s32 attack_count;
    ITAttackPos attack_pos[ITEM_ATKCOLL_NUM_MAX];
    GMAttackRecord attack_records[GMATTACKREC_NUM_MAX];
} ITAttackColl;

typedef struct ITDamageColl {
    u8 interact_mask;
    s32 hitstatus;
    Vec3f offset;
    Vec3f size;
} ITDamageColl;
#endif

typedef struct ITAttackEvent {
    u8 timer;
    s32 angle;
    u32 damage;
    u16 size;
} ITAttackEvent;

/* DS-native semantic view of BattleShip's audited 0x48-byte ROM
 * ITAttributes record. The loader still owns relocation/endian conversion;
 * the item manager decodes the N64 packed words once into this ordinary ARM
 * structure so gameplay never depends on IDO bitfield packing. */
typedef struct ITAttributes {
    void *data;
    MObjSub ***p_mobjsubs;
    AObjEvent32 **anim_joints;
    AObjEvent32 ***p_matanim_joints;
    u8 is_display_xlu;
    u8 is_item_dobjs;
    u8 is_display_colanim;
    u8 is_give_hitlag;
    u8 weight;
    s16 attack_offset0_x;
    s16 attack_offset0_y;
    s16 attack_offset0_z;
    s16 attack_offset1_x;
    s16 attack_offset1_y;
    s16 attack_offset1_z;
    Vec3h damage_coll_offset;
    Vec3h damage_coll_size;
    s16 map_coll_top;
    s16 map_coll_center;
    s16 map_coll_bottom;
    s16 map_coll_width;
    u16 size;
    s32 angle;
    u32 knockback_scale;
    u32 damage;
    u32 element;
    u32 knockback_weight;
    s32 shield_damage;
    u32 attack_count;
    u8 can_setoff;
    u16 hit_sfx;
    u32 priority;
    u8 can_rehit_item;
    u8 can_rehit_fighter;
    u8 can_hop;
    u8 can_reflect;
    u8 can_shield;
    u32 knockback_base;
    u32 type;
    u32 hitstatus;
    u16 drop_sfx;
    u16 throw_sfx;
    u16 smash_sfx;
    u16 vel_scale;
    u16 spin_speed;
} ITAttributes;

typedef struct ITDesc {
    s32 kind;
    void **p_file;
    const void *o_attributes;
    DObjTransformTypes transform_types;
    s32 attack_state;
    sb32 (*proc_update)(GObj *);
    sb32 (*proc_map)(GObj *);
    sb32 (*proc_hit)(GObj *);
    sb32 (*proc_shield)(GObj *);
    sb32 (*proc_hop)(GObj *);
    sb32 (*proc_setoff)(GObj *);
    sb32 (*proc_reflector)(GObj *);
    sb32 (*proc_damage)(GObj *);
} ITDesc;

typedef struct ITStatusDesc {
    sb32 (*proc_update)(GObj *);
    sb32 (*proc_map)(GObj *);
    sb32 (*proc_hit)(GObj *);
    sb32 (*proc_shield)(GObj *);
    sb32 (*proc_hop)(GObj *);
    sb32 (*proc_setoff)(GObj *);
    sb32 (*proc_reflector)(GObj *);
    sb32 (*proc_damage)(GObj *);
} ITStatusDesc;

typedef struct ITFighterItemVarsPKFire {
    struct LBTransform *xf;
} ITFighterItemVarsPKFire;

typedef struct ITFighterItemVarsLinkBomb {
    u16 unk_0x0;
    u16 drop_update_wait;
    u16 scale_id;
    u16 scale_int;
} ITFighterItemVarsLinkBomb;

/* BattleShip it/itvars.h:546-554. GBumper per-item state: the hit-flash timer
 * ProcUpdate counts down, plus the owner-damage delay word the NBumper-class
 * behavior owns (reserved here so the union slot matches source layout). */
typedef struct ITCommonItemVarsBumper {
    u16 hit_anim_length;
    u16 unk_0x2;
    u16 damage_all_delay;
} ITCommonItemVarsBumper;

typedef struct ITStruct {
    struct ITStruct *next;
    GObj *item_gobj;
    GObj *owner_gobj;
    s32 kind;
    s32 type;
    u8 team;
    u8 player;
    u8 handicap;
    s32 player_num;
    s32 percent_damage;
    u32 hitlag_tics;
    s32 lr;
    struct {
        f32 vel_ground;
        Vec3f vel_air;
    } physics;
    MPCollData coll_data;
    sb32 ga;
    ITAttackColl attack_coll;
    ITDamageColl damage_coll;
    s32 hit_normal_damage;
    s32 hit_lr;
    s32 hit_refresh_damage;
    s32 hit_attack_damage;
    s32 hit_shield_damage;
    f32 shield_collide_angle;
    Vec3f shield_collide_dir;
    GObj *reflect_gobj;
    GMStatFlags reflect_stat_flags;
    u16 reflect_stat_count;
    s32 damage_highest;
    f32 damage_knockback;
    s32 damage_queue;
    s32 damage_angle;
    s32 damage_element;
    s32 damage_lr;
    GObj *damage_gobj;
    u8 damage_team;
    u8 damage_port;
    s32 damage_player_num;
    u8 damage_handicap;
    s32 damage_display_mode;
    s32 damage_lag;
    s32 lifetime;
    f32 vel_scale;
    u16 drop_sfx;
    u16 throw_sfx;
    u16 smash_sfx;
    ub32 is_allow_pickup : 1;
    ub32 is_hold : 1;
    u32 times_landed : 2;
    u32 times_thrown : 3;
    ub32 weight : 1;
    ub32 is_damage_all : 1;
    ub32 is_attach_surface : 1;
    ub32 is_thrown : 1;
    u16 attach_line_id;
    u32 pickup_wait : 12;
    ub32 is_allow_knockback : 1;
    ub32 is_unused_item_bool : 1;
    ub32 is_static_damage : 1;
    ITAttributes *attr;
    GMColAnim colanim;
    ub32 is_hitlag_victim : 1;
    u16 multi;
    u32 event_id : 4;
    f32 spin_step;
    GObj *arrow_gobj;
    u8 arrow_timer;
    /* EVERY item kind's state variables, transcribed from BattleShip
     * it/itvars.h in the order it/ittypes.h:290 declares them.
     *
     * Adding one member per landed batch meant a build failure per batch
     * saying only "union has no member named <kind>", and the union is the
     * same size either way -- it is a union. The raw[] escape below is kept
     * so a kind whose vars are not yet transcribed still has somewhere to
     * live rather than silently sharing another kind's bytes. */
    union {
        /* decomp it/itvars.h, ITCommonItemVarsTaru */
        struct {
            f32 roll_rotate_step;
        } taru;
        /* decomp it/itvars.h, ITCommonItemVarsBombHei */
        struct {
            u16 smoke_delay;
        } bombhei;
        /* decomp it/itvars.h, ITCommonItemVarsBumper */
        struct {
            u16 hit_anim_length;
            u16 unk_0x2;
            u16 damage_all_delay;
        } bumper;
        /* decomp it/itvars.h, ITCommonItemVarsShell */
        struct {
            u8 damage_all_delay;
            u8 dust_effect_int;
            u8 health;
            ub8 is_damage;
            ub8 is_setup_vars;
            u8 interact;
            f32 vel_x;
        } shell;
        /* decomp it/itvars.h, ITCommonItemVarsMBall */
        struct {
            ub16 is_rebound;
            GObj* owner_gobj;
            GObj* effect_gobj;
        } mball;
        /* decomp it/itvars.h, ITFighterItemVarsPKFire */
        struct {
            LBTransform *xf;
        } pkfire;
        /* decomp it/itvars.h, ITFighterItemVarsLinkBomb */
        struct {
            u16 unk_0x0;
            u16 drop_update_wait;
            u16 scale_id;
            u16 scale_int;
        } linkbomb;
        /* decomp it/itvars.h, ITGroundItemVarsPakkun */
        struct {
            Vec3f pos;
            ub8 is_wait_fighter;
        } pakkun;
        /* decomp it/itvars.h, ITGroundItemVarsTaruBomb */
        struct {
            f32 roll_rotate_step;
        } tarubomb;
        /* decomp it/itvars.h, ITGroundItemVarsGLucky */
        struct {
            Vec3f pos;
            u16 egg_spawn_count;
        } glucky;
        /* decomp it/itvars.h, ITGroundItemVarsMarumine */
        struct {
            Vec3f offset;
        } marumine;
        /* decomp it/itvars.h, ITGroundItemVarsHitokage */
        struct {
            Vec3f offset;
            u16 flags;
            u16 flame_spawn_wait;
        } hitokage;
        /* decomp it/itvars.h, ITGroundItemVarsFushigibana */
        struct {
            Vec3f offset;
            u16 flags;
            u16 razor_spawn_wait;
        } fushigibana;
        /* decomp it/itvars.h, ITGroundItemVarsPorygon */
        struct {
            Vec3f offset;
        } porygon;
        /* decomp it/itvars.h, ITMonsterItemVarsIwark */
        struct {
            u16 rock_spawn_remain;
            s32 rock_spawn_wait;
            u16 rock_spawn_max;
            u16 rumble_frame;
            u16 rumble_wait;
            u16 rock_spawn_count;
        } iwark;
        /* decomp it/itvars.h, ITMonsterItemVarsKabigon */
        struct {
            s32 dust_effect_int;
            s32 rumble_wait;
        } kabigon;
        /* decomp it/itvars.h, ITMonsterItemVarsTosakinto */
        struct {
            Vec3f pos;
        } tosakinto;
        /* decomp it/itvars.h, ITMonsterItemVarsNyars */
        struct {
            u16 coin_spawn_wait;
            u16 coin_rotate_step;
            u16 model_rotate_wait;
        } nyars;
        /* decomp it/itvars.h, ITMonsterItemVarsLizardon */
        struct {
            Vec3f pos;
            u16 turn_wait;
            u16 flame_spawn_wait;
        } lizardon;
        /* decomp it/itvars.h, ITMonsterItemVarsSpear */
        struct {
            u16 spear_spawn_count;
            s32 spear_spawn_wait;
            f32 spear_spawn_pos_y;
        } spear;
        /* decomp it/itvars.h, ITMonsterItemVarsKamex */
        struct {
            s32 hydro_spawn_wait;
            f32 hydro_push_vel_x;
            sb32 is_apply_push;
        } kamex;
        /* decomp it/itvars.h, ITMonsterItemVarsMLucky */
        struct {
            u16 egg_spawn_wait;
            u16 lifetime;
        } mlucky;
        /* decomp it/itvars.h, ITMonsterItemVarsStarmie */
        struct {
            s32 unk_0x0;
            s32 swift_spawn_wait;
            Vec3f target_pos;
            Vec3f victim_pos;
            f32 add_vel_x;
        } starmie;
        /* decomp it/itvars.h, ITMonsterItemVarsDogas */
        struct {
            Vec3f pos;
            s32 smog_spawn_wait;
        } dogas;
        /* decomp it/itvars.h, ITMonsterItemVarsMew */
        struct {
            s32 esper_gfx_int;
        } mew;
        u8 raw[16];
    } item_vars;
    s32 display_mode;
    sb32 (*proc_update)(GObj *);
    sb32 (*proc_map)(GObj *);
    sb32 (*proc_hit)(GObj *);
    sb32 (*proc_shield)(GObj *);
    sb32 (*proc_hop)(GObj *);
    sb32 (*proc_setoff)(GObj *);
    sb32 (*proc_reflector)(GObj *);
    sb32 (*proc_damage)(GObj *);
    sb32 (*proc_dead)(GObj *);
} ITStruct;

typedef enum ITKind {
    nITKindCommonStart,
    nITKindContainerStart = nITKindCommonStart,
    nITKindBox = nITKindContainerStart,
    nITKindTaru,
    nITKindCapsule,
    nITKindEgg,
    nITKindContainerEnd = nITKindEgg,
    nITKindUtilityStart,
    nITKindTomato = nITKindUtilityStart,
    nITKindHeart,
    nITKindStar,
    nITKindSword,
    nITKindBat,
    nITKindHarisen,
    nITKindStarRod,
    nITKindLGun,
    nITKindFFlower,
    nITKindHammer,
    nITKindMSBomb,
    nITKindBombHei,
    nITKindNBumper,
    nITKindGShell,
    nITKindRShell,
    nITKindMBall,
    nITKindUtilityEnd = nITKindMBall,
    nITKindCommonEnd = nITKindUtilityEnd,
    nITKindFighterStart,
    nITKindNessPKFire = nITKindFighterStart,
    nITKindLinkBomb,
    nITKindFighterEnd = nITKindLinkBomb,
    nITKindGroundoundStart,
    nITKindPowerBlock = nITKindGroundoundStart,
    nITKindGBumper,
    nITKindPakkun,
    nITKindTarget,
    nITKindTaruBomb,
    nITKindGroundMonsterStart,
    /* decomp it/itdef.h:139-145. Saffron City spawns all five. */
    nITKindGLucky = nITKindGroundMonsterStart,
    nITKindMarumine,
    nITKindHitokage,
    nITKindFushigibana,
    nITKindPorygon,
    nITKindGroundMonsterEnd = nITKindPorygon,
    /* decomp it/itdef.h:150-166, the Poke Ball Pokemon, in the source's own
     * order. The containers name Chansey directly as a payload, so the whole
     * run arrives together rather than one kind at a time. */
    nITKindMBallMonsterStart,
    nITKindMBallCommonStart = nITKindMBallMonsterStart,
    nITKindIwark = nITKindMBallMonsterStart,
    nITKindKabigon,
    nITKindTosakinto,
    nITKindNyars,
    nITKindLizardon,
    nITKindSpear,
    nITKindKamex,
    nITKindMLucky,
    nITKindStarmie,
    nITKindSawamura,
    nITKindDogas,
    nITKindPippi,
    nITKindMBallCommonEnd = nITKindPippi,
    nITKindMew,
    nITKindMBallMonsterEnd = nITKindMew,
    nITKindEnumCount
} ITKind;

enum {
    nITTypeDamage = 0,
    nITTypeSwing = 1,
    nITTypeShoot = 2,
    nITTypeThrow = 3,
    nITTypeTouch = 4,
    nITTypeConsume = 5,
    nITTypeFighter = 6
};

enum {
    nITWeightHeavy = 0,
    nITWeightLight = 1
};

#define itGetStruct(item_gobj) ((ITStruct *)(item_gobj)->user_data.p)

/* decomp it/item.h:40-47. Items reach their own data by subtracting the
 * kind's data-start offset from attr->data and adding the field's; the two
 * offsets are the reloc constants each item TU owns. */
#define itGetPData(ip, off1, off2) ((void *)(((uintptr_t)(ip)->attr->data - (intptr_t)(off1)) + (intptr_t)(off2)))


ITAttackEvent *ndsItGetAttackEvent(const ITDesc *item_desc,
                                   const void *offset_token);
#define itGetAttackEvent(it_desc, off) ndsItGetAttackEvent(&(it_desc), (off))

void itManagerInitItems(void);
/* decomp it/itmanager.h:41. The port declared this void while the source
 * returns the actor GObj, which forced the item core to rename the prototype
 * at include time to transcribe the real one. */
GObj *itManagerMakeAppearActor(void);

/* decomp it/item.h:33 and itmanager.c:15: zero spawns a random Pokemon,
 * non-zero forces one. Saffron City reads and writes it (gryamabuki.c:89). */
extern s32 dITManagerForceMonsterKind;

/* decomp it/itground/itpakkun.c:118-126. The whole function, and it is
 * NULL-guarded at source, which is what makes Mushroom Kingdom safe before
 * the Piranha item kind lands. */
void itPakkunCommonSetWaitFighter(GObj *item_gobj);
GObj *itManagerMakeItem(GObj *parent_gobj, ITDesc *item_desc, Vec3f *pos,
                        Vec3f *vel, u32 flags);
/* P2-5i1 minimal maker table (decomp it/itmanager.c:41-97, :717-720).
 * Only the GBumper (kind 23) slot is registered in this slice; the two
 * fighter-article slots stay NULL per source :68-69 and LinkBomb keeps its
 * direct fighter-owned maker, so LinkBomb behaves identically. All other
 * slots are NULL until their item slice lands. */
GObj *itManagerMakeItemKind(GObj *parent_gobj, s32 kind, Vec3f *pos,
                            Vec3f *vel, u32 flags);
/* P2-5i1 spawn-setup companion (decomp it/itmanager.c:464-477). Trivial by
 * design: delegate to the kind maker; the source's spawn swirl + appear spin
 * (itMainSetAppearSpin / efManagerItemSpawnSwirlMakeEffect) are NOT ported
 * here -- neither helper exists in the port yet (UNVERIFIED: swirl/tempo on
 * stage-spawned bumpers), so SetupCommon is a pass-through that the Castle
 * stage can already call. Lands the call shape, defers the presentation. */
GObj *itManagerMakeItemSetupCommon(GObj *parent_gobj, s32 kind, Vec3f *pos,
                                   Vec3f *vel, u32 flags);
/* P2-5i1 GBumper (decomp it/itground/itgbumper.h:8-10). */
sb32 itGBumperCommonProcUpdate(GObj *item_gobj);
sb32 itGBumperCommonProcHit(GObj *item_gobj);
GObj *itGBumperMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags);
/* P2-5i1 ordinary counters (defined in the item core TU). */
extern volatile u32 gNdsGBumperMakeCount;
extern volatile u32 gNdsGBumperAttrValidCount;
extern volatile u32 gNdsItSetupDObjCount;
extern volatile u32 gNdsItSetupDObjOrphanCount;
extern volatile u32 gNdsItSetupDObjOrphanIndex;
extern volatile u32 gNdsItSetupDObjOrphanID;
void itManagerSetPrevStructAlloc(ITStruct *ip);
void itMainSetSpinVelLR(GObj *item_gobj);
void itMainApplyGravityClampTVel(ITStruct *ip, f32 gravity,
                                 f32 terminal_velocity);
void itMainResetPlayerVars(GObj *item_gobj);
void itMainClearAttackRecord(ITStruct *ip);
void itMainRefreshAttackColl(GObj *item_gobj);
void itMainClearOwnerStats(GObj *item_gobj);
void itMainSetFighterHold(GObj *item_gobj, GObj *fighter_gobj);
void itMainSetFighterRelease(GObj *item_gobj, Vec3f *vel, f32 throw_mul,
                             u16 stat_flags, u16 stat_count);
void itMainSetFighterDrop(GObj *item_gobj, Vec3f *vel, f32 throw_mul);
void itMainSetFighterThrow(GObj *item_gobj, Vec3f *vel, f32 throw_mul,
                           sb32 is_smash_throw);
void itMainSetStatus(GObj *item_gobj, ITStatusDesc *status_desc, s32 status_id);
sb32 itMainCheckSetColAnimID(GObj *item_gobj, s32 colanim_id, s32 duration);
void itMainClearColAnim(GObj *item_gobj);
void itMainVelSetRebound(GObj *item_gobj);
sb32 itMainCommonProcHop(GObj *item_gobj);
sb32 itMainCommonProcReflector(GObj *item_gobj);
s32 itMainGetDamageOutput(ITStruct *ip);
void itMainDestroyItem(GObj *item_gobj);
void itProcessUpdateAttackPositions(GObj *item_gobj);
void itProcessProcItemMain(GObj *item_gobj);
void itProcessProcSearchHitAll(GObj *item_gobj);
void itProcessProcHitCollisions(GObj *item_gobj);
void itVisualsUpdateSpin(GObj *item_gobj);
void itVisualsUpdateColAnim(GObj *item_gobj);
void itMapSetGround(ITStruct *ip);
/* decomp it/itmain.c:482 and it/itmap.c:150, :156. The first is owned by
 * battleship_item_map_core.c; the other two come from the it/itmap.c import
 * inside battleship_item_link_core.c. Every common item calls all three. */
/* decomp it/ittypes.h:53-60. Layout-identical DS view (the port never
 * declares ITRandomWeights); u8 unused[8] keeps the offsets the source's.
 * It lives here rather than in the item core because the containers roll
 * their payload through it. */
typedef struct NdsITRandomWeights
{
    u8 unused[8];
    u8 valids_num;
    u8 *kinds;
    u16 weights_sum;
    u16 *blocks;
} NdsITRandomWeights;

/* decomp it/itmain.c:569. The container payload roll; gITManagerRandomWeights
 * is the manager's own table, built by itManagerSetupContainerDrops. */
s32 itMainGetWeightedItemKind(NdsITRandomWeights *weights);
extern NdsITRandomWeights gITManagerRandomWeights;
/* decomp it/itmap.h:10, defined by the itmap.c import in the item core. */
sb32 itMapTestLRWallCheckFloor(GObj *item_gobj);
/* The rest of decomp it/itmap.h, all defined by the it/itmap.c import in
 * battleship_item_link_core.c. Declared wholesale rather than one per item
 * batch, which is how the last four batches each found a new one. */
sb32 itMapProcLRWallCheckFloor(MPCollData* coll_data, GObj* item_gobj, u32 flags);
sb32 itMapProcAllCheckCollEnd(MPCollData* coll_data, GObj* item_gobj, u32 flags);
sb32 itMapTestAllCheckCollEnd(GObj* item_gobj);
sb32 itMapProcAllCheckCollisionFlag(MPCollData* coll_data, GObj* item_gobj, u32 coll_flags);
void itMapSetGroundRebound(Vec3f* vel, Vec3f* floor_angle, f32 ground_rebound);
sb32 itMapCheckLanding(GObj* item_gobj, f32 common_rebound, f32 ground_rebound, void (*proc_map)(GObj*));
sb32 itMapCheckDestroyLanding(GObj* item_gobj, f32 common_rebound);
sb32 itMapCheckMapProcAll(GObj* item_gobj, void (*proc_map)(GObj*));
sb32 func_ovl3_80173E9C(GObj* item_gobj, void (*proc_map)(GObj*));
sb32 itMapCheckMapReboundProcNoFloor(GObj* item_gobj, f32 common_rebound, void (*proc)(GObj*));
/* decomp it/itmain.c:575 and :615, owned by battleship_item_map_core.c.
 * The containers roll their payload through the first and drive their
 * attack-event script through the second. */
sb32 itMainMakeContainerItem(GObj *parent_gobj);
void itMainUpdateAttackEvent(GObj *item_gobj, ITAttackEvent *ev);
/* decomp it/ittypes.h:15-22. The Poke Ball's last-two-spawns memory. */
typedef struct NdsITMonsterData {
    u8 monster_curr;
    u8 monster_prev;
    u8 monster_id[44];
    u8 monsters_num;
} NdsITMonsterData;
extern NdsITMonsterData gITManagerMonsterData;
void itManagerInitMonsterVars(void);
GObj *itMainMakeMonster(GObj *item_gobj);
void itMainCopyDamageStats(GObj *item_gobj);
void itMainSetGroundAllowPickup(GObj *item_gobj);
sb32 itMapTestAllCollisionFlag(GObj *item_gobj, u32 flag);
sb32 itMapCheckCollideAllRebound(GObj *item_gobj, u32 check_flags, f32 mod_vel,
                                 Vec3f *pos);
void itMapSetAir(ITStruct *ip);
sb32 itMapCheckLRWallProcNoFloor(GObj *item_gobj, void (*proc_map)(GObj *));
sb32 itMapCheckDestroyDropped(GObj *item_gobj, f32 common_rebound,
                              f32 ground_rebound, void (*proc_status)(GObj *));
sb32 itMapCheckMapReboundProcAll(GObj *item_gobj, f32 common_rebound,
                                 f32 ground_rebound, void (*proc_map)(GObj *));
void itProcessSetHitInteractStats(ITAttackColl *attack_coll,
                                  GObj *victim_gobj, s32 attack_type,
                                  u32 victim_group_id);
sb32 gmCollisionCheckItemInFighterRange(ITAttackColl *attack_coll,
                                        s32 attack_id, GObj *fighter_gobj);
sb32 gmCollisionCheckItemAttackFighterAttackCollide(
    ITAttackColl *it_attack_coll, s32 attack_id, FTAttackColl *ft_attack_coll);
sb32 gmCollisionCheckItemAttackFighterDamageCollide(
    ITAttackColl *attack_coll, s32 attack_id, FTDamageColl *damage_coll);
sb32 gmCollisionCheckItemAttackShieldCollide(ITAttackColl *attack_coll,
                                             s32 attack_id,
                                             GObj *fighter_gobj, DObj *dobj,
                                             f32 *p_angle, Vec3f *vec);
sb32 gmCollisionCheckItemAttackSpecialCollide(ITAttackColl *attack_coll,
                                              s32 attack_id, FTStruct *fp,
                                              FTSpecialColl *special_coll);
void gmCollisionGetItemAttackFighterAttackPosition(Vec3f *dst,
                                                   ITAttackColl *it_attack_coll,
                                                   s32 attack_id,
                                                   FTAttackColl *ft_attack_coll);
void gmCollisionGetItemAttackFighterDamagePosition(Vec3f *dst,
                                                   ITAttackColl *attack_coll,
                                                   s32 attack_id,
                                                   FTDamageColl *damage_coll);
void gmCollisionGetItemAttackShieldPosition(Vec3f *dst,
                                            ITAttackColl *attack_coll,
                                            s32 attack_id, GObj *gobj,
                                            DObj *dobj);
#endif
