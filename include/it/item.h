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
    void *attr;
    u8 colanim[0x20];
    ub32 is_hitlag_victim : 1;
    GObj *arrow_gobj;
    u8 arrow_timer;
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
    nITKindGLucky = nITKindGroundMonsterStart
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

void itManagerMakeAppearActor(void);
s32 itMainGetDamageOutput(ITStruct *ip);
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
