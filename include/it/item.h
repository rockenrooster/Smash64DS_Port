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
    union {
        ITFighterItemVarsPKFire pkfire;
        ITFighterItemVarsLinkBomb linkbomb;
        ITCommonItemVarsBumper bumper;
        /* decomp it/itvars.h:573-578. Mushroom Kingdom sets the wait flag
         * on the Piranha occupying a pipe (grinishie.c:402). */
        struct {
            Vec3f pos;
            ub8 is_wait_fighter;
        } pakkun;
        /* decomp it/itvars.h:540-544. The Barrel's roll. */
        struct {
            f32 roll_rotate_step;
        } taru;
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
/* decomp it/itmain.c:575 and :615, owned by battleship_item_map_core.c.
 * The containers roll their payload through the first and drive their
 * attack-event script through the second. */
sb32 itMainMakeContainerItem(GObj *parent_gobj);
void itMainUpdateAttackEvent(GObj *item_gobj, ITAttackEvent *ev);
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
