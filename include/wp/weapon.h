#ifndef SSB64_NDS_WEAPON_H
#define SSB64_NDS_WEAPON_H

#include <ft/fighter.h>

#ifndef WEAPON_ATKCOLL_NUM_MAX
#define WEAPON_ATKCOLL_NUM_MAX 2
#endif
#define WEAPON_REHIT_TIME_DEFAULT 16

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
    u8 coll_data[0x80];
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
} WPStruct;

#define wpGetStruct(weapon_gobj) ((WPStruct *)(weapon_gobj)->user_data.p)

s32 wpMainGetStaledDamage(WPStruct *wp);
void wpProcessUpdateHitInteractStats(WPStruct *wp, WPAttackColl *attack_coll,
                                     GObj *victim_gobj, s32 attack_type,
                                     u32 victim_group_id);
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

#endif
