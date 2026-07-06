#ifndef SSB64_NDS_WEAPON_H
#define SSB64_NDS_WEAPON_H

#include <ft/fighter.h>
#include <sys/develop.h>
#include <sys/objman.h>

#ifndef WEAPON_ATKCOLL_NUM_MAX
#define WEAPON_ATKCOLL_NUM_MAX 2
#endif
#define WEAPON_ALLOC_MAX 32
#define WEAPON_ALLOC_ALIGN 0x8
#define WEAPON_REHIT_TIME_DEFAULT 16
#define WEAPON_STALE_DEFAULT 1.0F
#define WEAPON_TEAM_DEFAULT 4
#define WEAPON_PORT_DEFAULT GMCOMMON_PLAYERS_MAX
#define WEAPON_HANDICAP_DEFAULT 9
#define WEAPON_FLAG_DOBJDESC 0x1
#define WEAPON_FLAG_DOBJLINKS 0x2
#define WEAPON_FLAG_COLLPROJECT (1u << 31)
#define WEAPON_FLAG_PARENT_FIGHTER 0
#define WEAPON_FLAG_PARENT_GROUND 1
#define WEAPON_FLAG_PARENT_WEAPON 2
#define WEAPON_FLAG_PARENT_ITEM 3
#define WEAPON_MASK_PARENT 0xF
#define WEAPON_REFLECT_TIME_DEFAULT 100
#define WEAPON_REFLECT_MUL_DEFAULT 1.8F
#define WEAPON_REFLECT_ADD_DEFAULT 0.99F
#define WEAPON_HOP_ANGLE_DEFAULT F_CLC_DTOR32(135.0F)

typedef enum WPKind {
    nWPKindFireball,
    nWPKindBlaster,
    nWPKindChargeShot,
    nWPKindSamusBomb,
    nWPKindCutter,
    nWPKindEggThrow,
    nWPKindYoshiStar,
    nWPKindBoomerang,
    nWPKindSpinAttack,
    nWPKindThunderJoltAir,
    nWPKindThunderJoltGround,
    nWPKindThunderHead,
    nWPKindThunderTrail,
    nWPKindPKFire,
    nWPKindPKThunderHead,
    nWPKindPKThunderTrail,
    nWPKindBulletNormal,
    nWPKindBulletHard,
    nWPKindArwingLaser2D,
    nWPKindArwingLaser3D,
    nWPKindLGunAmmo,
    nWPKindFFlowerFlame,
    nWPKindStarRodStar,
    nWPKindMonsterStart,
    nWPKindIwarkRock = nWPKindMonsterStart,
    nWPKindNyarsCoin,
    nWPKindLizardonFlame,
    nWPKindSpearSwarm,
    nWPKindKamexHydro,
    nWPKindStarmieSwift,
    nWPKindDogasSmog,
    nWPKindHitokageFlame,
    nWPKindFushigibanaRazor,
    nWPKindMonsterEnd = nWPKindFushigibanaRazor
} WPKind;

typedef struct WPDesc {
    u8 flags;
    s32 kind;
    void **p_weapon;
    intptr_t o_attributes;
    DObjTransformTypes transform_types;
    sb32 (*proc_update)(GObj *);
    sb32 (*proc_map)(GObj *);
    sb32 (*proc_hit)(GObj *);
    sb32 (*proc_shield)(GObj *);
    sb32 (*proc_hop)(GObj *);
    sb32 (*proc_setoff)(GObj *);
    sb32 (*proc_reflector)(GObj *);
    sb32 (*proc_absorb)(GObj *);
} WPDesc;

typedef struct WPAttributes {
    void *data;
    MObjSub ***p_mobjsubs;
    AObjEvent32 **anim_joints;
    AObjEvent32 ***p_matanim_joints;
    Vec3h attack_offsets[2];
    s16 map_coll_top;
    s16 map_coll_center;
    s16 map_coll_bottom;
    s16 map_coll_width;
    u32 unused_0x28_b0_5 : 6;
    s32 angle : 10;
    u32 size : 16;
    u32 knockback_weight : 10;
    u32 element : 4;
    u32 damage : 8;
    u32 knockback_scale : 10;
    ub32 unused_0x2F_b7 : 1;
    ub32 unused_0x2F_b6 : 1;
    ub32 can_shield : 1;
    ub32 can_absorb : 1;
    ub32 can_reflect : 1;
    ub32 can_hop : 1;
    ub32 can_rehit_fighter : 1;
    ub32 can_rehit_item : 1;
    u32 priority : 3;
    u32 sfx : 10;
    ub32 can_setoff : 1;
    u32 attack_count : 2;
    s32 shield_damage : 8;
    u32 unused_0x30_b0_21 : 22;
    u32 knockback_base : 10;
} WPAttributes;

typedef struct wpMarioFireballAttributes {
    s32 lifetime;
    f32 vel_terminal;
    f32 vel_min;
    f32 gravity;
    f32 collide_rebound;
    f32 rotate_speed;
    f32 angle_ground;
    f32 angle_air;
    f32 vel_base;
    void *p_weapon;
    intptr_t offset;
    f32 anim_frame;
} wpMarioFireballAttributes;

#ifndef SSB64_NDS_WP_ATTACK_COLL_DECLARED
#define SSB64_NDS_WP_ATTACK_COLL_DECLARED
typedef struct WPAttackPos {
    Vec3f pos_curr;
    Vec3f pos_prev;
    sb32 unk_wphitpos_0x18;
    Mtx44f mtx;
    f32 unk_wphitpos_0x5C;
} WPAttackPos;

typedef struct WPAttackColl {
    s32 attack_state;
    s32 damage;
    f32 stale;
    s32 element;
    Vec3f offsets[WEAPON_ATKCOLL_NUM_MAX];
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
    ub32 can_absorb : 1;
    ub32 can_not_heal : 1;
    ub32 can_shield : 1;
    u32 motion_attack_id : 6;
    u16 motion_count;
    GMStatFlags stat_flags;
    u16 stat_count;
    s32 attack_count;
    WPAttackPos attack_pos[WEAPON_ATKCOLL_NUM_MAX];
    GMAttackRecord attack_records[GMATTACKREC_NUM_MAX];
} WPAttackColl;
#endif

typedef struct WPStruct {
    struct WPStruct *next;
    GObj *weapon_gobj;
    GObj *owner_gobj;
    s32 kind;
    u8 team;
    u8 player;
    u8 handicap;
    s32 player_num;
    s32 lr;
    struct {
        f32 vel_ground;
        Vec3f vel_air;
    } physics;
    MPCollData coll_data;
    sb32 ga;
    WPAttackColl attack_coll;
    s32 hit_normal_damage;
    s32 hit_refresh_damage;
    s32 hit_attack_damage;
    s32 hit_shield_damage;
    f32 shield_collide_angle;
    Vec3f shield_collide_dir;
    GObj *reflect_gobj;
    GMStatFlags reflect_stat_flags;
    u16 reflect_stat_count;
    GObj *absorb_gobj;
    ub32 is_hitlag_victim : 1;
    ub32 is_hitlag_weapon : 1;
    u32 group_id;
    s32 lifetime;
    ub32 is_camera_follow : 1;
    ub32 is_static_damage : 1;
    alSoundEffect *p_sfx;
    u16 sfx_id;
    sb32 (*proc_update)(GObj *);
    sb32 (*proc_map)(GObj *);
    sb32 (*proc_hit)(GObj *);
    sb32 (*proc_shield)(GObj *);
    sb32 (*proc_hop)(GObj *);
    sb32 (*proc_setoff)(GObj *);
    sb32 (*proc_reflector)(GObj *);
    sb32 (*proc_absorb)(GObj *);
    sb32 (*proc_dead)(GObj *);
    union {
        struct {
            s32 index;
        } fireball;
        struct {
            s32 status;
            f32 angle;
            GObj *parent_gobj;
            GObj *trail_gobj[WPPKTHUNDER_PARTS_COUNT];
        } pkthunder;
        struct {
            s32 status;
            s32 trail_id;
            GObj *parent_gobj;
            GObj *head_gobj;
        } pkthunder_trail;
        u8 raw[32];
    } weapon_vars;
    s32 display_mode;
} WPStruct;

#define wpGetStruct(weapon_gobj) ((WPStruct *)(weapon_gobj)->user_data.p)

void wpManagerAllocWeapons(void);
WPStruct *wpManagerGetNextStructAlloc(void);
void wpManagerSetPrevStructAlloc(WPStruct *wp);
u32 wpManagerGetGroupID(void);
GObj *wpManagerMakeWeapon(GObj *parent_gobj, WPDesc *wp_desc,
                          Vec3f *spawn_pos, u32 flags);
sb32 wpMainDecLifeCheckExpire(WPStruct *wp);
void wpMainApplyGravityClampTVel(WPStruct *wp, f32 gravity,
                                 f32 terminal_velocity);
void wpMainVelSetModelPitch(GObj *weapon_gobj);
void wpMainReflectorSetLR(WPStruct *wp, FTStruct *fp);
s32 wpMainGetStaledDamage(WPStruct *wp);
void wpMainClearAttackRecord(WPStruct *wp);
void wpMainDestroyWeapon(GObj *weapon_gobj);
void wpProcessUpdateHitInteractStats(WPStruct *wp, WPAttackColl *attack_coll,
                                     GObj *victim_gobj, s32 attack_type,
                                     u32 victim_group_id);
void wpProcessUpdateHitPositions(GObj *weapon_gobj);
void wpProcessProcWeaponMain(GObj *weapon_gobj);
void wpProcessProcSearchHitWeapon(GObj *weapon_gobj);
void wpProcessProcHitCollisions(GObj *weapon_gobj);
sb32 wpMapTestAll(GObj *weapon_gobj);
sb32 wpMapCheckAllRebound(GObj *weapon_gobj, u32 check_flags, f32 mod_vel,
                          Vec3f *pos);
sb32 wpMarioFireballProcUpdate(GObj *weapon_gobj);
sb32 wpMarioFireballProcMap(GObj *weapon_gobj);
sb32 wpMarioFireballProcHit(GObj *weapon_gobj);
sb32 wpMarioFireballProcHop(GObj *weapon_gobj);
sb32 wpMarioFireballProcReflector(GObj *weapon_gobj);
GObj *wpMarioFireballMakeWeapon(GObj *fighter_gobj, Vec3f *pos, s32 index);
sb32 gmCollisionCheckWeaponInFighterRange(WPAttackColl *attack_coll,
                                          s32 attack_id, GObj *fighter_gobj);
sb32 gmCollisionCheckWeaponAttackFighterAttackCollide(
    WPAttackColl *wp_attack_coll, s32 attack_id, FTAttackColl *ft_attack_coll);
sb32 gmCollisionCheckWeaponAttackFighterDamageCollide(
    WPAttackColl *attack_coll, s32 attack_id, FTDamageColl *damage_coll);
sb32 gmCollisionCheckWeaponAttackShieldCollide(WPAttackColl *attack_coll,
                                               s32 attack_id,
                                               GObj *fighter_gobj,
                                               DObj *dobj, f32 *p_angle,
                                               Vec3f *vec);
sb32 gmCollisionCheckWeaponAttackSpecialCollide(WPAttackColl *attack_coll,
                                                s32 attack_id, FTStruct *fp,
                                                FTSpecialColl *special_coll);
void gmCollisionGetWeaponAttackFighterAttackPosition(
    Vec3f *dst, WPAttackColl *attack_coll, s32 attack_id,
    FTAttackColl *ft_attack_coll);
void gmCollisionGetWeaponAttackFighterDamagePosition(
    Vec3f *dst, WPAttackColl *attack_coll, s32 attack_id,
    FTDamageColl *damage_coll);
void gmCollisionGetWeaponAttackShieldPosition(Vec3f *dst,
                                              WPAttackColl *attack_coll,
                                              s32 attack_id, GObj *gobj,
                                              DObj *dobj);
sb32 gmCollisionCheckWeaponAttacksCollide(WPAttackColl *attack_coll1,
                                          s32 atk1_id,
                                          WPAttackColl *attack_coll2,
                                          s32 atk2_id);
void gmCollisionGetWeaponAttacksPosition(Vec3f *dst,
                                         WPAttackColl *attack_coll1,
                                         s32 atk1_id,
                                         WPAttackColl *attack_coll2,
                                         s32 atk2_id);

#endif
