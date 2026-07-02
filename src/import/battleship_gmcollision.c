#include <ft/fighter.h>
#include <it/item.h>
#include <wp/weapon.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)(gobj)->obj)
#endif

#ifndef WEAPON_ATKCOLL_NUM_MAX
#define WEAPON_ATKCOLL_NUM_MAX 2
#endif

#ifndef ITEM_ATKCOLL_NUM_MAX
#define ITEM_ATKCOLL_NUM_MAX 2
#endif

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

f32 lbCommonSin(f32 angle);
f32 lbCommonCos(f32 angle);
void syDebugPrintf(const char *fmt, ...);
void scManagerRunPrintGObjStatus(void);

#include "../../decomp/BattleShip-main/decomp/src/gm/gmcollision.c"
