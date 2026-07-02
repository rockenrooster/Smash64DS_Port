#ifndef SSB64_NDS_FIGHTER_H
#define SSB64_NDS_FIGHTER_H

/* Narrow fighter ABI shadow. The full BattleShip ft/fighter.h pulls the entire
 * fighter data tree (fttypes/ftfunctions/ftcommondata + per-fighter headers),
 * which is not imported in this increment. Only the FTKind enum and the
 * ftManagerSetupFileSize contract that the port's scene backend references are
 * exposed here. Full fighter headers are still deferred; this shadow keeps
 * scene/task slices compiling without importing the fighter tree. */
#include <PR/ultratypes.h>
#include <PR/gbi.h>
#include <stddef.h>
#include <ssb_types.h>
#include <sys/controller.h>
#include <sys/objtypes.h>

#ifndef U8_MAX
#define U8_MAX 255
#endif

typedef struct GObj GObj;
typedef struct FTData FTData;

typedef enum FTKind {
    nFTKindPlayableStart,
    nFTKindMario = nFTKindPlayableStart,
    nFTKindFox,
    nFTKindDonkey,
    nFTKindSamus,
    nFTKindLuigi,
    nFTKindLink,
    nFTKindYoshi,
    nFTKindCaptain,
    nFTKindKirby,
    nFTKindPikachu,
    nFTKindPurin,
    nFTKindNess,
    nFTKindPlayableEnd = nFTKindNess,
    nFTKindBoss,
    nFTKindMMario,
    nFTKindNStart,
    nFTKindNMario = nFTKindNStart,
    nFTKindNFox,
    nFTKindNDonkey,
    nFTKindNSamus,
    nFTKindNLuigi,
    nFTKindNLink,
    nFTKindNYoshi,
    nFTKindNCaptain,
    nFTKindNKirby,
    nFTKindNPikachu,
    nFTKindNPurin,
    nFTKindNNess,
    nFTKindNEnd = nFTKindNNess,
    nFTKindGDonkey,
    nFTKindEnumCount,
    nFTKindNull
} FTKind;

typedef enum FTPlayerKind {
    nFTPlayerKindMan,
    nFTPlayerKindCom,
    nFTPlayerKindNot,
    nFTPlayerKindDemo,
    nFTPlayerKindKey,
    nFTPlayerKindGameKey
} FTPlayerKind;

typedef struct FTFileSize {
    size_t main;
    size_t mainmotion_largest_anim;
    size_t submotion_largest_anim;
} FTFileSize;

typedef enum FTKeyEventKind {
    nFTKeyEventEnd,
    nFTKeyEventButton,
    nFTKeyEventStick
} FTKeyEventKind;

#ifndef I_CONTROLLER_RANGE_MAX
#define I_CONTROLLER_RANGE_MAX 80
#endif

#define FTDATA_FLAG_SUBMOTION 0x1
#define FTDATA_FLAG_MAINMOTION 0x2
#define FTDISPLAY_DLLINK_DEFAULT 5

#define LBBACKUP_MASK_FIGHTER(kind) (1u << (kind))
#define LBBACKUP_CHARACTER_MASK_ALL \
    (LBBACKUP_MASK_FIGHTER(nFTKindMario) | \
     LBBACKUP_MASK_FIGHTER(nFTKindFox) | \
     LBBACKUP_MASK_FIGHTER(nFTKindDonkey) | \
     LBBACKUP_MASK_FIGHTER(nFTKindSamus) | \
     LBBACKUP_MASK_FIGHTER(nFTKindLuigi) | \
     LBBACKUP_MASK_FIGHTER(nFTKindLink) | \
     LBBACKUP_MASK_FIGHTER(nFTKindYoshi) | \
     LBBACKUP_MASK_FIGHTER(nFTKindCaptain) | \
     LBBACKUP_MASK_FIGHTER(nFTKindKirby) | \
     LBBACKUP_MASK_FIGHTER(nFTKindPikachu) | \
     LBBACKUP_MASK_FIGHTER(nFTKindPurin) | \
     LBBACKUP_MASK_FIGHTER(nFTKindNess))
#define LBBACKUP_CHARACTER_MASK_UNLOCK \
    (LBBACKUP_MASK_FIGHTER(nFTKindNess) | \
     LBBACKUP_MASK_FIGHTER(nFTKindPurin) | \
     LBBACKUP_MASK_FIGHTER(nFTKindCaptain) | \
     LBBACKUP_MASK_FIGHTER(nFTKindLuigi))
#define LBBACKUP_CHARACTER_MASK_STARTER \
    (LBBACKUP_CHARACTER_MASK_ALL & ~LBBACKUP_CHARACTER_MASK_UNLOCK)

enum {
    nFTDemoStatusStance = 0,
    nFTDemoStatusWin1,
    nFTDemoStatusWin2,
    nFTDemoStatusWin3,
    nFTDemoStatusWin4
};

enum {
    nFTPartsDetailNone = 0,
    nFTPartsDetailStart = 1,
    nFTPartsDetailHigh = nFTPartsDetailStart,
    nFTPartsDetailLow = 2
};

enum {
    nFTPartsJointTopN = 0,
    nFTPartsJointTransN = 1,
    nFTPartsJointXRotN = 2,
    nFTPartsJointYRotN = 3,
    nFTPartsJointCommonStart = 4,
    nFTPartsJointNumMax = 74
};

#define FTKEY_EVENT_INSTRUCTION(k, t) \
    ((((u16)(k) << 12) & 0xF000u) | ((u16)(t) & 0x0FFFu))
#define FTKEY_EVENT_STICK(x, y, t) \
    FTKEY_EVENT_INSTRUCTION(nFTKeyEventStick, (t)), \
    ((((u16)(x) << 8) & 0xFF00u) | (((u16)(y) << 0) & 0x00FFu))
#define FTKEY_EVENT_BUTTON(b, t) \
    FTKEY_EVENT_INSTRUCTION(nFTKeyEventButton, (t)), ((u16)(b))
#define FTKEY_EVENT_END() FTKEY_EVENT_INSTRUCTION(nFTKeyEventEnd, 0)

#define FTCOMMON_DOWNWAIT_STAND_STICK_RANGE_MIN 20
#define FTCOMMON_DAMAGE_CATCH_RELEASE_THRESHOLD 6

typedef union FTKeyEvent {
    u16 halfword;
    struct {
        u16 opcode : 4;
        u16 param : 12;
    } command;
    Vec2b stick_range;
} FTKeyEvent;

typedef struct FTDesc {
    s32 fkind;
    Vec3f pos;
    s32 lr;
    u8 team;
    u8 player;
    u8 detail;
    u8 costume;
    u8 shade;
    u8 handicap;
    u8 level;
    u8 stock_count;
    u8 unk_rebirth_0x1C;
    u8 unk_rebirth_0x1D;
    u8 team_order;
    ub32 is_skip_entry : 1;
    ub32 is_skip_shadow_setup : 1;
    ub32 is_magnify_ignore : 1;
    s32 copy_kind;
    s32 damage;
    s32 pkind;
    SYController *controller;
    u16 button_mask_a;
    u16 button_mask_b;
    u16 button_mask_z;
    u16 button_mask_l;
    void *figatree_heap;
    void *proc_display;
} FTDesc;

typedef struct FTAttributes FTAttributes;

#ifndef SSB64_NDS_MPOBJECTCOLL_DECLARED
#define SSB64_NDS_MPOBJECTCOLL_DECLARED
typedef struct MPObjectColl {
    f32 top;
    f32 center;
    f32 bottom;
    f32 width;
} MPObjectColl;
#endif

typedef struct FTInputPlayer {
    Vec2b stick_range;
    Vec2b stick_prev;
    u16 button_hold;
    u16 button_tap;
    u16 button_release;
} FTInputPlayer;

typedef struct FTInputCPU {
    Vec2b stick_range;
    u16 button_inputs;
} FTInputCPU;

typedef struct FTInput {
    FTInputPlayer pl;
    FTInputCPU cp;
    SYController *controller;
    u16 button_mask_a;
    u16 button_mask_b;
    u16 button_mask_z;
    u16 button_mask_l;
} FTInput;

typedef struct FTCollisionData {
    Vec3f *p_translate;
    s32 *p_lr;
    MPObjectColl map_coll;
    MPObjectColl *p_map_coll;
    Vec2f cliffcatch_coll;
    s32 ignore_line_id;
    u32 update_tic;
    u32 mask_curr;
    s32 floor_line_id;
    s32 cliff_id;
    f32 floor_dist;
    u32 floor_flags;
    Vec3f floor_angle;
    s32 ceil_line_id;
    u32 ceil_flags;
    Vec3f ceil_angle;
    s32 lwall_line_id;
    u32 lwall_flags;
    Vec3f lwall_angle;
    s32 rwall_line_id;
    u32 rwall_flags;
    Vec3f rwall_angle;
    u32 mask_stat;
    sb32 is_coll_end;
    Vec3f pos_prev;
} FTCollisionData;

typedef enum GMAttackState {
    nGMAttackStateOff,
    nGMAttackStateNew,
    nGMAttackStateTransfer,
    nGMAttackStateInterpolate
} GMAttackState;

#define FTATTACKCOLL_NUM_MAX 4
#define FTDAMAGECOLL_NUM_MAX 11
#define GMATTACKREC_NUM_MAX 4

typedef struct GMHitFlags {
    u32 is_interact_hurt : 1;
    u32 is_interact_shield : 1;
    u32 is_interact_reflect : 1;
    u32 is_interact_absorb : 1;
    u32 group_id : 3;
    u32 timer_rehit : 6;
} GMHitFlags;

typedef struct GMAttackRecord {
    GObj *victim_gobj;
    GMHitFlags victim_flags;
} GMAttackRecord;

typedef struct FTAttackMatrix {
    sb32 unk_fthitmtx_0x0;
    Mtx44f mtx;
    f32 unk_fthitmtx_0x44;
} FTAttackMatrix;

typedef struct FTAttackColl {
    s32 attack_state;
    u32 group_id;
    s32 joint_id;
    s32 damage;
    s32 element;
    DObj *joint;
    Vec3f offset;
    f32 size;
    s32 angle;
    s32 knockback_scale;
    s32 knockback_weight;
    s32 knockback_base;
    s32 shield_damage;
    u32 fgm_level;
    u32 fgm_kind;
    sb32 is_hit_air;
    sb32 is_hit_ground;
    sb32 can_rebound;
    sb32 is_scale_pos;
    u32 motion_attack_id;
    u16 motion_count;
    u16 stat_count;
    Vec3f pos_curr;
    Vec3f pos_prev;
    GMAttackRecord attack_records[GMATTACKREC_NUM_MAX];
    FTAttackMatrix attack_matrix;
} FTAttackColl;

typedef struct FTDamageCollDesc {
    s32 joint_id;
    s32 placement;
    sb32 is_grabbable;
    Vec3f offset;
    Vec3f size;
} FTDamageCollDesc;

typedef struct FTDamageColl {
    s32 hitstatus;
    s32 joint_id;
    DObj *joint;
    s32 placement;
    sb32 is_grabbable;
    Vec3f offset;
    Vec3f size;
} FTDamageColl;

typedef struct FTHitLog {
    s32 attacker_object_class;
    void *attack_coll;
    s32 attack_id;
    GObj *attacker_gobj;
    FTDamageColl *damage_coll;
    u8 attacker_player;
    s32 attacker_player_num;
} FTHitLog;

typedef enum FTSpecialCollKind {
    nFTSpecialCollKindFoxReflector,
    nFTSpecialCollKindNessAbsorb,
    nFTSpecialCollKindNessReflector
} FTSpecialCollKind;

typedef struct FTSpecialColl {
    s32 kind;
    s32 joint_id;
    Vec3f offset;
    Vec3f size;
    s32 damage_resist;
} FTSpecialColl;

typedef struct FTParts {
    s32 transform_update_mode;
    union {
        struct {
            u8 unk_dobjtrans_0x4;
            u8 unk_dobjtrans_0x5;
            u8 unk_dobjtrans_0x6;
            u8 unk_dobjtrans_0x7;
        };
        s32 unk_dobjtrans_word;
    };
    struct FTParts *next;
    u8 flags;
    u8 joint_id;
    ub8 is_have_anim;
    u8 unk_dobjtrans_0xF;
    Mtx44f unk_dobjtrans_0x10;
    Mtx44f mtx_translate;
    Vec3f vec_scale;
    Mtx44f unk_dobjtrans_0x9C;
    GObj *gobj;
} FTParts;

#define ftGetParts(fighter_dobj) ((FTParts *)(fighter_dobj)->user_data.p)

typedef struct FTMotionScript {
    f32 script_wait;
    u32 *p_script;
    s32 script_id;
    void *p_goto[1];
    s32 loop_count[4];
} FTMotionScript;

typedef struct FTThrownStatus {
    s32 status1;
    s32 status2;
} FTThrownStatus;

typedef struct FTThrownStatusArray {
    FTThrownStatus ft_thrown[2];
} FTThrownStatusArray;

typedef struct FTThrowHitDesc {
    s32 status_id;
    s32 damage;
    s32 angle;
    s32 knockback_scale;
    s32 knockback_weight;
    s32 knockback_base;
    s32 element;
} FTThrowHitDesc;

#define NDS_FTSTRUCT_MAGIC 0x46545348u

enum {
    nFTStatusIDNone = -1
};

typedef union GMStatFlags {
    struct {
        u16 unused : 3;
        ub16 is_smash_attack : 1;
        ub16 ga : 1;
        ub16 is_projectile : 1;
        u16 attack_id : 10;
    };
    u16 halfword;
} GMStatFlags;

enum {
    nFTCommonMotionNull = -1,
    nFTCommonMotionEntryNull = 0,
    nFTCommonMotionRebirthDown = 1,
    nFTCommonMotionRebirthStand = 2,
    nFTCommonMotionRebirthWait = 3,
    nFTCommonMotionWait = 4,
    nFTCommonMotionWalkSlow = 5,
    nFTCommonMotionWalkMiddle = 6,
    nFTCommonMotionWalkFast = 7,
    nFTCommonMotionWalkEnd = 8,
    nFTCommonMotionDash = 9,
    nFTCommonMotionRun = 10,
    nFTCommonMotionRunBrake = 11,
    nFTCommonMotionTurn = 12,
    nFTCommonMotionTurnRun = 13,
    nFTCommonMotionKneeBend = 14,
    nFTCommonMotionGuardKneeBend = 15,
    nFTCommonMotionJumpF = 16,
    nFTCommonMotionJumpB = 17,
    nFTCommonMotionJumpAerialF = 18,
    nFTCommonMotionJumpAerialB = 19,
    nFTCommonMotionFall = 20,
    nFTCommonMotionFallAerial = 21,
    nFTCommonMotionSquat = 22,
    nFTCommonMotionSquatWait = 23,
    nFTCommonMotionSquatRv = 24,
    nFTCommonMotionLandingLight = 25,
    nFTCommonMotionLandingHeavy = 26,
    nFTCommonMotionPass = 27,
    nFTCommonMotionGuardPass = 28,
    nFTCommonMotionOttottoWait = 29,
    nFTCommonMotionOttotto = 30,
    nFTCommonMotionDamageHi1 = 31,
    nFTCommonMotionDamageHi2 = 32,
    nFTCommonMotionDamageHi3 = 33,
    nFTCommonMotionDamageN1 = 34,
    nFTCommonMotionDamageN2 = 35,
    nFTCommonMotionDamageN3 = 36,
    nFTCommonMotionDamageLw1 = 37,
    nFTCommonMotionDamageLw2 = 38,
    nFTCommonMotionDamageLw3 = 39,
    nFTCommonMotionDamageAir1 = 40,
    nFTCommonMotionDamageAir2 = 41,
    nFTCommonMotionDamageAir3 = 42,
    nFTCommonMotionDamageE = 43,
    nFTCommonMotionDamageFlyHi = 44,
    nFTCommonMotionDamageFlyN = 45,
    nFTCommonMotionDamageFlyLw = 46,
    nFTCommonMotionDamageFlyTop = 47,
    nFTCommonMotionDamageFlyRoll = 48,
    nFTCommonMotionGuardOn = 134,
    nFTCommonMotionGuardOff = 135,
    nFTCommonMotionEscapeF = 136,
    nFTCommonMotionEscapeB = 137,
    nFTCommonMotionShieldBreakFly = 138,
    nFTCommonMotionFuraFura = 144,
    nFTCommonMotionFuraSleep = 145,
    nFTCommonMotionCatch = 146,
    nFTCommonMotionCatchPull = 147,
    nFTCommonMotionThrowF = 148,
    nFTCommonMotionThrowB = 149,
    nFTCommonMotionCapturePulled = 150,
    nFTCommonMotionThrownCommon = 161,
    nFTCommonMotionWallDamage = 49,
    nFTCommonMotionDamageFall = 50,
    nFTCommonMotionTwister = 53,
    nFTCommonMotionStopCeil = 57,
    nFTCommonMotionDownBounceD = 58,
    nFTCommonMotionDownBounceU = 59,
    nFTCommonMotionDownStandD = 60,
    nFTCommonMotionDownStandU = 61,
    nFTCommonMotionPassiveStandF = 62,
    nFTCommonMotionPassiveStandB = 63,
    nFTCommonMotionDownForwardD = 64,
    nFTCommonMotionDownForwardU = 65,
    nFTCommonMotionDownBackD = 66,
    nFTCommonMotionDownBackU = 67,
    nFTCommonMotionDownAttackD = 68,
    nFTCommonMotionDownAttackU = 69,
    nFTCommonMotionPassive = 70,
    nFTCommonMotionRebound = 71,
    nFTCommonMotionCliffCatch = 72,
    nFTCommonMotionCliffWait = 73,
    nFTCommonMotionCliffQuick = 74,
    nFTCommonMotionCliffClimbQuick1 = 75,
    nFTCommonMotionCliffClimbQuick2 = 76,
    nFTCommonMotionCliffSlow = 77,
    nFTCommonMotionCliffClimbSlow1 = 78,
    nFTCommonMotionCliffClimbSlow2 = 79,
    nFTCommonMotionCliffAttackQuick1 = 80,
    nFTCommonMotionCliffAttackQuick2 = 81,
    nFTCommonMotionCliffAttackSlow1 = 82,
    nFTCommonMotionCliffAttackSlow2 = 83,
    nFTCommonMotionCliffEscapeQuick1 = 84,
    nFTCommonMotionCliffEscapeQuick2 = 85,
    nFTCommonMotionCliffEscapeSlow1 = 86,
    nFTCommonMotionCliffEscapeSlow2 = 87,
    nFTCommonMotionAppeal = 164,
    nFTCommonMotionAttack11 = 165,
    nFTCommonMotionAttack12 = 166,
    nFTCommonMotionAttackDash = 167,
    nFTCommonMotionAttackAirStart = 184,
    nFTCommonMotionAttackAirN = nFTCommonMotionAttackAirStart,
    nFTCommonMotionAttackAirF,
    nFTCommonMotionAttackAirB,
    nFTCommonMotionAttackAirHi,
    nFTCommonMotionAttackAirLw,
    nFTCommonMotionAttackAirEnd = nFTCommonMotionAttackAirLw,
    nFTCommonMotionLandingAirStart,
    nFTCommonMotionLandingAirN = nFTCommonMotionLandingAirStart,
    nFTCommonMotionLandingAirF,
    nFTCommonMotionLandingAirB,
    nFTCommonMotionLandingAirHi,
    nFTCommonMotionLandingAirLw,
    nFTCommonMotionLandingAirEnd = nFTCommonMotionLandingAirLw,
    nFTCommonMotionLandingAirNull,
    nFTCommonMotionSpecialStart = 195
};

enum {
    nFTCommonStatusDeadDown = 0,
    nFTCommonStatusDeadLeftRight = 1,
    nFTCommonStatusDeadUpStar = 2,
    nFTCommonStatusDeadUpFall = 3,
    nFTCommonStatusSleep = 4,
    nFTCommonStatusEntry = 5,
    nFTCommonStatusActionStart = 6,
    nFTCommonStatusEntryNull = nFTCommonStatusActionStart,
    nFTCommonStatusRebirthDown = 7,
    nFTCommonStatusRebirthStand = 8,
    nFTCommonStatusRebirthWait = 9,
    nFTCommonStatusControlStart = 10,
    nFTCommonStatusWait = nFTCommonStatusControlStart,
    nFTCommonStatusWalkSlow = 11,
    nFTCommonStatusWalkMiddle = 12,
    nFTCommonStatusWalkFast = 13,
    nFTCommonStatusWalkEnd = 14,
    nFTCommonStatusDash = 15,
    nFTCommonStatusRun = 16,
    nFTCommonStatusRunBrake = 17,
    nFTCommonStatusTurn = 18,
    nFTCommonStatusTurnRun = 19,
    nFTCommonStatusKneeBend = 20,
    nFTCommonStatusGuardKneeBend = 21,
    nFTCommonStatusJumpF = 22,
    nFTCommonStatusJumpB = 23,
    nFTCommonStatusJumpAerialF = 24,
    nFTCommonStatusJumpAerialB = 25,
    nFTCommonStatusFall = 26,
    nFTCommonStatusFallAerial = 27,
    nFTCommonStatusSquat = 28,
    nFTCommonStatusSquatWait = 29,
    nFTCommonStatusSquatRv = 30,
    nFTCommonStatusLandingLight = 31,
    nFTCommonStatusLandingHeavy = 32,
    nFTCommonStatusPass = 33,
    nFTCommonStatusGuardPass = 34,
    nFTCommonStatusOttottoWait = 35,
    nFTCommonStatusOttotto = 36,
    nFTCommonStatusDamageStart = 37,
    nFTCommonStatusDamageHi1 = nFTCommonStatusDamageStart,
    nFTCommonStatusDamageHi2 = 38,
    nFTCommonStatusDamageHi3 = 39,
    nFTCommonStatusDamageN1 = 40,
    nFTCommonStatusDamageN2 = 41,
    nFTCommonStatusDamageN3 = 42,
    nFTCommonStatusDamageLw1 = 43,
    nFTCommonStatusDamageLw2 = 44,
    nFTCommonStatusDamageLw3 = 45,
    nFTCommonStatusDamageAir1 = 46,
    nFTCommonStatusDamageAir2 = 47,
    nFTCommonStatusDamageAir3 = 48,
    nFTCommonStatusDamageE1 = 49,
    nFTCommonStatusDamageE2 = 50,
    nFTCommonStatusDamageFlyHi = 51,
    nFTCommonStatusDamageFlyN = 52,
    nFTCommonStatusDamageFlyLw = 53,
    nFTCommonStatusDamageFlyTop = 54,
    nFTCommonStatusDamageFlyRoll = 55,
    nFTCommonStatusDamageEnd = 56,
    nFTCommonStatusGuardOn = 152,
    nFTCommonStatusGuard = 153,
    nFTCommonStatusGuardOff = 154,
    nFTCommonStatusGuardSetOff = 155,
    nFTCommonStatusEscapeF = 156,
    nFTCommonStatusEscapeB = 157,
    nFTCommonStatusShieldBreakFly = 158,
    nFTCommonStatusFuraFura = 164,
    nFTCommonStatusFuraSleep = 165,
    nFTCommonStatusCatch = 166,
    nFTCommonStatusCatchPull = 167,
    nFTCommonStatusCatchWait = 168,
    nFTCommonStatusThrowF = 169,
    nFTCommonStatusThrowB = 170,
    nFTCommonStatusCapturePulled = 171,
    nFTCommonStatusCaptureWait = 172,
    nFTCommonStatusThrownStart = 181,
    nFTCommonStatusThrownCommon = 186,
    nFTCommonStatusThrownEnd = 188,
    nFTCommonStatusWallDamage = 56,
    nFTCommonStatusDamageFall = 57,
    nFTCommonStatusLandingFallSpecial = 59,
    nFTCommonStatusTwister = 60,
    nFTCommonStatusTaruCann = 61,
    nFTCommonStatusStopCeil = 66,
    nFTCommonStatusDownBounceD = 67,
    nFTCommonStatusDownBounceU = 68,
    nFTCommonStatusDownWaitD = 69,
    nFTCommonStatusDownWaitU = 70,
    nFTCommonStatusDownStandD = 71,
    nFTCommonStatusDownStandU = 72,
    nFTCommonStatusPassiveStandF = 73,
    nFTCommonStatusPassiveStandB = 74,
    nFTCommonStatusDownForwardD = 75,
    nFTCommonStatusDownForwardU = 76,
    nFTCommonStatusDownBackD = 77,
    nFTCommonStatusDownBackU = 78,
    nFTCommonStatusDownAttackD = 79,
    nFTCommonStatusDownAttackU = 80,
    nFTCommonStatusPassive = 81,
    nFTCommonStatusReboundWait = 82,
    nFTCommonStatusRebound = 83,
    nFTCommonStatusCliffCatch = 84,
    nFTCommonStatusCliffWait = 85,
    nFTCommonStatusCliffQuick = 86,
    nFTCommonStatusCliffClimbQuick1 = 87,
    nFTCommonStatusCliffClimbQuick2 = 88,
    nFTCommonStatusCliffSlow = 89,
    nFTCommonStatusCliffClimbSlow1 = 90,
    nFTCommonStatusCliffClimbSlow2 = 91,
    nFTCommonStatusCliffAttackQuick1 = 92,
    nFTCommonStatusCliffAttackQuick2 = 93,
    nFTCommonStatusCliffAttackSlow1 = 94,
    nFTCommonStatusCliffAttackSlow2 = 95,
    nFTCommonStatusCliffEscapeQuick1 = 96,
    nFTCommonStatusCliffEscapeQuick2 = 97,
    nFTCommonStatusCliffEscapeSlow1 = 98,
    nFTCommonStatusCliffEscapeSlow2 = 99,
    nFTCommonStatusLightThrowDrop = 103,
    nFTCommonStatusLightThrowDash = 104,
    nFTCommonStatusLightThrowF = 105,
    nFTCommonStatusLightThrowB,
    nFTCommonStatusLightThrowHi,
    nFTCommonStatusLightThrowLw,
    nFTCommonStatusLightThrowF4,
    nFTCommonStatusLightThrowB4,
    nFTCommonStatusLightThrowHi4,
    nFTCommonStatusLightThrowLw4,
    nFTCommonStatusLightThrowAirF,
    nFTCommonStatusLightThrowAirB,
    nFTCommonStatusLightThrowAirHi,
    nFTCommonStatusLightThrowAirLw,
    nFTCommonStatusLightThrowAirF4,
    nFTCommonStatusLightThrowAirB4,
    nFTCommonStatusLightThrowAirHi4,
    nFTCommonStatusLightThrowAirLw4,
    nFTCommonStatusAppeal = 189,
    nFTCommonStatusAttack11 = 190,
    nFTCommonStatusAttack12 = 191,
    nFTCommonStatusAttackDash = 192,
    nFTCommonStatusAttackAirStart = 209,
    nFTCommonStatusAttackAirN = nFTCommonStatusAttackAirStart,
    nFTCommonStatusAttackAirF,
    nFTCommonStatusAttackAirB,
    nFTCommonStatusAttackAirHi,
    nFTCommonStatusAttackAirLw,
    nFTCommonStatusAttackAirEnd = nFTCommonStatusAttackAirLw,
    nFTCommonStatusLandingAirStart,
    nFTCommonStatusLandingAirN = nFTCommonStatusLandingAirStart,
    nFTCommonStatusLandingAirF,
    nFTCommonStatusLandingAirB,
    nFTCommonStatusLandingAirHi,
    nFTCommonStatusLandingAirLw,
    nFTCommonStatusLandingAirNull,
    nFTCommonStatusLandingAirEnd = nFTCommonStatusLandingAirNull,
    nFTCommonStatusSpecialStart = 220,
};

enum {
    nFTItemSwingTypeAttack1 = 0,
    nFTItemSwingTypeAttack3 = 1,
    nFTItemSwingTypeAttack4 = 2,
    nFTItemSwingTypeAttackDash = 3
};

#define FTSTATUS_PRESERVE_NONE 0u
#define FTSTATUS_PRESERVE_EFFECT 0x4u
#define FTSTATUS_PRESERVE_FASTFALL 0x8u
#define FTSTATUS_PRESERVE_HITSTATUS 0x10u
#define FTSTATUS_PRESERVE_MODELPART 0x20u
#define FTSTATUS_PRESERVE_SLOPECONTOUR 0x40u
#define FTSTATUS_PRESERVE_TEXTUREPART 0x80u
#define FTSTATUS_PRESERVE_PLAYERTAG 0x100u
#define FTSTATUS_PRESERVE_SHUFFLETIME 0x400u
#define FTSTATUS_PRESERVE_DAMAGEPLAYER 0x1000u
#define F_CONTROLLER_RANGE_MAX 80.0F
#define FTCOMMON_WALKMIDDLE_STICK_RANGE_MIN 26
#define FTCOMMON_WALKFAST_STICK_RANGE_MIN 62
#define FTCOMMON_TURN_STICK_RANGE_MIN (-20)
#define FTCOMMON_TURNRUN_STICK_RANGE_MIN (-30)
#define FTCOMMON_DASH_BUFFER_TICS_MAX 3
#define FTCOMMON_DASH_STICK_RANGE_MIN 56
#define FTCOMMON_ATTACK1_FOLLOWUP_FRAMES_DEFAULT 24.0F
#define FTCOMMON_CATCH_THROW_STICK_RANGE_MIN 20
#define FTCOMMON_GUARD_RELEASE_LAG 8
#define FTCOMMON_GUARD_DECAY_INT 16
#define FTCOMMON_GUARD_ANGLE_MAX 359.0F
#define FTCOMMON_GUARD_SIZE_HEALTH_DIV 55.0F
#define FTCOMMON_GUARD_SIZE_SCALE_MUL_INIT 0.65F
#define FTCOMMON_GUARD_SIZE_SCALE_MUL_ADD 0.35F
#define FTCOMMON_GUARD_SIZE_SCALE_MUL_DIV 30.0F
#define FTCOMMON_GUARD_SETOFF_MUL 1.62F
#define FTCOMMON_GUARD_SETOFF_ADD 4.0F
#define FTCOMMON_GUARD_VEL_MUL 2.0F
#define FTCOMMON_ESCAPE_STICK_RANGE_MIN 56
#define FTCOMMON_ESCAPE_BUFFER_TICS_MAX 4

enum {
    nFTMotionAttackIDNone = -1,
    nFTMotionAttackIDAttack11 = 1,
    nFTMotionAttackIDAttack12 = 2,
    nFTMotionAttackIDAttack13 = 3,
    nFTMotionAttackIDAttack100 = 4,
    nFTMotionAttackIDAttackDash = 5,
    nFTMotionAttackIDAttackAirN = 12,
    nFTMotionAttackIDAttackAirF = 13,
    nFTMotionAttackIDAttackAirB = 14,
    nFTMotionAttackIDAttackAirHi = 15,
    nFTMotionAttackIDAttackAirLw = 16,
    nFTMotionAttackIDDownAttackD = 52,
    nFTMotionAttackIDDownAttackU = 53,
    nFTMotionAttackIDCliffAttackQuick = 54,
    nFTMotionAttackIDCliffAttackSlow = 55
};

enum {
    nFTStatusAttackIDNone = -1,
    nFTStatusAttackIDAttack11 = 1,
    nFTStatusAttackIDAttackDash = 2,
    nFTStatusAttackIDAttackAirN = 9,
    nFTStatusAttackIDAttackAirF = 10,
    nFTStatusAttackIDAttackAirB = 11,
    nFTStatusAttackIDAttackAirHi = 12,
    nFTStatusAttackIDAttackAirLw = 13,
    nFTStatusAttackIDAttack12 = 14,
    nFTStatusAttackIDAttack13 = 15,
    nFTStatusAttackIDAttack100 = 16,
    nFTStatusAttackIDDownAttackD = 32,
    nFTStatusAttackIDDownAttackU = 33,
    nFTStatusAttackIDCliffAttackQuick = 34,
    nFTStatusAttackIDCliffAttackSlow = 35,
    nFTStatusAttackIDBatSwing4 = 44
};

enum {
    nFTMarioMotionAttack13 = nFTCommonMotionSpecialStart,
    nFTFoxMotionAttack100Start = nFTCommonMotionSpecialStart,
    nFTFoxMotionAttack100Loop,
    nFTFoxMotionAttack100End,
    nFTMarioStatusAttack13 = nFTCommonStatusSpecialStart,
    nFTFoxStatusAttack100Start = nFTCommonStatusSpecialStart,
    nFTFoxStatusAttack100Loop,
    nFTFoxStatusAttack100End,
    nFTCaptainStatusAttack13 = nFTCommonStatusSpecialStart,
    nFTCaptainStatusAttack100Start = nFTCommonStatusSpecialStart,
    nFTCaptainStatusAttack100Loop,
    nFTCaptainStatusAttack100End,
    nFTLinkStatusAttack13 = nFTCommonStatusSpecialStart,
    nFTLinkStatusAttack100Start = nFTCommonStatusSpecialStart,
    nFTLinkStatusAttack100Loop,
    nFTLinkStatusAttack100End,
    nFTKirbyStatusAttack100Start = nFTCommonStatusSpecialStart,
    nFTKirbyStatusAttack100Loop,
    nFTKirbyStatusAttack100End,
    nFTPurinStatusAttack100Start = nFTCommonStatusSpecialStart,
    nFTPurinStatusAttack100Loop,
    nFTPurinStatusAttack100End,
    nFTNessStatusAttack13 = nFTCommonStatusSpecialStart
};
#define FTCOMMON_DASH_DECELERATE_BEGIN 7.0F
#define FTCOMMON_RUN_STICK_RANGE_MIN 50
#define FTCOMMON_KNEEBEND_INPUT_TYPE_NONE 0
#define FTCOMMON_KNEEBEND_INPUT_TYPE_STICK 1
#define FTCOMMON_KNEEBEND_INPUT_TYPE_BUTTON 2
#define FTCOMMON_KNEEBEND_BUFFER_TICS_MAX 3
#define FTCOMMON_KNEEBEND_JUMP_F_OR_B_RANGE (-10)
#define FTCOMMON_KNEEBEND_RUN_STICK_RANGE_MIN 44
#define FTCOMMON_KNEEBEND_STICK_RANGE_MIN 53
#define FTCOMMON_KNEEBEND_BUTTON_SHORT_FORCE 9.0F
#define FTCOMMON_KNEEBEND_BUTTON_LONG_FORCE 17.0F
#define FTCOMMON_OTTOTTO_WALK_DIST_X_MIN 60.0F
#define FTCOMMON_OTTOTTO_WALK_STICK_RANGE_MIN 60
#define FTCOMMON_KNEEBEND_BUTTON_SHORT_MIN 36.0F
#define FTCOMMON_KNEEBEND_BUTTON_LONG_MIN 63.0F
#define FTCOMMON_KNEEBEND_BUTTON_HEIGHT_CLAMP 77.0F
#define FTCOMMON_KNEEBEND_SHORTHOP_FRAMES 3.0F
#define FTCOMMON_FASTFALL_STICK_RANGE_MIN (-53)
#define FTCOMMON_FASTFALL_BUFFER_TICS_MAX 4
#define FTCOMMON_PASS_STICK_RANGE_MIN (-53)
#define FTCOMMON_PASS_BUFFER_TICS_MAX 4
#define FTCOMMON_SQUAT_STICK_RANGE_MIN (-53)
#define FTCOMMON_SQUAT_BUFFER_TICS_MAX 4
#define FTCOMMON_SQUAT_PASS_WAIT 3
#define FTCOMMON_LANDING_INTERRUPT_BEGIN 4.0F
#define FTCOMMON_LANDING_HEAVY_ANIM_SPEED 0.5F
#define FTCOMMON_LANDING_LIGHT_ANIM_SPEED 1.0F
#define FTCOMMON_ATTACKAIR_SMOOTHLANDING_TICS_MAX 10
#define FTCOMMON_ATTACKAIR_SKIPLANDING_VEL_Y_MAX (-20.0F)
#define FTCOMMON_ATTACKAIR_DIRECTION_STICK_RANGE_MIN 20
#define FTCOMMON_LIGHTTHROWAIR4_BUFFER_TICS_MAX 8
#define FTCOMMON_ATTACKAIRLW_LINK_REHIT_TIMER 30
#define FTCOMMON_ATTACKAIRLW_LINK_REHIT_FRAME_BEGIN 35.0F
#define FTCOMMON_ATTACKAIRLW_LINK_REHIT_FRAME_END 65.0F
#define FTCOMMON_ATTACKAIRLW_LINK_REHIT_BOUNCE_VEL_Y 40.0F
#define FTCOMMON_CLIFF_CATCH_WAIT 30
#define FTCOMMON_DOWNWAIT_STAND_WAIT 180
#define FTCOMMON_DOWNBOUNCE_ATTACK_BUFFER 60
#define FTCOMMON_DOWN_FORWARD_BACK_RANGE_MIN 20
#define FTCOMMON_CLIFF_DAMAGE_HIGH 100
#define FTCOMMON_CLIFF_FALL_WAIT_DAMAGE_HIGH 480
#define FTCOMMON_CLIFF_FALL_WAIT_DAMAGE_LOW 1080
#define FTCOMMON_CLIFF_MOTION_STICK_RANGE_MIN 20
#define FTCOMMON_PASSIVE_BUFFER_TICS_MAX 20
#define FTCOMMON_PASSIVE_F_OR_B_RANGE 20
#define FTCOMMON_FURASLEEP_BREAKOUT_WAIT_DEFAULT 300
#if defined(REGION_US)
#define FTCOMMON_FURASLEEP_BREAKOUT_WAIT_MIN 75
#else
#define FTCOMMON_FURASLEEP_BREAKOUT_WAIT_MIN 40
#endif
#define FTCOMMON_CAPTURE_MASH_STICK_RANGE_MIN 40
#define FTCOMMON_DAMAGE_SMASH_DI_BUFFER_TICS_MAX 4
#define FTCOMMON_DAMAGE_SMASH_DI_RANGE_MIN 53
#if defined(REGION_US)
#define FTCOMMON_DAMAGE_SMASH_DI_RANGE_MUL 2.1F
#else
#define FTCOMMON_DAMAGE_SMASH_DI_RANGE_MUL 1.5F
#endif
#define FTPHYSICS_AIRDRIFT_CLAMP_RANGE_MIN 8
#define FTINPUT_STICKBUFFER_TICS_MAX (U8_MAX - 1)
#define FTINPUT_ZTRIGLAST_TICS_MAX 65536
#define FTCATCHKIND_MASK_NONE 0x0u
#define FTCATCHKIND_MASK_TARUCANN 0x4u
#define FTCATCHKIND_MASK_YOSHISPECIALN 0x1u
#define FTCATCHKIND_MASK_KIRBYSPECIALN 0x2u
#define FTCATCHKIND_MASK_COMMON 0x10u
#define FTCATCHKIND_MASK_ALL 0x3fu
#define FTCOMMON_CATCH_THROW_WAIT 60
#define FTCATCHKIND_MASK_CAPTAINSPECIALHI 0x20u

enum {
    nMPKineticsGround = 0,
    nMPKineticsAir = 1
};

#define MAP_FLAG_LWALL 0x00000001u
#define MAP_FLAG_RWALL 0x00000020u
#define MAP_FLAG_CEIL 0x00000400u
#define MAP_FLAG_FLOOR 0x00000800u
#define MAP_FLAG_LCLIFF 0x00001000u
#define MAP_FLAG_RCLIFF 0x00002000u
#define MAP_FLAG_CEILHEAVY 0x00004000u
#define MAP_FLAG_FLOOREDGE 0x00008000u
#define MAP_FLAG_CLIFF_MASK (MAP_FLAG_LCLIFF | MAP_FLAG_RCLIFF)
#define MAP_VERTEX_COLL_CLIFF 0x00008000u
#define MAP_VERTEX_COLL_PASS 0x00004000u
#define MAP_VERTEX_COLL_BITS 0x0000ff00u
#define MAP_VERTEX_MAT_MASK (~MAP_VERTEX_COLL_BITS)

enum {
    nMPMaterialCommon = 0,
    nMPMaterial4 = 4
};

enum {
    nGMHitStatusNormal = 0,
    nGMHitStatusNone = 1,
    nGMHitStatusInvincible = 2,
    nGMHitStatusIntangible = 3
};

enum {
    nGMHitElementNormal = 0,
    nGMHitElementFire = 1,
    nGMHitElementElectric = 2,
    nGMHitElementSlash = 3,
    nGMHitElementCoin = 4,
    nGMHitElementFreezing = 5,
    nGMHitElementSleep = 6
};

enum {
    nGMHitEnvironmentAcid = 0,
    nGMHitEnvironmentPowerBlock = 1,
    nGMHitEnvironmentTwister = 2,
    nGMHitEnvironmentTaruCann = 3
};

enum {
    nFTHitLogObjectNone = 0,
    nFTHitLogObjectFighter = 1,
    nFTHitLogObjectWeapon = 2,
    nFTHitLogObjectItem = 3,
    nFTHitLogObjectGround = 4
};

enum {
    nFTDamageKindDefault = 0,
    nFTDamageKindStatus = 1,
    nFTDamageKindColAnim = 2,
    nFTDamageKindCatch = 3,
    nFTDamageKindNone = 4
};

enum {
    nFTCommonCliffKindClimbQuick = 0,
    nFTCommonCliffKindAttackQuick = 1,
    nFTCommonCliffKindEscapeQuick = 2,
    nFTCommonCliffKindClimbSlow = 3,
    nFTCommonCliffKindAttackSlow = 4,
    nFTCommonCliffKindEscapeSlow = 5,
    nFTCommonCliffKindEnumCount = 6
};

typedef struct FTMotionFlags {
    s16 motion_id : 10;
    u16 attack_id : 6;
} FTMotionFlags;

typedef struct FTStatusDesc {
    FTMotionFlags mflags;
    GMStatFlags sflags;
    void (*proc_update)(GObj *);
    void (*proc_interrupt)(GObj *);
    void (*proc_physics)(GObj *);
    void (*proc_map)(GObj *);
} FTStatusDesc;

typedef struct FTPhysics {
    Vec3f vel_air;
    Vec3f vel_damage_air;
    Vec3f vel_ground;
    f32 vel_damage_ground;
    Vec3f vel_push;
    f32 vel_jostle_x;
    f32 vel_jostle_z;
} FTPhysics;

typedef union FTAnimDesc {
    u32 word;
    struct {
        ub32 is_use_xrotn_joint : 1;
        ub32 is_use_transn_joint : 1;
        ub32 is_use_yrotn_joint : 1;
        ub32 is_enabled_joints : 24;
        ub32 is_use_submotion_script : 1;
        ub32 is_anim_joint : 1;
        ub32 is_have_translate_scale : 1;
        ub32 is_use_shieldpose : 1;
        ub32 is_use_animlocks : 1;
    } flags;
} FTAnimDesc;

typedef struct FTMotionDesc {
    u32 anim_file_id;
    intptr_t offset;
    FTAnimDesc anim_desc;
} FTMotionDesc;

typedef struct FTMotionDescArray {
    FTMotionDesc motion_desc[1];
} FTMotionDescArray;

struct FTData {
    FTMotionDescArray *mainmotion;
};

typedef union FTMotionVars {
    struct {
        ub32 flag0 : 1;
        ub32 flag1 : 1;
        ub32 flag2 : 1;
        ub32 flag3 : 1;
        ub32 flag4 : 1;
        ub32 flag5 : 1;
        ub32 flag6 : 1;
        ub32 flag7 : 1;
    } flags;
    u32 word;
} FTMotionVars;

typedef union FTStatusVars {
    struct {
        struct {
            sb32 is_allow_turn_direction;
            sb32 is_disable_sa_interrupts;
            u16 button_mask;
            s32 lr_dash;
            s32 lr_turn;
            s32 attacks4_buffer;
        } turn;
        struct {
            s32 jump_force;
            f32 anim_frame;
            s32 input_source;
            sb32 is_shorthop;
        } kneebend;
        struct {
            sb32 is_allow_interrupt;
        } landing;
        struct {
            s32 hitstun_tics;
            s32 dust_effect_int;
            f32 public_knockback;
            u16 coll_mask_curr;
            u16 coll_mask_prev;
            u16 coll_mask_ignore;
            Vec3f wall_collide_angle;
            s32 unk_0xB38;
            s32 unk_0xB3C;
            s32 script_id;
            s32 status_id;
            sb32 is_knockback_over;
        } damage;
        struct {
            sb32 is_allow_pass;
            s32 pass_wait;
            s32 unk_0x8;
        } squat;
        struct {
            sb32 is_allow_interrupt;
            s32 fall_wait;
        } cliffwait;
        struct {
            s32 status_id;
            s32 cliff_id;
        } cliffmotion;
        struct {
            s32 stand_wait;
        } downwait;
        struct {
            s32 attack_buffer;
        } downbounce;
        struct {
            sb32 is_goto_followup;
            s32 interrupt_catch_timer;
        } attack1;
        struct {
            sb32 is_anim_end;
            sb32 is_goto_loop;
        } attack100;
        struct {
            s32 release_lag;
            s32 shield_decay_wait;
            GObj *effect_gobj;
            sb32 is_release;
            s32 angle_i;
            f32 angle_f;
            f32 shield_rotate_range;
            f32 setoff_frames;
            s32 slide_tics;
            sb32 is_setoff;
        } guard;
        struct {
            f32 catch_pull_frame_begin;
            f32 catch_pull_anim_frames;
        } catchmain;
        struct {
            s32 throw_wait;
        } catchwait;
        struct {
            sb32 is_goto_pulled_wait;
        } capture;
        struct {
            s32 status_id;
        } thrown;
        struct {
            s32 itemthrow_buffer_tics;
        } escape;
        struct {
            f32 anim_speed;
            f32 rebound_timer;
        } rebound;
        struct {
            s32 rehit_timer;
        } attackair;
        struct {
            s32 release_wait;
            GObj *tornado_gobj;
        } twister;
        struct {
            s32 release_wait;
            s32 shoot_wait;
            GObj *tarucann_gobj;
        } tarucann;
    } common;
} FTStatusVars;

typedef struct ftKirbyAttack100Effect {
    Vec3f offset;
    f32 rotate;
    f32 vel;
    f32 add;
} ftKirbyAttack100Effect;

typedef struct FTStruct {
    s32 pkind;
    GObj *fighter_gobj;
    s32 fkind;
    FTData *data;
    FTAttributes *attr;
    void *figatree_heap;

    u8 team;
    u8 player;
    u8 stock_count;
    u8 detail_curr;
    u8 detail_base;
    u8 costume;
    u8 shade;
    SYColorRGBA shade_color;

    u8 handicap;
    u8 level;
    s32 lr;
    s32 damage;
    f32 damage_mul;

    s32 status_id;
    s32 status_prev;
    s32 status_total_tics;
    f32 motion_frame;
    f32 anim_frame;
    f32 anim_speed;
    FTAnimDesc anim_desc;

    sb32 is_invisible;
    sb32 is_shadow_hide;
    sb32 is_magnify_ignore;
    sb32 is_have_translate_scale;
    sb32 is_playertag_hide;
    sb32 is_ghost;

    s32 display_mode;
    s32 dl_link;

    f32 camera_zoom_frame;

    FTInput input;
    u8 tap_stick_x;
    u8 tap_stick_y;
    u8 hold_stick_x;
    u8 hold_stick_y;
    f32 camera_zoom_range;
    FTCollisionData coll_data;

    DObj *joints[nFTPartsJointNumMax];
    FTAttackColl attack_colls[FTATTACKCOLL_NUM_MAX];
    FTDamageColl damage_colls[FTDAMAGECOLL_NUM_MAX];

    s32 percent_damage;
    s32 damage_resist;
    s32 shield_health;
    f32 unk_ft_0x38;
    s32 hitlag_tics;
    sb32 is_knockback_paused;

    Vec3f vel_air;
    Vec3f vel_ground;
    Vec3f vel_push;
    FTPhysics physics;

    s32 ga;
    s32 jumps_used;
    sb32 is_reflect;
    sb32 is_absorb;
    sb32 is_shield;
    sb32 is_attack_active;
    sb32 is_effect_attach;
    sb32 is_jostle_ignore;
    sb32 is_cliff_hold;
    sb32 is_ignore_dead;
    sb32 is_damage_resist;
    u8 capture_immune_mask;
    u8 catch_mask;

    s32 cliffcatch_wait;
    s32 breakout_wait;
    s32 breakout_lr;
    s32 breakout_ud;
    s32 tics_since_last_z;
    s32 acid_wait;
    s32 twister_wait;
    s32 tarucann_wait;
    s32 damagefloor_wait;

    s32 attack_damage;
    s32 attack_count;
    s32 attack_shield_push;
    s32 hit_lr;
    s32 shield_damage;
    s32 shield_damage_total;
    s32 shield_lr;
    s32 damage_lag;
    s32 damage_queue;
    s32 damage_angle;
    s32 damage_element;
    s32 damage_lr;
    s32 damage_index;
    s32 damage_player_num;
    s32 damage_player;
    s32 damage_object_class;
    s32 damage_object_kind;
    s32 damage_count;
    s32 damage_kind;
    s32 damage_heal;
    s32 damage_joint_id;

    s32 invincible_tics;
    s32 intangible_tics;
    s32 star_invincible_tics;

    s32 hitstatus;
    s32 star_hitstatus;
    s32 special_hitstatus;

    GObj *throw_gobj;
    s32 throw_fkind;
    u8 throw_team;
    u8 throw_player;
    s32 throw_player_num;
    FTThrowHitDesc *throw_desc;
    GObj *search_gobj;
    f32 search_gobj_dist;
    void (*proc_catch)(GObj *);
    void (*proc_capture)(GObj *, GObj *);
    GObj *catch_gobj;
    GObj *capture_gobj;
    sb32 is_catchstatus;
    sb32 is_catch_or_capture;
    sb32 is_shield_catch;
    GObj *item_gobj;
    FTSpecialColl *special_coll;

    s32 reflect_lr;
    s32 absorb_lr;
    s32 reflect_damage;

    f32 attack1_followup_frames;
    s32 attack1_status_id;
    s32 attack1_input_count;
    f32 attack_knockback;
    f32 attack_rebound;
    f32 damage_knockback_stack;
    f32 knockback_resist_status;
    f32 knockback_resist_passive;
    f32 damage_knockback;
    f32 hitlag_mul;
    f32 shield_heal_wait;

    sb32 is_fastfall;
    s32 player_num;
    f32 public_knockback;
    sb32 is_hitstun;
    sb32 is_use_animlocks;
    s32 shuffle_frame_index;
    s32 shuffle_index_max;
    sb32 is_use_fogcolor;
    sb32 is_shuffle_electric;
    s32 shuffle_tics;

    s32 motion_attack_id;
    s32 motion_id;
    s32 motion_script_id;
    FTMotionScript motion_scripts[2][2];
    s32 status_attack_id;
    s32 status_is_smash;
    s32 status_is_projectile;
    u32 status_flags;
    s32 motion_count;
    s32 stat_attack_id;
    s32 stat_count;
    s32 damage_stat_count;
    GMStatFlags stat_flags;

    void (*proc_update)(GObj *);
    void (*proc_interrupt)(GObj *);
    void (*proc_physics)(GObj *);
    void (*proc_map)(GObj *);
    void (*proc_slope)(GObj *);
    void (*proc_status)(GObj *);
    void (*proc_damage)(GObj *);
    void (*proc_trap)(GObj *);
    void (*proc_shield)(GObj *);
    void (*proc_hit)(GObj *);
    void (*proc_lagstart)(GObj *);
    void (*proc_lagupdate)(GObj *);
    void (*proc_passive)(GObj *);
    void (*proc_lagend)(GObj *);

    FTMotionVars motion_vars;
    FTStatusVars status_vars;

    sb32 is_special_interrupt;
    s32 playertag_wait;
    sb32 is_wait_status_setup;
    sb32 is_wait_motion_setup;
    sb32 is_goto_attack100;

    union {
        struct {
            sb32 is_expend_tornado;
        } mario;
        struct {
            s32 reserved;
        } fox;
        struct {
            s32 copy_id;
            sb32 is_ignore_losecopy;
        } kirby;
    } passive_vars;

    u32 nds_magic;
    u32 nds_slot;
    u32 nds_joint_count;
    u32 nds_common_joint_count;
    u32 nds_init_mask;
    u32 nds_init_floor_project_attempted;
    u32 nds_init_floor_project_result;
} FTStruct;

typedef struct FTCommonPart {
    DObjDesc *dobjdesc;
    MObjSub ***p_mobjsubs;
    AObjEvent32 ***p_costume_matanim_joints;
    u8 flags;
} FTCommonPart;

typedef struct FTCommonPartContainer {
    FTCommonPart commonparts[2];
} FTCommonPartContainer;

typedef struct FTModelPart {
    Gfx *dl;
    MObjSub **mobjsubs;
    AObjEvent32 **costume_matanim_joints;
    AObjEvent32 **main_matanim_joints;
    u8 flags;
} FTModelPart;

typedef struct FTModelPartDesc {
    FTModelPart modelparts[1][2];
} FTModelPartDesc;

typedef struct FTModelPartContainer {
    FTModelPartDesc *modelparts_desc[nFTPartsJointNumMax -
                                     nFTPartsJointCommonStart];
} FTModelPartContainer;

typedef struct FTAttributes {
    f32 size;
    f32 walkslow_anim_length;
    f32 walkmiddle_anim_length;
    f32 walkfast_anim_length;
    f32 throw_walkslow_anim_length;
    f32 throw_walkmiddle_anim_length;
    f32 throw_walkfast_anim_length;
    f32 rebound_anim_length;
    f32 walk_speed_mul;
    f32 traction;
    f32 dash_speed;
    f32 dash_decel;
    f32 run_speed;
    f32 kneebend_anim_length;
    f32 jump_vel_x;
    f32 jump_height_mul;
    f32 jump_height_base;
    f32 jumpaerial_vel_x;
    f32 jumpaerial_height;
    f32 air_accel;
    f32 air_speed_max_x;
    f32 air_friction;
    f32 gravity;
    f32 tvel_base;
    f32 tvel_fast;
    s32 jumps_max;
    f32 weight;
    f32 attack1_followup_frames;
    f32 dash_to_run;
    f32 shield_size;
    u8 filler_0x078[0x09C - 0x078];
    MPObjectColl map_coll;
    Vec2f cliffcatch_coll;
    u16 dead_fgm_ids[2];
    u16 deadup_sfx;
    u16 damage_sfx;
    u8 filler_0x0BC[0x100 - 0x0BC];
    ub32 filler_flags_low    : 10;
    ub32 is_have_voice       : 1;
    ub32 is_have_catch       : 1;
    ub32 is_have_specialairlw: 1;
    ub32 is_have_speciallw   : 1;
    ub32 is_have_specialairhi: 1;
    ub32 is_have_specialhi   : 1;
    ub32 is_have_specialairn : 1;
    ub32 is_have_specialn    : 1;
    ub32 is_have_attackairlw : 1;
    ub32 is_have_attackairhi : 1;
    ub32 is_have_attackairb  : 1;
    ub32 is_have_attackairf  : 1;
    ub32 is_have_attackairn  : 1;
    ub32 is_have_attacklw4   : 1;
    ub32 is_have_attackhi4   : 1;
    ub32 is_have_attacks4    : 1;
    ub32 is_have_attacklw3   : 1;
    ub32 is_have_attackhi3   : 1;
    ub32 is_have_attacks3    : 1;
    ub32 is_have_attackdash  : 1;
    ub32 is_have_attack12    : 1;
    ub32 is_have_attack11    : 1;
    FTDamageCollDesc damage_coll_descs[FTDAMAGECOLL_NUM_MAX];
    Vec3f hit_detect_range;
    u8 filler_0x29C[0x2B8 - 0x29C];
    s32 cliff_status_ga[5];
    u8 filler_0x2CC[0x2D4 - 0x2CC];
    FTCommonPartContainer *commonparts_container;
    DObjDesc *dobj_lookup;
    AObjEvent32 **shield_anim_joints[8];
    s32 joint_rfoot_id;
    f32 joint_rfoot_rotate;
    s32 joint_lfoot_id;
    f32 joint_lfoot_rotate;
    u8 filler_0x30C[0x31C - 0x30C];
    f32 unk_0x31C;
    f32 unk_0x320;
    Vec3f *translate_scales;
    FTModelPartContainer *modelparts_container;
    void *accesspart;
    void *textureparts_container;
    s32 joint_itemheavy_id;
    FTThrownStatusArray *thrown_status;
    s32 joint_itemlight_id;
    void *sprites;
    void *skeleton;
} FTAttributes;

extern size_t gFTManagerFigatreeHeapSize;
extern FTDesc dFTManagerDefaultFighterDesc;
extern f32 dSCSubsysFighterScales[];

void ftManagerSetupFileSize(void);
void ftManagerAllocFighter(u32 data_flags, s32 allocs_num);
void ftManagerSetupFilesAllKind(s32 fkind);
void ftManagerSetupFilesPlayablesAll(void);
void *ftManagerAllocFigatreeHeapKind(s32 fkind);
GObj *ftManagerMakeFighter(FTDesc *desc);
void ftManagerDestroyFighter(GObj *fighter_gobj);
void ftPublicMakeActor(void);
void ftParamInitGame(void);
void ftParamInitPlayerBattleStats(s32 player, GObj *fighter_gobj);
void ftParamSetKey(GObj *fighter_gobj, FTKeyEvent *script);
s32 ftParamGetCostumeCommonID(s32 fkind, s32 color);
s32 ftParamGetCostumeTeamID(s32 fkind, s32 color);
void ftParamInitAllParts(GObj *fighter_gobj, s32 costume, s32 shade);
sb32 ftParamCheckSetFighterColAnimID(GObj *fighter_gobj, s32 colanim_id,
                                     s32 unused);
sb32 ftParamCheckSetSkeletonColAnimID(GObj *fighter_gobj, s32 damage_level);
void ftMainRunUpdateColAnim(GObj *fighter_gobj);
void ftCommonDamageSetPublic(FTStruct *fp, f32 knockback, f32 angle);
void ftCommonDamageSetDustEffectInterval(FTStruct *fp);
f32 ftCommonDamageGetKnockbackAngle(s32 angle_i, sb32 ga, f32 knockback);
s32 ftCommonDamageGetDamageLevel(f32 hitstun);
sb32 ftCommonDamageCheckCatchResist(FTStruct *fp);
sb32 ftCommonDamageCheckCaptureKeepHold(FTStruct *fp);
sb32 ftCommonDamageCheckElementSetColAnim(GObj *fighter_gobj, s32 element,
                                          s32 damage_level);
void ftCommonDamageCheckMakeScreenFlash(f32 knockback, s32 element);
void ftCommonDamageUpdateDustEffect(GObj *fighter_gobj);
void ftCommonDamageDecHitStunSetPublic(GObj *fighter_gobj);
void ftCommonDamageUpdateCatchResist(GObj *fighter_gobj);
void ftCommonDamageUpdateDamageColAnim(GObj *fighter_gobj, f32 knockback,
                                       s32 element);
void ftCommonDamageSetDamageColAnim(GObj *fighter_gobj);
void ftCommonDamageUpdateMain(GObj *fighter_gobj);
void ftPhysicsStopVelAll(GObj *fighter_gobj);
void ftPhysicsApplyGroundVelFriction(GObj *fighter_gobj);
void ftParamClearAttackCollAll(GObj *fighter_gobj);
void ftParamSetHitStatusPartAll(GObj *fighter_gobj, s32 hitstatus);
void ftParamResetFighterColAnim(GObj *fighter_gobj);
void ftParamSetPlayerTagWait(GObj *fighter_gobj, s32 wait);
FTStruct *ftGetStruct(GObj *fighter_gobj);
void ftParamSetVelPush(GObj *fighter_gobj, Vec3f *vel);
void ftDisplayLightsDrawReflect(Gfx **display_list, f32 light_angle_x,
                                f32 light_angle_y);
void ftDisplayMainProcDisplay(GObj *fighter_gobj);
sb32 ftHammerCheckHoldHammer(GObj *fighter_gobj);
void ftHammerProcInterrupt(GObj *fighter_gobj);
void ftHammerSetStatusHammerWait(GObj *fighter_gobj);
sb32 ftCommonGroundCheckInterrupt(GObj *fighter_gobj);
void ftCommonWaitSetStatus(GObj *fighter_gobj);
void ftCommonWaitProcInterrupt(GObj *fighter_gobj);
sb32 ftCommonWaitCheckInterruptCommon(GObj *fighter_gobj);
f32 ftCommonWalkGetWalkAnimLength(FTStruct *fp, s32 status_id);
s32 ftCommonWalkGetWalkStatus(s8 stick_range_x);
void ftCommonWalkProcInterrupt(GObj *fighter_gobj);
void ftCommonWalkProcPhysics(GObj *fighter_gobj);
void ftCommonWalkSetStatusParam(GObj *fighter_gobj, f32 anim_frame_begin);
void ftCommonWalkSetStatusDefault(GObj *fighter_gobj);
sb32 ftCommonWalkCheckInputSuccess(GObj *fighter_gobj);
sb32 ftCommonWalkCheckInterruptCommon(GObj *fighter_gobj);
sb32 ftCommonSpecialNCheckInterruptCommon(GObj *fighter_gobj);
sb32 ftCommonSpecialHiCheckInterruptCommon(GObj *fighter_gobj);
sb32 ftCommonSpecialLwCheckInterruptCommon(GObj *fighter_gobj);
void ftCommonCatchProcUpdate(GObj *fighter_gobj);
void ftCommonCatchCaptureSetStatusRelease(GObj *fighter_gobj);
void ftCommonCatchProcMap(GObj *fighter_gobj);
void ftCommonCatchSetStatus(GObj *fighter_gobj);
sb32 ftCommonCatchCheckInterruptGuard(GObj *fighter_gobj);
sb32 ftCommonCatchCheckInterruptCommon(GObj *fighter_gobj);
void ftCommonCatchPullProcUpdate(GObj *fighter_gobj);
void ftCommonCatchWaitProcInterrupt(GObj *fighter_gobj);
void ftCommonCatchWaitSetStatus(GObj *fighter_gobj);
void ftCommonCapturePulledProcPhysics(GObj *fighter_gobj);
void ftCommonCapturePulledProcMap(GObj *fighter_gobj);
void ftCommonCaptureWaitProcMap(GObj *fighter_gobj);
void ftCommonCaptureWaitSetStatus(GObj *fighter_gobj);
sb32 ftCommonAttackS4CheckInterruptCommon(GObj *fighter_gobj);
sb32 ftCommonAttackS4CheckInterruptTurn(GObj *fighter_gobj);
sb32 ftCommonAttackHi4CheckInterruptCommon(GObj *fighter_gobj);
sb32 ftCommonAttackLw4CheckInterruptCommon(GObj *fighter_gobj);
sb32 ftCommonAttackLw4CheckInterruptSquat(GObj *fighter_gobj);
sb32 ftCommonAttackS3CheckInterruptCommon(GObj *fighter_gobj);
sb32 ftCommonAttackHi3CheckInterruptCommon(GObj *fighter_gobj);
sb32 ftCommonAttackLw3CheckInterruptCommon(GObj *fighter_gobj);
sb32 ftCommonAttack1CheckInterruptCommon(GObj *fighter_gobj);
void ftCommonAttack11ProcUpdate(GObj *fighter_gobj);
void ftCommonAttack11ProcInterrupt(GObj *fighter_gobj);
void ftCommonAttack11SetStatus(GObj *fighter_gobj);
sb32 ftCommonAttack11CheckGoto(GObj *fighter_gobj);
void ftCommonAttack12ProcUpdate(GObj *fighter_gobj);
void ftCommonAttack12ProcInterrupt(GObj *fighter_gobj);
void ftCommonAttack12SetStatus(GObj *fighter_gobj);
sb32 ftCommonGuardOnCheckInterruptCommon(GObj *fighter_gobj);
sb32 ftCommonAppealCheckInterruptCommon(GObj *fighter_gobj);
sb32 ftCommonEscapeCheckInterruptGuard(GObj *fighter_gobj);
sb32 ftCommonKneeBendCheckInterruptCommon(GObj *fighter_gobj);
void ftCommonKneeBendProcUpdate(GObj *fighter_gobj);
void ftCommonKneeBendProcInterrupt(GObj *fighter_gobj);
void ftCommonKneeBendSetStatusParam(GObj *fighter_gobj, s32 status_id,
                                    s32 input_source);
void ftCommonKneeBendSetStatus(GObj *fighter_gobj, s32 input_source);
void ftCommonGuardKneeBendSetStatus(GObj *fighter_gobj, s32 input_source);
sb32 ftCommonKneeBendCheckButtonTap(FTStruct *fp);
s32 ftCommonKneeBendGetInputTypeCommon(FTStruct *fp);
s32 ftCommonKneeBendGetInputTypeRun(FTStruct *fp);
sb32 ftCommonGuardKneeBendCheckInterruptGuard(GObj *fighter_gobj);
sb32 ftCommonDashCheckInterruptCommon(GObj *fighter_gobj);
void ftCommonDashProcUpdate(GObj *fighter_gobj);
void ftCommonDashProcInterrupt(GObj *fighter_gobj);
void ftCommonDashProcPhysics(GObj *fighter_gobj);
void ftCommonDashProcMap(GObj *fighter_gobj);
void ftCommonDashSetStatus(GObj *fighter_gobj, u32 flag);
sb32 ftCommonDashCheckTurn(GObj *fighter_gobj);
void ftCommonRunProcInterrupt(GObj *fighter_gobj);
void ftCommonRunSetStatus(GObj *fighter_gobj);
sb32 ftCommonRunCheckInterruptDash(GObj *fighter_gobj);
void ftCommonRunBrakeProcInterrupt(GObj *fighter_gobj);
void ftCommonRunBrakeProcPhysics(GObj *fighter_gobj);
void ftCommonRunBrakeSetStatus(GObj *fighter_gobj, u32 flag);
sb32 ftCommonRunBrakeCheckInterruptRun(GObj *fighter_gobj);
sb32 ftCommonRunBrakeCheckInterruptTurnRun(GObj *fighter_gobj);
sb32 ftCommonAttackS4CheckInterruptDash(GObj *fighter_gobj);
sb32 ftCommonEscapeCheckInterruptDash(GObj *fighter_gobj);
sb32 ftCommonCatchCheckInterruptDashRun(GObj *fighter_gobj);
void ftParamSetCatchParams(FTStruct *fp, u8 catch_mask,
                           void (*proc_catch)(GObj *),
                           void (*proc_capture)(GObj *, GObj *));
sb32 ftCommonLightThrowCheckItemTypeThrow(FTStruct *fp);
void ftCommonLightThrowDecideSetStatus(GObj *fighter_gobj);
void ftCommonCatchPullProcCatch(GObj *fighter_gobj);
void ftCommonCapturePulledProcCapture(GObj *fighter_gobj,
                                      GObj *capture_gobj);
void ftCommonThrowProcUpdate(GObj *fighter_gobj);
void ftCommonThrowSetStatus(GObj *fighter_gobj, sb32 is_throwf);
sb32 ftCommonThrowCheckInterruptCatchWait(GObj *fighter_gobj);
void ftCommonThrownProcUpdate(GObj *fighter_gobj);
void ftCommonThrownProcPhysics(GObj *fighter_gobj);
void ftCommonThrownProcMap(GObj *fighter_gobj);
void ftCommonThrownSetStatusQueue(GObj *fighter_gobj, s32 status_id_new,
                                  s32 status_id_queue);
void ftCommonThrownSetStatusImmediate(GObj *fighter_gobj, s32 status_id);
void ftCommonThrownReleaseThrownUpdateStats(GObj *fighter_gobj, s32 lr,
                                            s32 script_id,
                                            sb32 is_proc_status);
void ftParamStopVoiceRunProcDamage(GObj *fighter_gobj);
void ftSetupDropItem(FTStruct *fp);
void ftCommonThrownSetStatusDamageRelease(GObj *fighter_gobj);
void ftCommonThrownSetStatusNoDamageRelease(GObj *fighter_gobj);
void ftCommonThrownUpdateDamageStats(FTStruct *this_fp);
void ftCommonThrownReleaseFighterLoseGrip(GObj *fighter_gobj);
void ftCommonThrownDecideFighterLoseGrip(GObj *fighter_gobj,
                                         GObj *interact_gobj);
void ftCommonThrownDecideDeadResult(GObj *fighter_gobj);
void ftParamSetHitStatusAll(GObj *fighter_gobj, s32 hitstatus);
void ftCommonAttackDashSetStatus(GObj *fighter_gobj);
sb32 ftCommonAttackDashCheckInterruptCommon(GObj *fighter_gobj);
sb32 ftCommonGuardOnCheckInterruptDashRun(GObj *fighter_gobj, f32 frame);
sb32 ftCommonKneeBendCheckInterruptRun(GObj *fighter_gobj);
void ftCommonTurnProcUpdate(GObj *fighter_gobj);
void ftCommonTurnProcInterrupt(GObj *fighter_gobj);
void ftCommonTurnSetStatus(GObj *fighter_gobj, s32 lr_dash);
void ftCommonTurnSetStatusCenter(GObj *fighter_gobj);
sb32 ftCommonTurnCheckInputSuccess(GObj *fighter_gobj);
sb32 ftCommonAttackHi4CheckInterruptKneeBend(GObj *fighter_gobj);
sb32 ftCommonHammerKneeBendCheckInterruptCommon(GObj *fighter_gobj);
void ftCommonJumpProcInterrupt(GObj *fighter_gobj);
void ftCommonJumpGetJumpForceButton(s32 stick_range_x, s32 *jump_vel_x,
                                    s32 *jump_vel_y, sb32 is_shorthop);
void ftCommonJumpSetStatus(GObj *fighter_gobj);
void ftCommonFallProcInterrupt(GObj *fighter_gobj);
void ftCommonFallSetStatus(GObj *fighter_gobj);
void ftCommonHammerFallProcInterrupt(GObj *fighter_gobj);
void ftCommonHammerFallSetStatus(GObj *fighter_gobj);
void ftCommonOttottoProcUpdate(GObj *fighter_gobj);
void ftCommonOttottoProcInterrupt(GObj *fighter_gobj);
void ftCommonOttottoProcMap(GObj *fighter_gobj);
void ftCommonOttottoWaitSetStatus(GObj *fighter_gobj);
void ftCommonOttottoSetStatus(GObj *fighter_gobj);
void ftCommonStopCeilSetStatus(GObj *fighter_gobj);
void ftCommonCliffCatchProcUpdate(GObj *fighter_gobj);
void ftCommonCliffCommonProcPhysics(GObj *fighter_gobj);
void ftCommonCliffCommonProcMap(GObj *fighter_gobj);
void ftCommonCliffCatchSetStatus(GObj *fighter_gobj);
void ftCommonCliffCommonProcDamage(GObj *fighter_gobj);
void ftCommonCliffWaitProcInterrupt(GObj *fighter_gobj);
void ftCommonCliffWaitSetStatus(GObj *fighter_gobj);
sb32 ftCommonCliffWaitCheckFall(GObj *fighter_gobj);
void ftCommonCliffQuickProcUpdate(GObj *fighter_gobj);
void ftCommonCliffSlowProcUpdate(GObj *fighter_gobj);
void ftCommonCliffQuickOrSlowSetStatus(GObj *fighter_gobj,
                                       s32 status_input);
sb32 ftCommonCliffAttackCheckInterruptCommon(GObj *fighter_gobj);
sb32 ftCommonCliffEscapeCheckInterruptCommon(GObj *fighter_gobj);
sb32 ftCommonCliffClimbOrFallCheckInterruptCommon(GObj *fighter_gobj);
void ftCommonCliffClimbQuick1ProcUpdate(GObj *fighter_gobj);
void ftCommonCliffClimbSlow1ProcUpdate(GObj *fighter_gobj);
void ftCommonCliffClimbQuick1SetStatus(GObj *fighter_gobj);
void ftCommonCliffClimbSlow1SetStatus(GObj *fighter_gobj);
void ftCommonCliffCommon2ProcUpdate(GObj *fighter_gobj);
void ftCommonCliffCommon2ProcPhysics(GObj *fighter_gobj);
void ftCommonCliffClimbCommon2ProcMap(GObj *fighter_gobj);
void ftCommonCliffAttackEscape2ProcMap(GObj *fighter_gobj);
void ftCommonCliffCommon2UpdateCollData(GObj *fighter_gobj);
void ftCommonCliffCommon2InitStatusVars(GObj *fighter_gobj);
void ftCommonCliffClimbQuick2SetStatus(GObj *fighter_gobj);
void ftCommonCliffClimbSlow2SetStatus(GObj *fighter_gobj);
void ftCommonCliffAttackQuick1ProcUpdate(GObj *fighter_gobj);
void ftCommonCliffAttackSlow1ProcUpdate(GObj *fighter_gobj);
void ftCommonCliffAttackQuick1SetStatus(GObj *fighter_gobj);
void ftCommonCliffAttackSlow1SetStatus(GObj *fighter_gobj);
void ftCommonCliffAttackQuick2SetStatus(GObj *fighter_gobj);
void ftCommonCliffAttackSlow2SetStatus(GObj *fighter_gobj);
void ftCommonCliffEscapeQuick1ProcUpdate(GObj *fighter_gobj);
void ftCommonCliffEscapeSlow1ProcUpdate(GObj *fighter_gobj);
void ftCommonCliffEscapeQuick1SetStatus(GObj *fighter_gobj);
void ftCommonCliffEscapeSlow1SetStatus(GObj *fighter_gobj);
void ftCommonCliffEscapeQuick2SetStatus(GObj *fighter_gobj);
void ftCommonCliffEscapeSlow2SetStatus(GObj *fighter_gobj);
void ftCommonDamageFallProcInterrupt(GObj *fighter_gobj);
void ftCommonDamageFallProcMap(GObj *fighter_gobj);
void ftCommonDamageFallClampRumble(GObj *fighter_gobj);
void ftCommonDamageFallSetStatusFromDamage(GObj *fighter_gobj);
void ftCommonDamageFallSetStatusFromCliffWait(GObj *fighter_gobj);
void ftCommonDamageFlyRollUpdateModelPitch(GObj *fighter_gobj);
void ftCommonDamageCheckSetInvincible(GObj *fighter_gobj);
void ftCommonDamageSetStatus(GObj *fighter_gobj);
void ftCommonDamageCommonProcLagUpdate(GObj *fighter_gobj);
sb32 ftCommonHammerFallCheckInterruptDamageFall(GObj *fighter_gobj);
void ftCommonPassiveStandSetStatus(GObj *fighter_gobj, s32 status_id);
sb32 ftCommonPassiveStandCheckInterruptDamage(GObj *fighter_gobj);
void ftCommonPassiveSetStatus(GObj *fighter_gobj);
sb32 ftCommonPassiveCheckInterruptDamage(GObj *fighter_gobj);
void ftCommonReboundProcUpdate(GObj *fighter_gobj);
void ftCommonReboundSetStatus(GObj *fighter_gobj);
void ftCommonReboundWaitProcUpdate(GObj *fighter_gobj);
void ftCommonReboundWaitSetStatus(GObj *fighter_gobj);
void ftCommonDownWaitProcUpdate(GObj *fighter_gobj);
void ftCommonDownWaitProcInterrupt(GObj *fighter_gobj);
void ftCommonDownWaitSetStatus(GObj *fighter_gobj);
void ftCommonDownBounceProcUpdate(GObj *fighter_gobj);
sb32 ftCommonDownBounceCheckUpOrDown(GObj *fighter_gobj);
void ftCommonDownBounceUpdateEffects(GObj *fighter_gobj);
void ftCommonDownBounceSetStatus(GObj *fighter_gobj);
void ftCommonDownStandProcInterrupt(GObj *fighter_gobj);
void ftCommonDownStandSetStatus(GObj *fighter_gobj);
void ftCommonDownAttackSetStatus(GObj *fighter_gobj, s32 status_id);
sb32 ftCommonDownAttackCheckInterruptDownWait(GObj *fighter_gobj);
void ftCommonDownForwardOrBackSetStatus(GObj *fighter_gobj, s32 status_id);
sb32 ftCommonDownForwardOrBackCheckInterruptCommon(GObj *fighter_gobj);
sb32 ftCommonDownStandCheckInterruptCommon(GObj *fighter_gobj);
sb32 ftCommonDownAttackCheckInterruptDownBounce(GObj *fighter_gobj);
void ftCommonLandingProcInterrupt(GObj *fighter_gobj);
void ftCommonLandingSetStatusParam(GObj *fighter_gobj, s32 status_id,
                                   sb32 is_allow_interrupt, f32 anim_speed);
void ftCommonLandingSetStatus(GObj *fighter_gobj);
void ftCommonLandingAirNullSetStatus(GObj *fighter_gobj, f32 anim_speed);
void ftCommonLandingAirSetStatus(GObj *fighter_gobj);
void ftCommonLandingFallSpecialSetStatus(GObj *fighter_gobj,
                                         sb32 is_allow_interrupt,
                                         f32 anim_speed);
sb32 ftCommonSpecialAirCheckInterruptCommon(GObj *fighter_gobj);
sb32 ftCommonAttackAirCheckInterruptCommon(GObj *fighter_gobj);
void ftCommonAttackAirProcMap(GObj *fighter_gobj);
sb32 ftCommonJumpAerialCheckInterruptCommon(GObj *fighter_gobj);
sb32 ftCommonTurnRunCheckInterruptRun(GObj *fighter_gobj);
void ftCommonTurnSetStatusInvertLR(GObj *fighter_gobj);
void ftParamSetStickLR(FTStruct *fp);
sb32 ftCommonSquatCheckInterruptCommon(GObj *fighter_gobj);
sb32 ftCommonPassCheckInterruptCommon(GObj *fighter_gobj);
sb32 ftCommonPassCheckInterruptSquat(GObj *fighter_gobj);
sb32 ftCommonDokanStartCheckInterruptCommon(GObj *fighter_gobj);
sb32 ftCommonSquatWaitCheckInterruptLanding(GObj *fighter_gobj);
sb32 ftCommonHammerFallCheckInterruptCommon(GObj *fighter_gobj);
void ndsBaseFTCommonPassSetStatusParam(GObj *fighter_gobj, s32 status_id,
                                       f32 frame_begin, u32 flags);
sb32 ndsBaseFTCommonPassCheckInterruptCommon(GObj *fighter_gobj);
sb32 ndsBaseFTCommonPassCheckInterruptSquat(GObj *fighter_gobj);
sb32 ndsBaseFTCommonSquatCheckInterruptCommon(GObj *fighter_gobj);
void ndsBaseFTCommonSquatSetStatusPass(GObj *fighter_gobj);
sb32 ndsBaseFTCommonSquatCheckGotoPass(GObj *fighter_gobj);
void ndsBaseFTCommonAttackDashSetStatus(GObj *fighter_gobj);
sb32 ndsBaseFTCommonAttackDashCheckInterruptCommon(GObj *fighter_gobj);
void ndsBaseFTCommonAttack11ProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonAttack11ProcInterrupt(GObj *fighter_gobj);
void ndsBaseFTCommonAttack11ProcStatus(GObj *fighter_gobj);
void ndsBaseFTCommonAttack11SetStatus(GObj *fighter_gobj);
void ndsBaseFTCommonAttack12ProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonAttack12ProcInterrupt(GObj *fighter_gobj);
void ndsBaseFTCommonAttack12SetStatus(GObj *fighter_gobj);
sb32 ndsBaseFTCommonAttack1CheckInterruptCommon(GObj *fighter_gobj);
sb32 ndsBaseFTCommonAttack11CheckGoto(GObj *fighter_gobj);
sb32 ndsBaseFTCommonAttack12CheckGoto(GObj *fighter_gobj);
void ndsBaseFTCommonCatchProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonCatchCaptureSetStatusRelease(GObj *fighter_gobj);
void ndsBaseFTCommonCatchProcMap(GObj *fighter_gobj);
void ndsBaseFTCommonCatchSetStatus(GObj *fighter_gobj);
sb32 ndsBaseFTCommonCatchCheckInterruptGuard(GObj *fighter_gobj);
sb32 ndsBaseFTCommonCatchCheckInterruptCommon(GObj *fighter_gobj);
sb32 ndsBaseFTCommonCatchCheckInterruptDashRun(GObj *fighter_gobj);
sb32 ndsBaseFTCommonCatchCheckInterruptAttack11(GObj *fighter_gobj);
void ftCommonItemThrowSetStatus(GObj *fighter_gobj, s32 status_id);
void ftCommonItemSwingSetStatus(GObj *fighter_gobj, s32 swing_type);
void ftCommonItemShootSetStatus(GObj *fighter_gobj);
void ftCommonItemShootAirSetStatus(GObj *fighter_gobj);
sb32 ftCommonGetCheckInterruptCommon(GObj *fighter_gobj);
sb32 ftCommonAttack100StartCheckInterruptCommon(GObj *fighter_gobj);
void ftCommonAttack100StartSetStatus(GObj *fighter_gobj);
void ftCommonAttack100StartProcUpdate(GObj *fighter_gobj);
void ftCommonAttack100LoopProcUpdate(GObj *fighter_gobj);
void ftCommonAttack100LoopProcInterrupt(GObj *fighter_gobj);
void ftCommonAttack100LoopSetStatus(GObj *fighter_gobj);
void ftCommonAttack100EndSetStatus(GObj *fighter_gobj);
sb32 ndsBaseFTCommonAttack100StartCheckInterruptCommon(GObj *fighter_gobj);
void ndsBaseFTCommonAttack100StartSetStatus(GObj *fighter_gobj);
void ndsBaseFTCommonAttack100StartProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonAttack100LoopProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonAttack100LoopProcInterrupt(GObj *fighter_gobj);
void ndsBaseFTCommonAttack100LoopSetStatus(GObj *fighter_gobj);
void ndsBaseFTCommonAttack100EndSetStatus(GObj *fighter_gobj);
extern void *gFTDataKirbyMainMotion;
extern uintptr_t llKirbyMainMotionftKirbyAttack100Effect;
void gmCollisionGetFighterPartsWorldPosition(DObj *main_dobj, Vec3f *vec);
GObj *efManagerKirbyVulcanJabMakeEffect(Vec3f *pos, s32 lr, f32 rotate,
                                        f32 vel, f32 add);
sb32 ftCommonCatchCheckInterruptAttack11(GObj *fighter_gobj);
void ftParamSetMotionID(FTStruct *fp, s32 motion_attack_id);
void ftParamSetStatUpdate(FTStruct *fp, u16 flags);
void ftParamUpdate1PGameAttackStats(FTStruct *fp, s32 attack_id);
void ftParamClearAttackRecordID(FTStruct *fp, s32 attack_id);
void ftParamRefreshAttackCollID(GObj *fighter_gobj, s32 attack_id);
sb32 ftCommonTurnCheckInterruptCommon(GObj *fighter_gobj);
void ftAnimEndSetWait(GObj *fighter_gobj);
void ftAnimEndSetFall(GObj *fighter_gobj);
void gmCollisionGetWorldPosition(Mtx44f mtx, Vec3f *vec);
void func_ovl2_800EDA0C(Mtx44f mtx, Vec3f *vec);
sb32 ftAnimEndCheckSetStatus(GObj *fighter_gobj, void (*proc_status)(GObj*));
void ftMainPlayAnimEventsAll(GObj *fighter_gobj);
void ftMainSetStatus(GObj *fighter_gobj, s32 status_id,
                     f32 frame_begin, f32 anim_speed, u32 flags);
void ftPhysicsSetGroundVelAbsStickRange(FTStruct *fp, f32 vel, f32 friction);
void ftPhysicsSetGroundVelFriction(FTStruct *fp, f32 friction);
void ftPhysicsSetGroundVelTransferAir(GObj *fighter_gobj);
void ftPhysicsApplyAirVelDrift(GObj *fighter_gobj);
f32 ftParamGetStickAngleRads(FTStruct *fp);
void ftParamSetCaptureImmuneMask(FTStruct *fp, u8 capture_immune_mask);
void ftParamSetThrowParams(FTStruct *fp, GObj *throw_gobj);
void ftParamSetModelPartID(GObj *fighter_gobj, s32 joint_id,
                           s32 modelpart_id);
void ftCommonDamageInitDamageVars(GObj *fighter_gobj, s32 status_id_replace,
                                  s32 damage, f32 knockback,
                                  s32 angle_start, s32 damage_lr,
                                  s32 damage_index, s32 element,
                                  s32 damage_player_num, s32 arg9,
                                  sb32 unk_bool, sb32 is_public);
void ftCommonDamageGotoDamageStatus(GObj *fighter_gobj);
void ftCommonFuraSleepProcUpdate(GObj *fighter_gobj);
void ftCommonFuraSleepSetStatus(GObj *fighter_gobj);
void ndsBaseFTCommonFuraSleepProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonFuraSleepSetStatus(GObj *fighter_gobj);
void ftCommonTwisterProcUpdate(GObj *fighter_gobj);
void ftCommonTwisterProcPhysics(GObj *fighter_gobj);
void ftCommonTwisterSetStatus(GObj *fighter_gobj, GObj *tornado_gobj);
void ftCommonTaruCannProcPhysics(GObj *fighter_gobj);
void ftCommonTaruCannSetStatus(GObj *fighter_gobj, GObj *tarucann_gobj);
void ndsBaseFTCommonTwisterProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonTwisterProcPhysics(GObj *fighter_gobj);
void ndsBaseFTCommonTwisterSetStatus(GObj *fighter_gobj,
                                     GObj *tornado_gobj);
void ftCommonCaptureTrappedInitBreakoutVars(FTStruct *fp,
                                            s32 breakout_wait);
sb32 ftCommonCaptureTrappedUpdateBreakoutVars(FTStruct *fp);
f32 ftParamGetCommonKnockback(s32 percent_damage, s32 recent_damage,
                              s32 hit_damage, s32 knockback_weight,
                              s32 knockback_scale, s32 knockback_base,
                              f32 weight, s32 attack_handicap,
                              s32 defend_handicap);
s32 ftParamGetHitLag(s32 damage, s32 status_id, f32 hitlag_mul);
f32 ftParamGetHitStun(f32 knockback);
void ftParamSetDamageShuffle(FTStruct *fp, sb32 is_electric, s32 damage,
                             s32 status_id, f32 hitlag_mul);
s32 ftParamGetBestHitStatusAll(GObj *fighter_gobj);
s32 ftParamGetStaledDamage(s32 player, s32 damage, s32 attack_id,
                           u16 motion_count);
void ftParamUpdateDamage(FTStruct *fp, s32 damage);
void ftParamUpdate1PGameDamageStats(FTStruct *fp, s32 damage_player,
                                    s32 damage_object_class,
                                    s32 damage_object_kind, u16 flags,
                                    u16 damage_stat_count);
void ftParamSetTimedHitStatusInvincible(FTStruct *fp, s32 invincible_tics);
void ftParamUpdatePlayerBattleStats(s32 attack_player, s32 defend_player,
                                    s32 attack_damage);
void ftParamUpdateStaleQueue(s32 attack_player, s32 defend_player,
                             s32 attack_id, u16 motion_count);
void ftParamVelDamageTransferGround(FTStruct *fp);
void *ftParamMakeEffect(GObj *fighter_gobj, s32 effect_id, s32 joint_id,
                        Vec3f *effect_pos, Vec3f *effect_scatter, s32 lr,
                        sb32 is_scale_pos, u32 arg7);
void mpCommonSetFighterGround(FTStruct *fp);
void mpCommonSetFighterAir(FTStruct *fp);
void mpCommonSetFighterWaitOrFall(GObj *fighter_gobj);
void mpCommonSetFighterLandingParams(GObj *fighter_gobj);
void mpCommonRunFighterCollisionDefault(GObj *fighter_gobj, Vec3f *pos,
                                        FTCollisionData *coll_data);
sb32 mpCommonCheckFighterCliff(GObj *fighter_gobj);
sb32 mpCommonCheckFighterOnFloor(GObj *fighter_gobj);
sb32 mpCommonCheckFighterLanding(GObj *fighter_gobj);
void mpCommonProcFighterOnCliffEdge(GObj *fighter_gobj);
void mpCommonProcFighterCliffFloorCeil(GObj *fighter_gobj);
void mpCommonSetFighterFallOnGroundBreak(GObj *fighter_gobj);
void mpCommonSetFighterFallOnEdgeBreak(GObj *fighter_gobj);
void ftPhysicsApplyGravityClampTVel(FTStruct *fp, f32 gravity, f32 tvel);
void ftPhysicsApplyGravityDefault(FTStruct *fp, FTAttributes *attr);
void ftPhysicsApplyFastFall(FTStruct *fp, FTAttributes *attr);
void ftPhysicsApplyGroundVelTransN(GObj *fighter_gobj);
void ftPhysicsGetAirVelTransN(FTStruct *fp, f32 *vel_x, f32 *vel_y,
                              f32 *vel_z);
void ftPhysicsApplyAirVelTransNAll(GObj *fighter_gobj);
void ftPhysicsClampAirVelX(FTStruct *fp, f32 clamp);
void ftPhysicsClampAirVelXMax(FTStruct *fp);
sb32 ftPhysicsCheckClampAirVelXDecMax(FTStruct *fp, FTAttributes *attr);
void ftPhysicsClampAirVelXStickRange(FTStruct *fp, s32 stick_range_min,
                                     f32 vel, f32 clamp);
void ftPhysicsClampAirVelXStickDefault(FTStruct *fp, FTAttributes *attr);
void ftPhysicsApplyAirVelXFriction(FTStruct *fp, FTAttributes *attr);
void ftPhysicsCheckSetFastFall(FTStruct *fp);
void ftPhysicsApplyAirVelFriction(GObj *fighter_gobj);
void ftPhysicsApplyAirVelDriftFastFall(GObj *fighter_gobj);
void ftParamMakeRumble(FTStruct *fp, s32 rumble_id, s32 length);
extern u16 dFTCommonDataDownBounceSFX[];
f32 scSubsysFighterGetLightAngleX(void);
f32 scSubsysFighterGetLightAngleY(void);
void scSubsysFighterSetStatus(GObj *fighter_gobj, s32 status_id);

#ifndef ABS
#define ABS(x) (((x) < 0) ? -(x) : (x))
#endif

#ifndef F_CST_DTOR32
#define F_CST_DTOR32(x) ((f32)((x) * 0.01745329251994329577))
#endif

#ifndef F_CLC_RTOD32
#define F_CLC_RTOD32(x) ((f32)(((x) / 3.14159265358979323846F) * 180.0F))
#endif

enum {
    nGMColAnimCommonNull = 0,
    nGMColAnimFighterComPlayer = 1,
    nGMColAnimFighterDamageCommon = 5,
    nGMColAnimFighterFastFall = 8,
    nGMColAnimFighterDamageFireStart = 12,
    nGMColAnimFighterDamageIceStart = 32,
    nGMColAnimFighterShieldBreakFly = 36,
    nGMColAnimFighterFuraFura = 37,
    nGMColAnimFighterFuraSleep = 38,
    nGMColAnimScreenFlashDamageNormal = 58,
    nGMColAnimScreenFlashDamageFire = 59,
    nGMColAnimScreenFlashDamageElectric = 60,
    nGMColAnimScreenFlashDamageIce = 61
};

#endif
