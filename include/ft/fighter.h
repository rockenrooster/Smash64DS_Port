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
#include <macros.h>
#include <ssb_types.h>
#include <mp/map.h>
#include <sys/controller.h>
#include <sys/objdef.h>
#include <sys/objtypes.h>

#ifndef U8_MAX
#define U8_MAX 255
#endif

#ifndef GMCOMMON_PLAYERS_MAX
#define GMCOMMON_PLAYERS_MAX 4
#endif

#define GMHITCOLLISION_FLAG_FIGHTER (1u << 0)
#define GMHITCOLLISION_FLAG_WEAPON (1u << 1)
#define GMHITCOLLISION_FLAG_ITEM (1u << 2)
#define GMHITCOLLISION_FLAG_ALL \
    (GMHITCOLLISION_FLAG_FIGHTER | GMHITCOLLISION_FLAG_WEAPON | \
     GMHITCOLLISION_FLAG_ITEM)

typedef struct GObj GObj;
typedef struct FTData FTData;
typedef struct alSoundEffect alSoundEffect;

#include <ft/ftdata_file_slots.h>

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
    nFTDemoStatusNull = 0x10000,
    nFTDemoStatusWin1,
    nFTDemoStatusWin2,
    nFTDemoStatusWin3,
    nFTDemoStatusWin4,
    nFTDemoStatusLose,
    nFTDemoStatusRun,
    nFTDemoStatusJump,
    nFTDemoStatusFigurePulled,
    nFTDemoStatusFigureDropped,
    nFTDemoStatusFigureStand,
    nFTDemoStatusClash,
    nFTDemoStatusStance,
    nFTDemoStatusIntroL,
    nFTDemoStatusIntroR,
    nFTDemoStatusSpecialStart
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
    nFTPartsJointNumMax = 37
};

#define FTPARTS_JOINT_NUM_MAX nFTPartsJointNumMax

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
#define FTKIRBY_COPY_MODELPARTS_JOINT 6

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
typedef struct FTAccessPart FTAccessPart;

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
    u16 button_hold;
    u16 button_tap;
    u16 button_release;
    Vec2b stick_range;
    Vec2b stick_prev;
} FTInputPlayer;

typedef struct FTInputCPU {
    u16 button_inputs;
    Vec2b stick_range;
} FTInputCPU;

typedef FTInputPlayer FTPlayerInput;
typedef FTInputCPU FTComputerInput;

typedef struct FTInput {
    SYController *controller;
    u16 button_mask_a;
    u16 button_mask_b;
    u16 button_mask_z;
    u16 button_mask_l;
    FTInputPlayer pl;
    FTInputCPU cp;
} FTInput;

typedef MPCollData FTCollisionData;

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
    u32 fgm_level : 3;
    u32 fgm_kind : 4;
    ub32 is_hit_air : 1;
    ub32 is_hit_ground : 1;
    ub32 can_rebound : 1;
    ub32 is_scale_pos : 1;
    u32 motion_attack_id : 6;
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

typedef enum GMHitType {
    nGMHitTypeDamage,
    nGMHitTypeShield,
    nGMHitTypeShieldRehit,
    nGMHitTypeAttack,
    nGMHitTypeDamageRehit,
    nGMHitTypeAbsorb,
    nGMHitTypeReflect
} GMHitType;

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

typedef struct FTItemPickup {
    Vec2f pickup_offset_light;
    Vec2f pickup_range_light;
    Vec2f pickup_offset_heavy;
    Vec2f pickup_range_heavy;
} FTItemPickup;

typedef struct FTKirbyCopy {
    u16 copy_id;
    s16 copy_modelpart_id;
    f32 effect_scale;
    s32 star_damage;
} FTKirbyCopy;

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

#define gmColEventAdvance(event, type) \
    ((event) = (void *)((uintptr_t)(event) + sizeof(type)))
#define gmColEventCast(event, type) ((type *)(event))
#define gmColEventCastAdvance(event, type) ((type *)(event)++)

typedef enum GMColEvent {
    nGMColEventEnd,
    nGMColEventWait,
    nGMColEventGoto,
    nGMColEventLoopBegin,
    nGMColEventLoopEnd,
    nGMColEventSubroutine,
    nGMColEventReturn,
    nGMColEventSetParallelScript,
    nGMColEventClearColorAll,
    nGMColEventSetColor1,
    nGMColEventBlendColor1,
    nGMColEventSetColor2,
    nGMColEventBlendColor2,
    nGMColEventEffect,
    nGMColEventEffectItemHoldOffset,
    nGMColEventSetLight,
    nGMColEventClearLight,
    nGMColEventPlayFGM,
    nGMColEventSetSkeletonID
} GMColEvent;

typedef struct GMColScript {
    u32 *p_script;
    u16 color_event_timer;
    u16 script_id;
    void *p_subroutine[1];
    s32 loop_count[1];
    void *p_goto[2];
    s32 unk_ca_timer;
} GMColScript;

typedef struct GMColKeys {
    u8 r, g, b, a;
    s16 ir, ig, ib, ia;
} GMColKeys;

typedef struct GMColAnim {
    GMColScript cs[2];
    s32 length;
    s32 colanim_id;
    GMColKeys color1;
    f32 light_angle_x;
    f32 light_angle_y;
    GMColKeys color2;
    ub8 is_use_color1 : 1;
    ub8 is_use_light : 1;
    ub8 is_use_color2 : 1;
    u8 skeleton_id : 2;
} GMColAnim;

typedef struct GMColDesc {
    void *p_script;
    u8 priority;
    ub8 is_unlocked;
} GMColDesc;

typedef struct GMColEventDefault {
    u32 opcode : 6;
    u32 value : 26;
} GMColEventDefault;

typedef struct GMColEventGoto1 {
    u32 opcode : 6;
} GMColEventGoto1;

typedef struct GMColEventGoto2 {
    void *p_goto;
} GMColEventGoto2;

typedef struct GMColEventSubroutine1 {
    u32 opcode : 6;
} GMColEventSubroutine1;

typedef struct GMColEventSubroutine2 {
    void *p_subroutine;
} GMColEventSubroutine2;

typedef struct GMColEventParallel1 {
    u32 opcode : 6;
} GMColEventParallel1;

typedef struct GMColEventParallel2 {
    void *p_script;
} GMColEventParallel2;

typedef struct GMColEventSetRGBA1 {
    u32 opcode : 6;
} GMColEventSetRGBA1;

typedef struct GMColEventSetRGBA2 {
    u32 r : 8;
    u32 g : 8;
    u32 b : 8;
    u32 a : 8;
} GMColEventSetRGBA2;

typedef struct GMColEventBlendRGBA1 {
    u32 opcode : 6;
    u32 blend_frames : 26;
} GMColEventBlendRGBA1;

typedef struct GMColEventBlendRGBA2 {
    u32 r : 8;
    u32 g : 8;
    u32 b : 8;
    u32 a : 8;
} GMColEventBlendRGBA2;

typedef struct GMColEventMakeEffect1 {
    u32 opcode : 6;
    s32 joint_id : 7;
    u32 effect_id : 9;
    u32 flag : 10;
} GMColEventMakeEffect1;

typedef struct GMColEventMakeEffect2 {
    s32 off_x : 16;
    s32 off_y : 16;
} GMColEventMakeEffect2;

typedef struct GMColEventMakeEffect3 {
    s32 off_z : 16;
    s32 rng_x : 16;
} GMColEventMakeEffect3;

typedef struct GMColEventMakeEffect4 {
    s32 rng_y : 16;
    s32 rng_z : 16;
} GMColEventMakeEffect4;

typedef struct GMColEventMakeEffect {
    GMColEventMakeEffect1 s1;
    GMColEventMakeEffect2 s2;
    GMColEventMakeEffect3 s3;
    GMColEventMakeEffect4 s4;
} GMColEventMakeEffect;

typedef struct GMColEventSetLight {
    u32 opcode : 6;
    s32 light1 : 13;
    s32 light2 : 13;
} GMColEventSetLight;

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
    nFTCommonMotionFallSpecial = 51,
    nFTCommonMotionLandingFallSpecial = 52,
    nFTCommonMotionDokanStart = 54,
    nFTCommonMotionDokanEnd = 55,
    nFTCommonMotionDokanWalk = 56,
    nFTCommonMotionLightGet = 88,
    nFTCommonMotionHeavyGet,
    nFTCommonMotionLightThrowDrop,
    nFTCommonMotionLightThrowDash,
    nFTCommonMotionLightThrowF,
    nFTCommonMotionLightThrowB,
    nFTCommonMotionLightThrowHi,
    nFTCommonMotionLightThrowLw,
    nFTCommonMotionLightThrowF4,
    nFTCommonMotionLightThrowB4,
    nFTCommonMotionLightThrowHi4,
    nFTCommonMotionLightThrowLw4,
    nFTCommonMotionLightThrowAirF,
    nFTCommonMotionLightThrowAirB,
    nFTCommonMotionLightThrowAirHi,
    nFTCommonMotionLightThrowAirLw,
    nFTCommonMotionLightThrowAirF4,
    nFTCommonMotionLightThrowAirB4,
    nFTCommonMotionLightThrowAirHi4,
    nFTCommonMotionLightThrowAirLw4,
    nFTCommonMotionHeavyThrowF,
    nFTCommonMotionHeavyThrowB,
    nFTCommonMotionHeavyThrowF4,
    nFTCommonMotionHeavyThrowB4,
    nFTCommonMotionSwordSwing1,
    nFTCommonMotionSwordSwing3,
    nFTCommonMotionSwordSwing4,
    nFTCommonMotionSwordSwingDash,
    nFTCommonMotionBatSwing1,
    nFTCommonMotionBatSwing3,
    nFTCommonMotionBatSwing4,
    nFTCommonMotionBatSwingDash,
    nFTCommonMotionHarisenSwing1,
    nFTCommonMotionHarisenSwing3,
    nFTCommonMotionHarisenSwing4,
    nFTCommonMotionHarisenSwingDash,
    nFTCommonMotionStarRodSwing1,
    nFTCommonMotionStarRodSwing3,
    nFTCommonMotionStarRodSwing4,
    nFTCommonMotionStarRodSwingDash,
    nFTCommonMotionLGunShoot,
    nFTCommonMotionLGunShootAir,
    nFTCommonMotionFireFlowerShoot,
    nFTCommonMotionFireFlowerShootAir,
    nFTCommonMotionHammerWait,
    nFTCommonMotionHammerWalk,
    nFTCommonMotionShieldBreakFall = 139,
    nFTCommonMotionShieldBreakDownD,
    nFTCommonMotionShieldBreakDownU,
    nFTCommonMotionShieldBreakStandD,
    nFTCommonMotionShieldBreakStandU,
    nFTCommonMotionThrownKirbyStar = 151,
    nFTCommonMotionThrownCopyStar,
    nFTCommonMotionYoshiEgg,
    nFTCommonMotionCaptureCaptain,
    nFTCommonMotionThrownDonkeyUnk,
    nFTCommonMotionThrownDonkeyF,
    nFTCommonMotionThrownMarioB1,
    nFTCommonMotionThrownUnk1,
    nFTCommonMotionShouldered,
    nFTCommonMotionThrownMarioB2,
    nFTCommonMotionThrownUnk2 = 162,
    nFTCommonMotionThrownUnk3,
    nFTCommonMotionAttackS3Hi = 168,
    nFTCommonMotionAttackS3HiS,
    nFTCommonMotionAttackS3,
    nFTCommonMotionAttackS3LwS,
    nFTCommonMotionAttackS3Lw,
    nFTCommonMotionAttackHi3F,
    nFTCommonMotionAttackHi3,
    nFTCommonMotionAttackHi3B,
    nFTCommonMotionAttackLw3,
    nFTCommonMotionAttackS4Hi,
    nFTCommonMotionAttackS4HiS,
    nFTCommonMotionAttackS4,
    nFTCommonMotionAttackS4LwS,
    nFTCommonMotionAttackS4Lw,
    nFTCommonMotionAttackHi4,
    nFTCommonMotionAttackLw4
};

enum {
    nFTCommonStatusDeadDown,
    nFTCommonStatusDeadLeftRight,
    nFTCommonStatusDeadUpStar,
    nFTCommonStatusDeadUpFall,
    nFTCommonStatusSleep,
    nFTCommonStatusEntry,
    nFTCommonStatusActionStart,
    nFTCommonStatusEntryNull = nFTCommonStatusActionStart,
    nFTCommonStatusRebirthDown,
    nFTCommonStatusRebirthStand,
    nFTCommonStatusRebirthWait,
    nFTCommonStatusControlStart,
    nFTCommonStatusWait = nFTCommonStatusControlStart,
    nFTCommonStatusWalkSlow,
    nFTCommonStatusWalkMiddle,
    nFTCommonStatusWalkFast,
    nFTCommonStatusWalkEnd,
    nFTCommonStatusDash,
    nFTCommonStatusRun,
    nFTCommonStatusRunBrake,
    nFTCommonStatusTurn,
    nFTCommonStatusTurnRun,
    nFTCommonStatusKneeBend,
    nFTCommonStatusGuardKneeBend,
    nFTCommonStatusJumpF,
    nFTCommonStatusJumpB,
    nFTCommonStatusJumpAerialF,
    nFTCommonStatusJumpAerialB,
    nFTCommonStatusFall,
    nFTCommonStatusFallAerial,
    nFTCommonStatusSquat,
    nFTCommonStatusSquatWait,
    nFTCommonStatusSquatRv,
    nFTCommonStatusLandingLight,
    nFTCommonStatusLandingHeavy,
    nFTCommonStatusPass,
    nFTCommonStatusGuardPass,
    nFTCommonStatusOttottoWait,
    nFTCommonStatusOttotto,
    nFTCommonStatusDamageStart,
    nFTCommonStatusDamageHi1 = nFTCommonStatusDamageStart,
    nFTCommonStatusDamageHi2,
    nFTCommonStatusDamageHi3,
    nFTCommonStatusDamageN1,
    nFTCommonStatusDamageN2,
    nFTCommonStatusDamageN3,
    nFTCommonStatusDamageLw1,
    nFTCommonStatusDamageLw2,
    nFTCommonStatusDamageLw3,
    nFTCommonStatusDamageAir1,
    nFTCommonStatusDamageAir2,
    nFTCommonStatusDamageAir3,
    nFTCommonStatusDamageE1,
    nFTCommonStatusDamageE2,
    nFTCommonStatusDamageFlyHi,
    nFTCommonStatusDamageFlyN,
    nFTCommonStatusDamageFlyLw,
    nFTCommonStatusDamageFlyTop,
    nFTCommonStatusDamageFlyRoll,
    nFTCommonStatusWallDamage,
    nFTCommonStatusDamageEnd = nFTCommonStatusWallDamage,
    nFTCommonStatusDamageFall,
    nFTCommonStatusFallSpecial,
    nFTCommonStatusLandingFallSpecial,
    nFTCommonStatusTwister,
    nFTCommonStatusTaruCann,
    nFTCommonStatusDokanStart,
    nFTCommonStatusDokanWait,
    nFTCommonStatusDokanEnd,
    nFTCommonStatusDokanWalk,
    nFTCommonStatusStopCeil,
    nFTCommonStatusDownBounceD,
    nFTCommonStatusDownBounceU,
    nFTCommonStatusDownWaitD,
    nFTCommonStatusDownWaitU,
    nFTCommonStatusDownStandD,
    nFTCommonStatusDownStandU,
    nFTCommonStatusPassiveStandF,
    nFTCommonStatusPassiveStandB,
    nFTCommonStatusDownForwardD,
    nFTCommonStatusDownForwardU,
    nFTCommonStatusDownBackD,
    nFTCommonStatusDownBackU,
    nFTCommonStatusDownAttackD,
    nFTCommonStatusDownAttackU,
    nFTCommonStatusPassive,
    nFTCommonStatusReboundWait,
    nFTCommonStatusRebound,
    nFTCommonStatusCliffCatch,
    nFTCommonStatusCliffWait,
    nFTCommonStatusCliffQuick,
    nFTCommonStatusCliffClimbQuick1,
    nFTCommonStatusCliffClimbQuick2,
    nFTCommonStatusCliffSlow,
    nFTCommonStatusCliffClimbSlow1,
    nFTCommonStatusCliffClimbSlow2,
    nFTCommonStatusCliffAttackQuick1,
    nFTCommonStatusCliffAttackQuick2,
    nFTCommonStatusCliffAttackSlow1,
    nFTCommonStatusCliffAttackSlow2,
    nFTCommonStatusCliffEscapeQuick1,
    nFTCommonStatusCliffEscapeQuick2,
    nFTCommonStatusCliffEscapeSlow1,
    nFTCommonStatusCliffEscapeSlow2,
    nFTCommonStatusLightGet,
    nFTCommonStatusHeavyGet,
    nFTCommonStatusLiftWait,
    nFTCommonStatusLiftTurn,
    nFTCommonStatusLightThrowStart,
    nFTCommonStatusLightThrowDrop = nFTCommonStatusLightThrowStart,
    nFTCommonStatusLightThrowDash,
    nFTCommonStatusLightThrowF,
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
    nFTCommonStatusLightThrowEnd = nFTCommonStatusLightThrowAirLw4,
    nFTCommonStatusHeavyThrowStart,
    nFTCommonStatusHeavyThrowF = nFTCommonStatusHeavyThrowStart,
    nFTCommonStatusHeavyThrowB,
    nFTCommonStatusHeavyThrow4Start,
    nFTCommonStatusHeavyThrowF4 = nFTCommonStatusHeavyThrow4Start,
    nFTCommonStatusHeavyThrowB4,
    nFTCommonStatusHeavyThrow4End = nFTCommonStatusHeavyThrowB4,
    nFTCommonStatusHeavyThrowEnd = nFTCommonStatusHeavyThrow4End,
    nFTCommonStatusItemSwingStart,
    nFTCommonStatusSwordSwing1 = nFTCommonStatusItemSwingStart,
    nFTCommonStatusSwordSwing3,
    nFTCommonStatusSwordSwing4,
    nFTCommonStatusSwordSwingDash,
    nFTCommonStatusBatSwing1,
    nFTCommonStatusBatSwing3,
    nFTCommonStatusBatSwing4,
    nFTCommonStatusBatSwingDash,
    nFTCommonStatusHarisenSwing1,
    nFTCommonStatusHarisenSwing3,
    nFTCommonStatusHarisenSwing4,
    nFTCommonStatusHarisenSwingDash,
    nFTCommonStatusStarRodSwing1,
    nFTCommonStatusStarRodSwing3,
    nFTCommonStatusStarRodSwing4,
    nFTCommonStatusStarRodSwingDash,
    nFTCommonStatusItemSwingEnd = nFTCommonStatusStarRodSwingDash,
    nFTCommonStatusLGunShoot,
    nFTCommonStatusLGunShootAir,
    nFTCommonStatusFireFlowerShoot,
    nFTCommonStatusFireFlowerShootAir,
    nFTCommonStatusHammerStart,
    nFTCommonStatusHammerWait = nFTCommonStatusHammerStart,
    nFTCommonStatusHammerWalk,
    nFTCommonStatusHammerTurn,
    nFTCommonStatusHammerKneeBend,
    nFTCommonStatusHammerFall,
    nFTCommonStatusHammerLanding,
    nFTCommonStatusHammerEnd = nFTCommonStatusHammerLanding,
    nFTCommonStatusGuardStart,
    nFTCommonStatusGuardOn = nFTCommonStatusGuardStart,
    nFTCommonStatusGuard,
    nFTCommonStatusGuardOff,
    nFTCommonStatusGuardSetOff,
    nFTCommonStatusGuardEnd = nFTCommonStatusGuardSetOff,
    nFTCommonStatusEscapeF,
    nFTCommonStatusEscapeB,
    nFTCommonStatusShieldBreakFly,
    nFTCommonStatusShieldBreakFall,
    nFTCommonStatusShieldBreakDownD,
    nFTCommonStatusShieldBreakDownU,
    nFTCommonStatusShieldBreakStandD,
    nFTCommonStatusShieldBreakStandU,
    nFTCommonStatusFuraFura,
    nFTCommonStatusFuraSleep,
    nFTCommonStatusCatch,
    nFTCommonStatusCatchPull,
    nFTCommonStatusCatchWait,
    nFTCommonStatusThrowF,
    nFTCommonStatusThrowB,
    nFTCommonStatusCapturePulled,
    nFTCommonStatusCaptureWait,
    nFTCommonStatusCaptureKirby,
    nFTCommonStatusCaptureWaitKirby,
    nFTCommonStatusThrownKirbyStar,
    nFTCommonStatusThrownCopyStar,
    nFTCommonStatusCaptureYoshi,
    nFTCommonStatusYoshiEgg,
    nFTCommonStatusCaptureCaptain,
    nFTCommonStatusThrownDonkeyUnk,
    nFTCommonStatusThrownStart,
    nFTCommonStatusThrownDonkeyF = nFTCommonStatusThrownStart,
    nFTCommonStatusThrownMarioBStart,
    nFTCommonStatusThrownFoxFStart,
    nFTCommonStatusShouldered,
    nFTCommonStatusThrownMarioB,
    nFTCommonStatusThrownCommon,
    nFTCommonStatusThrownFoxF,
    nFTCommonStatusThrownFoxB,
    nFTCommonStatusThrownEnd = nFTCommonStatusThrownFoxB,
    nFTCommonStatusAppeal,
    nFTCommonStatusAttack11,
    nFTCommonStatusAttack12,
    nFTCommonStatusAttackDash,
    nFTCommonStatusAttackS3Hi,
    nFTCommonStatusAttackS3HiS,
    nFTCommonStatusAttackS3,
    nFTCommonStatusAttackS3LwS,
    nFTCommonStatusAttackS3Lw,
    nFTCommonStatusAttackHi3F,
    nFTCommonStatusAttackHi3,
    nFTCommonStatusAttackHi3B,
    nFTCommonStatusAttackLw3,
    nFTCommonStatusAttackS4Hi,
    nFTCommonStatusAttackS4HiS,
    nFTCommonStatusAttackS4,
    nFTCommonStatusAttackS4LwS,
    nFTCommonStatusAttackS4Lw,
    nFTCommonStatusAttackHi4,
    nFTCommonStatusAttackLw4,
    nFTCommonStatusAttackAirStart,
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
    nFTItemSwingKindSword = 0,
    nFTItemSwingKindBat,
    nFTItemSwingKindHarisen,
    nFTItemSwingKindStarRod,
    nFTItemSwingKindEnumCount
};

enum {
    nFTItemSwingTypeAttack1 = 0,
    nFTItemSwingTypeAttack3 = 1,
    nFTItemSwingTypeAttack4 = 2,
    nFTItemSwingTypeAttackDash = 3,
    nFTItemSwingTypeEnumCount
};

#define FTSTAT_CHARDATA_START 0x20000
#define FTSTAT_OPENING1_START 0xE0000
#define FTSTAT_OPENING2_START 0x10000

#define FTSTATUS_PRESERVE_NONE 0u
#define FTSTATUS_PRESERVE_HIT 0x1u
#define FTSTATUS_PRESERVE_COLANIM 0x2u
#define FTSTATUS_PRESERVE_EFFECT 0x4u
#define FTSTATUS_PRESERVE_FASTFALL 0x8u
#define FTSTATUS_PRESERVE_HITSTATUS 0x10u
#define FTSTATUS_PRESERVE_MODELPART 0x20u
#define FTSTATUS_PRESERVE_SLOPECONTOUR 0x40u
#define FTSTATUS_PRESERVE_TEXTUREPART 0x80u
#define FTSTATUS_PRESERVE_PLAYERTAG 0x100u
#define FTSTATUS_PRESERVE_THROWPOINTER 0x200u
#define FTSTATUS_PRESERVE_SHUFFLETIME 0x400u
#define FTSTATUS_PRESERVE_LOOPSFX 0x800u
#define FTSTATUS_PRESERVE_DAMAGEPLAYER 0x1000u
#define FTSTATUS_PRESERVE_AFTERIMAGE 0x2000u
#define FTSTATUS_PRESERVE_RUMBLE 0x4000u
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
    nFTMotionAttackIDNone,
    nFTMotionAttackIDBaseStart,
    nFTMotionAttackIDAttack11 = nFTMotionAttackIDBaseStart,
    nFTMotionAttackIDAttack12,
    nFTMotionAttackIDAttack13,
    nFTMotionAttackIDAttack100,
    nFTMotionAttackIDAttackDash,
    nFTMotionAttackIDAttackS3,
    nFTMotionAttackIDAttackHi3,
    nFTMotionAttackIDAttackLw3,
    nFTMotionAttackIDAttackS4,
    nFTMotionAttackIDAttackHi4,
    nFTMotionAttackIDAttackLw4,
    nFTMotionAttackIDAttackAirN,
    nFTMotionAttackIDAttackAirF,
    nFTMotionAttackIDAttackAirB,
    nFTMotionAttackIDAttackAirHi,
    nFTMotionAttackIDAttackAirLw,
    nFTMotionAttackIDSpecialHi,
    nFTMotionAttackIDSpecialN,
    nFTMotionAttackIDSpecialNCopyMario,
    nFTMotionAttackIDSpecialNCopyLuigi,
    nFTMotionAttackIDSpecialNCopyFox,
    nFTMotionAttackIDSpecialNCopySamus,
    nFTMotionAttackIDSpecialNCopyDonkey,
    nFTMotionAttackIDSpecialNCopyPikachu,
    nFTMotionAttackIDSpecialNCopyNess,
    nFTMotionAttackIDSpecialNCopyLink,
    nFTMotionAttackIDSpecialNCopyPurin,
    nFTMotionAttackIDSpecialNCopyCaptain,
    nFTMotionAttackIDSpecialNCopyYoshi,
    nFTMotionAttackIDSpecialLw,
    nFTMotionAttackIDDownAttackD,
    nFTMotionAttackIDDownAttackU,
    nFTMotionAttackIDCliffAttackQuick,
    nFTMotionAttackIDCliffAttackSlow,
    nFTMotionAttackIDThrowF,
    nFTMotionAttackIDThrowB,
    nFTMotionAttackIDBaseEnd = nFTMotionAttackIDThrowB,
    nFTMotionAttackIDItemStart,
    nFTMotionAttackIDSwordSwing1 = nFTMotionAttackIDItemStart,
    nFTMotionAttackIDSwordSwing3,
    nFTMotionAttackIDSwordSwing4,
    nFTMotionAttackIDSwordSwingDash,
    nFTMotionAttackIDBatSwing1,
    nFTMotionAttackIDBatSwing3,
    nFTMotionAttackIDBatSwing4,
    nFTMotionAttackIDBatSwingDash,
    nFTMotionAttackIDHarisenSwing1,
    nFTMotionAttackIDHarisenSwing3,
    nFTMotionAttackIDHarisenSwing4,
    nFTMotionAttackIDHarisenSwingDash,
    nFTMotionAttackIDStarRodSwing1,
    nFTMotionAttackIDStarRodSwing3,
    nFTMotionAttackIDStarRodSwing4,
    nFTMotionAttackIDStarRodSwingDash,
    nFTMotionAttackIDLGunShoot,
    nFTMotionAttackIDFireFlowerShoot,
    nFTMotionAttackIDHammer,
    nFTMotionAttackIDItemThrow,
    nFTMotionAttackIDItemEnd = nFTMotionAttackIDItemThrow,
    nFTMotionAttackIDNull,
    nFTMotionAttackIDEnumCount
};

enum {
    nFTStatusAttackIDNone,
    nFTStatusAttackIDAttackStart,
    nFTStatusAttackIDNormalStart = nFTStatusAttackIDAttackStart,
    nFTStatusAttackIDAttack11 = nFTStatusAttackIDNormalStart,
    nFTStatusAttackIDAttackDash,
    nFTStatusAttackIDAttackS3,
    nFTStatusAttackIDAttackHi3,
    nFTStatusAttackIDAttackLw3,
    nFTStatusAttackIDAttackS4,
    nFTStatusAttackIDAttackHi4,
    nFTStatusAttackIDAttackLw4,
    nFTStatusAttackIDAttackAirN,
    nFTStatusAttackIDAttackAirF,
    nFTStatusAttackIDAttackAirB,
    nFTStatusAttackIDAttackAirHi,
    nFTStatusAttackIDAttackAirLw,
    nFTStatusAttackIDNormalEnd = nFTStatusAttackIDAttackAirLw,
    nFTStatusAttackIDJabStart,
    nFTStatusAttackIDAttack12 = nFTStatusAttackIDJabStart,
    nFTStatusAttackIDAttack13,
    nFTStatusAttackIDAttack100,
    nFTStatusAttackIDJabEnd = nFTStatusAttackIDAttack100,
    nFTStatusAttackIDSpecialStart,
    nFTStatusAttackIDSpecialHi = nFTStatusAttackIDSpecialStart,
    nFTStatusAttackIDSpecialN,
    nFTStatusAttackIDSpecialNCopyMario,
    nFTStatusAttackIDSpecialNCopyLuigi,
    nFTStatusAttackIDSpecialNCopyFox,
    nFTStatusAttackIDSpecialNCopySamus,
    nFTStatusAttackIDSpecialNCopyDonkey,
    nFTStatusAttackIDSpecialNCopyPikachu,
    nFTStatusAttackIDSpecialNCopyNess,
    nFTStatusAttackIDSpecialNCopyLink,
    nFTStatusAttackIDSpecialNCopyPurin,
    nFTStatusAttackIDSpecialNCopyCaptain,
    nFTStatusAttackIDSpecialNCopyYoshi,
    nFTStatusAttackIDSpecialLw,
    nFTStatusAttackIDSpecialEnd = nFTStatusAttackIDSpecialLw,
    nFTStatusAttackIDDownAttackD,
    nFTStatusAttackIDDownAttackU,
    nFTStatusAttackIDCliffAttackQuick,
    nFTStatusAttackIDCliffAttackSlow,
    nFTStatusAttackIDThrowF,
    nFTStatusAttackIDThrowB,
    nFTStatusAttackIDItemStart,
    nFTStatusAttackIDSwordSwing1 = nFTStatusAttackIDItemStart,
    nFTStatusAttackIDSwordSwing3,
    nFTStatusAttackIDSwordSwing4,
    nFTStatusAttackIDSwordSwingDash,
    nFTStatusAttackIDBatSwing1,
    nFTStatusAttackIDBatSwing3,
    nFTStatusAttackIDBatSwing4,
    nFTStatusAttackIDBatSwingDash,
    nFTStatusAttackIDHarisenSwing1,
    nFTStatusAttackIDHarisenSwing3,
    nFTStatusAttackIDHarisenSwing4,
    nFTStatusAttackIDHarisenSwingDash,
    nFTStatusAttackIDStarRodSwing1,
    nFTStatusAttackIDStarRodSwing3,
    nFTStatusAttackIDStarRodSwing4,
    nFTStatusAttackIDStarRodSwingDash,
    nFTStatusAttackIDLGunShoot,
    nFTStatusAttackIDFireFlowerShoot,
    nFTStatusAttackIDHammer,
    nFTStatusAttackIDItemThrow,
    nFTStatusAttackIDItemEnd = nFTStatusAttackIDItemThrow,
    nFTStatusAttackIDNull,
    nFTStatusAttackIDEnumCount
};

enum {
    nFTMarioMotionAttack13 = nFTCommonMotionSpecialStart,
    nFTMarioMotionAppearR,
    nFTMarioMotionAppearL,
    nFTMarioMotionSpecialN,
    nFTMarioMotionSpecialAirN,
    nFTMarioMotionSpecialHi,
    nFTMarioMotionSpecialAirHi,
    nFTMarioMotionSpecialLw,
    nFTMarioMotionSpecialAirLw,
    nFTFoxMotionAttack100Start = nFTCommonMotionSpecialStart,
    nFTFoxMotionAttack100Loop,
    nFTFoxMotionAttack100End,
    nFTFoxMotionAppearR,
    nFTFoxMotionAppearL,
    nFTFoxMotionSpecialN,
    nFTFoxMotionSpecialAirN,
    nFTFoxMotionSpecialHiStart,
    nFTFoxMotionSpecialAirHiStart,
    nFTFoxMotionSpecialHiHold,
    nFTFoxMotionSpecialAirHiHold,
    nFTFoxMotionSpecialHi,
    nFTFoxMotionSpecialAirHi,
    nFTFoxMotionSpecialHiEnd,
    nFTFoxMotionSpecialAirHiEnd,
    nFTFoxMotionSpecialAirHiBound,
    nFTFoxMotionSpecialLwStart,
    nFTFoxMotionSpecialLwLoop,
    nFTFoxMotionSpecialLwHit,
    nFTFoxMotionSpecialLwEnd,
    nFTFoxMotionSpecialAirLwStart,
    nFTFoxMotionSpecialAirLwLoop,
    nFTFoxMotionSpecialAirLwHit,
    nFTFoxMotionSpecialAirLwEnd,
    nFTMarioStatusAttack13 = nFTCommonStatusSpecialStart,
    nFTMarioStatusAppearR,
    nFTMarioStatusAppearL,
    nFTMarioStatusSpecialN,
    nFTMarioStatusSpecialAirN,
    nFTMarioStatusSpecialHi,
    nFTMarioStatusSpecialAirHi,
    nFTMarioStatusSpecialLw,
    nFTMarioStatusSpecialAirLw,
    nFTFoxStatusAttack100Start = nFTCommonStatusSpecialStart,
    nFTFoxStatusAttack100Loop,
    nFTFoxStatusAttack100End,
    nFTFoxStatusAppearR,
    nFTFoxStatusAppearL,
    nFTFoxStatusSpecialN,
    nFTFoxStatusSpecialAirN,
    nFTFoxStatusSpecialHiStart,
    nFTFoxStatusSpecialAirHiStart,
    nFTFoxStatusSpecialHiHold,
    nFTFoxStatusSpecialAirHiHold,
    nFTFoxStatusSpecialHi,
    nFTFoxStatusSpecialAirHi,
    nFTFoxStatusSpecialHiEnd,
    nFTFoxStatusSpecialAirHiEnd,
    nFTFoxStatusSpecialAirHiBound,
    nFTFoxStatusSpecialLwScopeStart,
    nFTFoxStatusSpecialLwStart = nFTFoxStatusSpecialLwScopeStart,
    nFTFoxStatusSpecialLwHit,
    nFTFoxStatusSpecialLwEnd,
    nFTFoxStatusSpecialLwLoop,
    nFTFoxStatusSpecialLwTurn,
    nFTFoxStatusSpecialAirLwStart,
    nFTFoxStatusSpecialAirLwHit,
    nFTFoxStatusSpecialAirLwEnd,
    nFTFoxStatusSpecialAirLwLoop,
    nFTFoxStatusSpecialAirLwTurn,
    nFTFoxStatusSpecialLwScopeEnd = nFTFoxStatusSpecialAirLwTurn,
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
    nMPMaterial1 = 1,
    nMPMaterial2 = 2,
    nMPMaterial3 = 3,
    nMPMaterial4 = 4,
    nMPMaterial5 = 5,
    nMPMaterial6 = 6,
    nMPMaterialFireWeakS1 = 7,
    nMPMaterialFireStrongS1 = 8,
    nMPMaterialFireWeakHi1 = 9,
    nMPMaterialSpikes = 10,
    nMPMaterialFireWeakHi2 = 11,
    nMPMaterialDokanL = 12,
    nMPMaterialDokanR = 13,
    nMPMaterialDetect = 14,
    nMPMaterialFireWeakHi3 = 15,
    nMPMaterialEnumCount = 16
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
    nGMHitLevelSmall = 0,
    nGMHitLevelMiddle = 1,
    nGMHitLevelLarge = 2,
    nGMHitLevelEnumCount = 3
};

enum {
    nGMHitEnvironmentAcid = 0,
    nGMHitEnvironmentPowerBlock = 1,
    nGMHitEnvironmentTwister = 2,
    nGMHitEnvironmentTaruCann = 3
};

enum {
    nFTCameraModeDefault = 0,
    nFTCameraModeGhost = 1,
    nFTCameraModeDeadUp = 2,
    nFTCameraModeEntry = 3,
    nFTCameraModeExplain = 4
};

enum {
    nFTSlopeContourLFoot = 0,
    nFTSlopeContourRFoot = 1,
    nFTSlopeContourFull = 2
};

#define FTSLOPECONTOUR_FLAG_LFOOT (1u << nFTSlopeContourLFoot)
#define FTSLOPECONTOUR_FLAG_RFOOT (1u << nFTSlopeContourRFoot)
#define FTSLOPECONTOUR_FLAG_FULL (1u << nFTSlopeContourFull)

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

typedef enum FTMotionEvent {
    nFTMotionEventEnd,
    nFTMotionEventSyncWait,
    nFTMotionEventAsyncWait,
    nFTMotionEventMakeAttackColl,
    nFTMotionEventMakeAttackCollScaled,
    nFTMotionEventClearAttackCollID,
    nFTMotionEventClearAttackCollAll,
    nFTMotionEventSetAttackCollOffset,
    nFTMotionEventSetAttackCollDamage,
    nFTMotionEventSetAttackCollSize,
    nFTMotionEventSetAttackCollSoundLevel,
    nFTMotionEventRefreshAttackCollID,
    nFTMotionEventSetThrow,
    nFTMotionEventSetDamageThrown,
    nFTMotionEventPlayFGM,
    nFTMotionEventPlayLoopSFXStoreInfo,
    nFTMotionEventStopLoopSFX,
    nFTMotionEventPlayVoiceStoreInfo,
    nFTMotionEventPlayLoopVoiceStoreInfo,
    nFTMotionEventPlayFGMStoreInfo,
    nFTMotionEventPlaySmashVoice,
    nFTMotionEventSetFlag0,
    nFTMotionEventSetFlag1,
    nFTMotionEventSetFlag2,
    nFTMotionEventSetFlag3,
    nFTMotionEventSetAirJumpAdd,
    nFTMotionEventSetAirJumpMax,
    nFTMotionEventSetHitStatusPartAll,
    nFTMotionEventSetHitStatusPartID,
    nFTMotionEventSetHitStatusAll,
    nFTMotionEventResetDamageCollPartAll,
    nFTMotionEventSetDamageCollPartID,
    nFTMotionEventLoopBegin,
    nFTMotionEventLoopEnd,
    nFTMotionEventSubroutine,
    nFTMotionEventReturn,
    nFTMotionEventGoto,
    nFTMotionEventPauseScript,
    nFTMotionEventEffect,
    nFTMotionEventEffectItemHold,
    nFTMotionEventSetModelPartID,
    nFTMotionEventResetModelPartAll,
    nFTMotionEventHideModelPartAll,
    nFTMotionEventSetTexturePartID,
    nFTMotionEventSetColAnim,
    nFTMotionEventResetColAnim,
    nFTMotionEventSetParallelScript,
    nFTMotionEventSetSlopeContour,
    nFTMotionEventHideItem,
    nFTMotionEventMakeRumble,
    nFTMotionEventStopRumble,
    nFTMotionEventSetAfterImage
} FTMotionEvent;

#define FTANIM_FLAG_TRANSN_JOINT 0x80000000u
#define FTANIM_FLAG_XROTN_JOINT 0x40000000u
#define FTANIM_FLAG_YROTN_JOINT 0x20000000u
#define FTANIM_FLAG_SUBMOTION_SCRIPT 0x00000010u
#define FTANIM_FLAG_ANIMJOINT 0x00000008u
#define FTANIM_FLAG_TRANSLATE_SCALES 0x00000004u
#define FTANIM_FLAG_SHIELDPOSE 0x00000002u
#define FTANIM_FLAG_ANIMLOCKS 0x00000001u
#define FTANIM_FLAG_NONE 0x00000000u

/*
 * BattleShip animation and motion payloads are stored with the first declared
 * bitfield in the high bits, as encoded by the original ft* macros.
 */
#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#define NDS_FTMOTION_BIGFIELD_ORDER 0
#else
#define NDS_FTMOTION_BIGFIELD_ORDER 1
#endif

#define ftMotionCommandEnd() GC_FIELDSET(nFTMotionEventEnd, 26, 6)
#define ftMotionCommandWait(frames) \
    (GC_FIELDSET(nFTMotionEventSyncWait, 26, 6) | GC_FIELDSET(frames, 0, 26))
#define ftMotionCommandWaitAsync(frames) \
    (GC_FIELDSET(nFTMotionEventAsyncWait, 26, 6) | GC_FIELDSET(frames, 0, 26))
#define ftMotionPlayFGM(fgm_id) \
    (GC_FIELDSET(nFTMotionEventPlayFGM, 26, 6) | GC_FIELDSET(fgm_id, 0, 26))
#define ftMotionCommandSubroutineS1() \
    GC_FIELDSET(nFTMotionEventSubroutine, 26, 6)
#define ftMotionCommandSubroutineS2(addr) ((uintptr_t)addr)
#define ftMotionCommandSubroutine(addr) \
    ftMotionCommandSubroutineS1(), ftMotionCommandSubroutineS2(addr)
#define ftMotionCommandReturn() GC_FIELDSET(nFTMotionEventReturn, 26, 6)
#define ftMotionCommandGotoS1() GC_FIELDSET(nFTMotionEventGoto, 26, 6)
#define ftMotionCommandGotoS2(addr) ((uintptr_t)addr)
#define ftMotionCommandGoto(addr) \
    ftMotionCommandGotoS1(), ftMotionCommandGotoS2(addr)
#define ftMotionCommandSetSlopeContour(flag) \
    (GC_FIELDSET(nFTMotionEventSetSlopeContour, 26, 6) | \
     GC_FIELDSET(flag, 0, 26))

#define ftMotionEventAdvance(event, type) \
    ((event)->p_script = \
         (void *)((uintptr_t)(event)->p_script + sizeof(type)))
#define ftMotionEventCast(event, type) ((type *)(event)->p_script)
#define ftMotionEventCastAdvance(event, type) ((type *)(event)->p_script++)

typedef struct FTPhysics {
    Vec3f vel_air;
    Vec3f vel_damage_air;
    Vec3f vel_ground;
    f32 vel_damage_ground;
    f32 vel_jostle_x;
    f32 vel_jostle_z;
} FTPhysics;

typedef union FTAnimDesc {
    u32 word;
    struct {
#if NDS_FTMOTION_BIGFIELD_ORDER
        ub32 is_use_xrotn_joint : 1;
        ub32 is_use_transn_joint : 1;
        ub32 is_use_yrotn_joint : 1;
        ub32 is_enabled_joints : 24;
        ub32 is_use_submotion_script : 1;
        ub32 is_anim_joint : 1;
        ub32 is_have_translate_scale : 1;
        ub32 is_use_shieldpose : 1;
        ub32 is_use_animlocks : 1;
#else
        ub32 is_use_animlocks : 1;
        ub32 is_use_shieldpose : 1;
        ub32 is_have_translate_scale : 1;
        ub32 is_anim_joint : 1;
        ub32 is_use_submotion_script : 1;
        ub32 is_enabled_joints : 24;
        ub32 is_use_yrotn_joint : 1;
        ub32 is_use_transn_joint : 1;
        ub32 is_use_xrotn_joint : 1;
#endif
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
    u32 file_main_id;
    u32 file_mainmotion_id;
    u32 file_submotion_id;
    u32 file_model_id;
    u32 file_shieldpose_id;
    u32 file_special1_id;
    u32 file_special2_id;
    u32 file_special3_id;
    u32 file_special4_id;
    size_t file_main_size;
    void **p_file_main;
    void **p_file_mainmotion;
    void **p_file_submotion;
    void **p_file_model;
    void **p_file_shieldpose;
    void **p_file_special1;
    void **p_file_special2;
    void **p_file_special3;
    void **p_file_special4;
    s32 *p_particle;
    uintptr_t particles_script_lo;
    uintptr_t particles_script_hi;
    uintptr_t particles_texture_lo;
    uintptr_t particles_texture_hi;
    intptr_t o_attributes;
    FTMotionDescArray *mainmotion;
    FTMotionDescArray *submotion;
    s32 mainmotion_array_count;
    s32 *submotion_array_count;
    size_t file_anim_size;
};

#define NDS_FTDATA_SIZE 120u
#define NDS_FTDATA_OFF_FILE_MAIN_ID 0u
#define NDS_FTDATA_OFF_FILE_MAINMOTION_ID 4u
#define NDS_FTDATA_OFF_FILE_SUBMOTION_ID 8u
#define NDS_FTDATA_OFF_FILE_MODEL_ID 12u
#define NDS_FTDATA_OFF_FILE_SHIELDPOSE_ID 16u
#define NDS_FTDATA_OFF_FILE_SPECIAL1_ID 20u
#define NDS_FTDATA_OFF_FILE_SPECIAL2_ID 24u
#define NDS_FTDATA_OFF_FILE_SPECIAL3_ID 28u
#define NDS_FTDATA_OFF_FILE_SPECIAL4_ID 32u
#define NDS_FTDATA_OFF_FILE_MAIN_SIZE 36u
#define NDS_FTDATA_OFF_P_FILE_MAIN 40u
#define NDS_FTDATA_OFF_P_FILE_MAINMOTION 44u
#define NDS_FTDATA_OFF_P_FILE_SUBMOTION 48u
#define NDS_FTDATA_OFF_P_FILE_MODEL 52u
#define NDS_FTDATA_OFF_P_FILE_SHIELDPOSE 56u
#define NDS_FTDATA_OFF_P_FILE_SPECIAL1 60u
#define NDS_FTDATA_OFF_P_FILE_SPECIAL2 64u
#define NDS_FTDATA_OFF_P_FILE_SPECIAL3 68u
#define NDS_FTDATA_OFF_P_FILE_SPECIAL4 72u
#define NDS_FTDATA_OFF_P_PARTICLE 76u
#define NDS_FTDATA_OFF_PARTICLES_SCRIPT_LO 80u
#define NDS_FTDATA_OFF_PARTICLES_SCRIPT_HI 84u
#define NDS_FTDATA_OFF_PARTICLES_TEXTURE_LO 88u
#define NDS_FTDATA_OFF_PARTICLES_TEXTURE_HI 92u
#define NDS_FTDATA_OFF_O_ATTRIBUTES 96u
#define NDS_FTDATA_OFF_MAINMOTION 100u
#define NDS_FTDATA_OFF_SUBMOTION 104u
#define NDS_FTDATA_OFF_MAINMOTION_ARRAY_COUNT 108u
#define NDS_FTDATA_OFF_SUBMOTION_ARRAY_COUNT 112u
#define NDS_FTDATA_OFF_FILE_ANIM_SIZE 116u

#define NDS_FTDATA_ASSERT_OFF(field, expected)                              \
    _Static_assert(offsetof(FTData, field) == (expected),                    \
                   "FTData " #field " offset changed")

_Static_assert(sizeof(FTData) == NDS_FTDATA_SIZE, "FTData size changed");
NDS_FTDATA_ASSERT_OFF(file_main_id, NDS_FTDATA_OFF_FILE_MAIN_ID);
NDS_FTDATA_ASSERT_OFF(file_mainmotion_id,
                      NDS_FTDATA_OFF_FILE_MAINMOTION_ID);
NDS_FTDATA_ASSERT_OFF(file_submotion_id, NDS_FTDATA_OFF_FILE_SUBMOTION_ID);
NDS_FTDATA_ASSERT_OFF(file_model_id, NDS_FTDATA_OFF_FILE_MODEL_ID);
NDS_FTDATA_ASSERT_OFF(file_shieldpose_id, NDS_FTDATA_OFF_FILE_SHIELDPOSE_ID);
NDS_FTDATA_ASSERT_OFF(file_special1_id, NDS_FTDATA_OFF_FILE_SPECIAL1_ID);
NDS_FTDATA_ASSERT_OFF(file_special2_id, NDS_FTDATA_OFF_FILE_SPECIAL2_ID);
NDS_FTDATA_ASSERT_OFF(file_special3_id, NDS_FTDATA_OFF_FILE_SPECIAL3_ID);
NDS_FTDATA_ASSERT_OFF(file_special4_id, NDS_FTDATA_OFF_FILE_SPECIAL4_ID);
NDS_FTDATA_ASSERT_OFF(file_main_size, NDS_FTDATA_OFF_FILE_MAIN_SIZE);
NDS_FTDATA_ASSERT_OFF(p_file_main, NDS_FTDATA_OFF_P_FILE_MAIN);
NDS_FTDATA_ASSERT_OFF(p_file_mainmotion, NDS_FTDATA_OFF_P_FILE_MAINMOTION);
NDS_FTDATA_ASSERT_OFF(p_file_submotion, NDS_FTDATA_OFF_P_FILE_SUBMOTION);
NDS_FTDATA_ASSERT_OFF(p_file_model, NDS_FTDATA_OFF_P_FILE_MODEL);
NDS_FTDATA_ASSERT_OFF(p_file_shieldpose, NDS_FTDATA_OFF_P_FILE_SHIELDPOSE);
NDS_FTDATA_ASSERT_OFF(p_file_special1, NDS_FTDATA_OFF_P_FILE_SPECIAL1);
NDS_FTDATA_ASSERT_OFF(p_file_special2, NDS_FTDATA_OFF_P_FILE_SPECIAL2);
NDS_FTDATA_ASSERT_OFF(p_file_special3, NDS_FTDATA_OFF_P_FILE_SPECIAL3);
NDS_FTDATA_ASSERT_OFF(p_file_special4, NDS_FTDATA_OFF_P_FILE_SPECIAL4);
NDS_FTDATA_ASSERT_OFF(p_particle, NDS_FTDATA_OFF_P_PARTICLE);
NDS_FTDATA_ASSERT_OFF(particles_script_lo,
                      NDS_FTDATA_OFF_PARTICLES_SCRIPT_LO);
NDS_FTDATA_ASSERT_OFF(particles_script_hi,
                      NDS_FTDATA_OFF_PARTICLES_SCRIPT_HI);
NDS_FTDATA_ASSERT_OFF(particles_texture_lo,
                      NDS_FTDATA_OFF_PARTICLES_TEXTURE_LO);
NDS_FTDATA_ASSERT_OFF(particles_texture_hi,
                      NDS_FTDATA_OFF_PARTICLES_TEXTURE_HI);
NDS_FTDATA_ASSERT_OFF(o_attributes, NDS_FTDATA_OFF_O_ATTRIBUTES);
NDS_FTDATA_ASSERT_OFF(mainmotion, NDS_FTDATA_OFF_MAINMOTION);
NDS_FTDATA_ASSERT_OFF(submotion, NDS_FTDATA_OFF_SUBMOTION);
NDS_FTDATA_ASSERT_OFF(mainmotion_array_count,
                      NDS_FTDATA_OFF_MAINMOTION_ARRAY_COUNT);
NDS_FTDATA_ASSERT_OFF(submotion_array_count,
                      NDS_FTDATA_OFF_SUBMOTION_ARRAY_COUNT);
NDS_FTDATA_ASSERT_OFF(file_anim_size, NDS_FTDATA_OFF_FILE_ANIM_SIZE);
#undef NDS_FTDATA_ASSERT_OFF

typedef struct FTMotionEventDefault {
#if NDS_FTMOTION_BIGFIELD_ORDER
    u32 opcode : 6;
    u32 value : 26;
#else
    u32 value : 26;
    u32 opcode : 6;
#endif
} FTMotionEventDefault;

typedef struct FTMotionEventDouble {
#if NDS_FTMOTION_BIGFIELD_ORDER
    u32 opcode : 6;
    u32 pad1 : 26;
#else
    u32 pad1 : 26;
    u32 opcode : 6;
#endif
    u32 pad2 : 32;
} FTMotionEventDouble;

typedef struct FTMotionEventMakeAttack1 {
#if NDS_FTMOTION_BIGFIELD_ORDER
    u32 opcode : 6;
    u32 attack_id : 3;
    u32 group_id : 3;
    s32 joint_id : 7;
    u32 damage : 8;
    ub32 can_rebound : 1;
    u32 element : 4;
#else
    u32 element : 4;
    ub32 can_rebound : 1;
    u32 damage : 8;
    s32 joint_id : 7;
    u32 group_id : 3;
    u32 attack_id : 3;
    u32 opcode : 6;
#endif
} FTMotionEventMakeAttack1;

typedef struct FTMotionEventMakeAttack2 {
#if NDS_FTMOTION_BIGFIELD_ORDER
    u32 size : 16;
    s32 off_x : 16;
#else
    s32 off_x : 16;
    u32 size : 16;
#endif
} FTMotionEventMakeAttack2;

typedef struct FTMotionEventMakeAttack3 {
#if NDS_FTMOTION_BIGFIELD_ORDER
    s32 off_y : 16;
    s32 off_z : 16;
#else
    s32 off_z : 16;
    s32 off_y : 16;
#endif
} FTMotionEventMakeAttack3;

typedef struct FTMotionEventMakeAttack4 {
#if NDS_FTMOTION_BIGFIELD_ORDER
    s32 angle : 10;
    u32 knockback_scale : 10;
    u32 knockback_weight : 10;
    u32 is_hit_ground_air : 2;
#else
    u32 is_hit_ground_air : 2;
    u32 knockback_weight : 10;
    u32 knockback_scale : 10;
    s32 angle : 10;
#endif
} FTMotionEventMakeAttack4;

typedef struct FTMotionEventMakeAttack5 {
#if NDS_FTMOTION_BIGFIELD_ORDER
    s32 shield_damage : 8;
    u32 fgm_level : 3;
    u32 fgm_kind : 4;
    u32 knockback_base : 10;
#else
    u32 pad : 7;
    u32 knockback_base : 10;
    u32 fgm_kind : 4;
    u32 fgm_level : 3;
    s32 shield_damage : 8;
#endif
} FTMotionEventMakeAttack5;

typedef struct FTMotionEventMakeAttack {
    FTMotionEventMakeAttack1 s1;
    FTMotionEventMakeAttack2 s2;
    FTMotionEventMakeAttack3 s3;
    FTMotionEventMakeAttack4 s4;
    FTMotionEventMakeAttack5 s5;
} FTMotionEventMakeAttack;

typedef struct FTMotionEventSetAttackOffset1 {
#if NDS_FTMOTION_BIGFIELD_ORDER
    u32 opcode : 6;
    u32 attack_id : 3;
    s32 off_x : 16;
#else
    u32 pad : 7;
    s32 off_x : 16;
    u32 attack_id : 3;
    u32 opcode : 6;
#endif
} FTMotionEventSetAttackOffset1;

typedef struct FTMotionEventSetAttackOffset2 {
#if NDS_FTMOTION_BIGFIELD_ORDER
    s32 off_y : 16;
    s32 off_z : 16;
#else
    s32 off_z : 16;
    s32 off_y : 16;
#endif
} FTMotionEventSetAttackOffset2;

typedef struct FTMotionEventSetAttackOffset {
    FTMotionEventSetAttackOffset1 s1;
    FTMotionEventSetAttackOffset2 s2;
} FTMotionEventSetAttackOffset;

typedef struct FTMotionEventSetAttackCollDamage {
#if NDS_FTMOTION_BIGFIELD_ORDER
    u32 opcode : 6;
    u32 attack_id : 3;
    u32 damage : 8;
#else
    u32 pad : 15;
    u32 damage : 8;
    u32 attack_id : 3;
    u32 opcode : 6;
#endif
} FTMotionEventSetAttackCollDamage;

typedef struct FTMotionEventSetAttackCollSize {
#if NDS_FTMOTION_BIGFIELD_ORDER
    u32 opcode : 6;
    u32 attack_id : 3;
    u32 size : 16;
#else
    u32 pad : 7;
    u32 size : 16;
    u32 attack_id : 3;
    u32 opcode : 6;
#endif
} FTMotionEventSetAttackCollSize;

typedef struct FTMotionEventSetAttackCollSound {
#if NDS_FTMOTION_BIGFIELD_ORDER
    u32 opcode : 6;
    u32 attack_id : 3;
    u32 fgm_level : 3;
#else
    u32 pad : 20;
    u32 fgm_level : 3;
    u32 attack_id : 3;
    u32 opcode : 6;
#endif
} FTMotionEventSetAttackCollSound;

typedef struct FTMotionEventSetThrow1 {
#if NDS_FTMOTION_BIGFIELD_ORDER
    u32 opcode : 6;
#else
    u32 pad : 26;
    u32 opcode : 6;
#endif
} FTMotionEventSetThrow1;

typedef struct FTMotionEventSetThrow2 {
    FTThrowHitDesc *throw_desc;
} FTMotionEventSetThrow2;

typedef struct FTMotionEventSetThrow {
    FTMotionEventSetThrow1 s1;
    FTMotionEventSetThrow2 s2;
} FTMotionEventSetThrow;

typedef struct FTMotionEventMakeEffect1 {
#if NDS_FTMOTION_BIGFIELD_ORDER
    u32 opcode : 6;
    s32 joint_id : 7;
    u32 effect_id : 9;
    u32 flag : 10;
#else
    u32 flag : 10;
    u32 effect_id : 9;
    s32 joint_id : 7;
    u32 opcode : 6;
#endif
} FTMotionEventMakeEffect1;

typedef struct FTMotionEventMakeEffect2 {
#if NDS_FTMOTION_BIGFIELD_ORDER
    s32 off_x : 16;
    s32 off_y : 16;
#else
    s32 off_y : 16;
    s32 off_x : 16;
#endif
} FTMotionEventMakeEffect2;

typedef struct FTMotionEventMakeEffect3 {
#if NDS_FTMOTION_BIGFIELD_ORDER
    s32 off_z : 16;
    s32 rng_x : 16;
#else
    s32 rng_x : 16;
    s32 off_z : 16;
#endif
} FTMotionEventMakeEffect3;

typedef struct FTMotionEventMakeEffect4 {
#if NDS_FTMOTION_BIGFIELD_ORDER
    s32 rng_y : 16;
    s32 rng_z : 16;
#else
    s32 rng_z : 16;
    s32 rng_y : 16;
#endif
} FTMotionEventMakeEffect4;

typedef struct FTMotionEventMakeEffect {
    FTMotionEventMakeEffect1 s1;
    FTMotionEventMakeEffect2 s2;
    FTMotionEventMakeEffect3 s3;
    FTMotionEventMakeEffect4 s4;
} FTMotionEventMakeEffect;

typedef struct FTMotionEventSetHitStatusPartID {
#if NDS_FTMOTION_BIGFIELD_ORDER
    u32 opcode : 6;
    s32 joint_id : 7;
    u32 hitstatus : 19;
#else
    u32 hitstatus : 19;
    s32 joint_id : 7;
    u32 opcode : 6;
#endif
} FTMotionEventSetHitStatusPartID;

typedef struct FTMotionEventSetDamageCollPartID1 {
#if NDS_FTMOTION_BIGFIELD_ORDER
    u32 opcode : 6;
    s32 joint_id : 7;
#else
    u32 pad : 19;
    s32 joint_id : 7;
    u32 opcode : 6;
#endif
} FTMotionEventSetDamageCollPartID1;

typedef struct FTMotionEventSetDamageCollPartID2 {
#if NDS_FTMOTION_BIGFIELD_ORDER
    s32 off_x : 16;
    s32 off_y : 16;
#else
    s32 off_y : 16;
    s32 off_x : 16;
#endif
} FTMotionEventSetDamageCollPartID2;

typedef struct FTMotionEventSetDamageCollPartID3 {
#if NDS_FTMOTION_BIGFIELD_ORDER
    s32 off_z : 16;
    s32 size_x : 16;
#else
    s32 size_x : 16;
    s32 off_z : 16;
#endif
} FTMotionEventSetDamageCollPartID3;

typedef struct FTMotionEventSetDamageCollPartID4 {
#if NDS_FTMOTION_BIGFIELD_ORDER
    s32 size_y : 16;
    s32 size_z : 16;
#else
    s32 size_z : 16;
    s32 size_y : 16;
#endif
} FTMotionEventSetDamageCollPartID4;

typedef struct FTMotionEventSetDamageCollPartID {
    FTMotionEventSetDamageCollPartID1 s1;
    FTMotionEventSetDamageCollPartID2 s2;
    FTMotionEventSetDamageCollPartID3 s3;
    FTMotionEventSetDamageCollPartID4 s4;
} FTMotionEventSetDamageCollPartID;

typedef struct FTMotionEventSubroutine1 {
#if NDS_FTMOTION_BIGFIELD_ORDER
    u32 opcode : 6;
#else
    u32 pad : 26;
    u32 opcode : 6;
#endif
} FTMotionEventSubroutine1;

typedef struct FTMotionEventSubroutine2 {
    void *p_goto;
} FTMotionEventSubroutine2;

typedef struct FTMotionEventSubroutine {
    FTMotionEventSubroutine1 s1;
    FTMotionEventSubroutine2 s2;
} FTMotionEventSubroutine;

typedef struct FTMotionEventSetDamageThrown1 {
#if NDS_FTMOTION_BIGFIELD_ORDER
    u32 opcode : 6;
#else
    u32 pad : 26;
    u32 opcode : 6;
#endif
} FTMotionEventSetDamageThrown1;

typedef struct FTMotionEventSetDamageThrown2 {
    void *p_subroutine;
} FTMotionEventSetDamageThrown2;

typedef struct FTMotionDamageScript {
    void *p_script[2][nFTKindEnumCount];
} FTMotionDamageScript;

typedef struct FTMotionEventSetDamageThrown {
    FTMotionEventSetDamageThrown1 s1;
    FTMotionEventSetDamageThrown2 s2;
} FTMotionEventSetDamageThrown;

typedef struct FTMotionEventGoto1 {
#if NDS_FTMOTION_BIGFIELD_ORDER
    u32 opcode : 6;
#else
    u32 pad : 26;
    u32 opcode : 6;
#endif
} FTMotionEventGoto1;

typedef struct FTMotionEventGoto2 {
    void *p_goto;
} FTMotionEventGoto2;

typedef struct FTMotionEventGoto {
    FTMotionEventGoto1 s1;
    FTMotionEventGoto2 s2;
} FTMotionEventGoto;

typedef struct FTMotionEventParallel1 {
#if NDS_FTMOTION_BIGFIELD_ORDER
    u32 opcode : 6;
#else
    u32 pad : 26;
    u32 opcode : 6;
#endif
} FTMotionEventParallel1;

typedef struct FTMotionEventParallel2 {
    void *p_goto;
} FTMotionEventParallel2;

typedef struct FTMotionEventParallel {
    FTMotionEventParallel1 s1;
    FTMotionEventParallel2 s2;
} FTMotionEventParallel;

typedef struct FTMotionEventSetModelPartID {
#if NDS_FTMOTION_BIGFIELD_ORDER
    u32 opcode : 6;
    s32 joint_id : 7;
    s32 modelpart_id : 19;
#else
    s32 modelpart_id : 19;
    s32 joint_id : 7;
    u32 opcode : 6;
#endif
} FTMotionEventSetModelPartID;

typedef struct FTMotionEventSetTexturePartID {
#if NDS_FTMOTION_BIGFIELD_ORDER
    u32 opcode : 6;
    u32 texturepart_id : 6;
    u32 frame : 20;
#else
    u32 frame : 20;
    u32 texturepart_id : 6;
    u32 opcode : 6;
#endif
} FTMotionEventSetTexturePartID;

typedef struct FTMotionEventSetColAnimID {
#if NDS_FTMOTION_BIGFIELD_ORDER
    u32 opcode : 6;
    u32 colanim_id : 8;
    u32 length : 18;
#else
    u32 length : 18;
    u32 colanim_id : 8;
    u32 opcode : 6;
#endif
} FTMotionEventSetColAnimID;

typedef struct FTMotionEventSetSlopeContour {
#if NDS_FTMOTION_BIGFIELD_ORDER
    u32 opcode : 6;
    u32 pad : 23;
    u32 flags : 3;
#else
    u32 flags : 3;
    u32 pad : 23;
    u32 opcode : 6;
#endif
} FTMotionEventSetSlopeContour;

typedef struct FTMotionEventSetAfterImage {
#if NDS_FTMOTION_BIGFIELD_ORDER
    u32 opcode : 6;
    u32 is_itemswing : 8;
    s32 drawstatus : 18;
#else
    s32 drawstatus : 18;
    u32 is_itemswing : 8;
    u32 opcode : 6;
#endif
} FTMotionEventSetAfterImage;

typedef struct FTMotionEventMakeRumble {
#if NDS_FTMOTION_BIGFIELD_ORDER
    u32 opcode : 6;
    u32 length : 13;
    u32 rumble_id : 13;
#else
    u32 rumble_id : 13;
    u32 length : 13;
    u32 opcode : 6;
#endif
} FTMotionEventMakeRumble;

typedef struct FTMotionEventStopRumble {
#if NDS_FTMOTION_BIGFIELD_ORDER
    u32 opcode : 6;
    u32 rumble_id : 26;
#else
    u32 rumble_id : 26;
    u32 opcode : 6;
#endif
} FTMotionEventStopRumble;

typedef union FTMotionVars {
    struct {
        u32 flag0;
        u32 flag1;
        u32 flag2;
        u32 flag3;
    } flags;
    struct {
        sb32 is_throw_item;
        u8 unk1;
        u32 damage : 24;
        u8 unk2;
        u32 vel : 12;
        s32 angle : 12;
    } item_throw;
    u32 word;
} FTMotionVars;

typedef struct FTComputer {
    u8 objective;
    u8 objective_base;
    u8 input_kind;
    u8 behavior;
    u8 unk_ftcom_0x4;
    u8 trait;
    u8 unk_ftcom_0x6;
    u8 input_wait;
    u8 *p_command;
    sb32 (*proc_com)(GObj *);
    u16 jump_wait;
    u16 item_track_wait;
    u16 behavior_change_wait;
    u16 unk_ftcom_0x16;
    u16 walk_stop_wait;
    u16 fighter_follow_since;
    u16 fighter_follow_wait;
    u16 fighter_follow_end;
    u16 unk_ftcom_0x20;
    u16 target_find_wait;
    u16 wiggle_wait;
    u16 target_damage_percent;
    u16 attack_count;
    u16 appeal_attempt_frames;
    u16 stand_stop_wait;
    GObj *target_gobj;
    u8 item_throw_wait;
    u8 unk_ftcom_0x35;
    u8 unk_ftcom_0x36;
    u8 input_repeat_count;
    u8 unk_ftcom_0x38;
    u8 stickn_button_a_count;
    u8 sticktilts_button_a_count;
    u8 sticksmashs_button_a_count;
    u8 sticktilthi_button_a_count;
    u8 sticksmashhi_button_a_count;
    u8 sticktiltlw_button_a_count;
    u8 sticksmashlw_button_a_count;
    u8 sticksmashs_button_b_count;
    u8 sticksmashhi_button_b_count;
    u8 sticksmashlw_button_b_count;
    u8 stickn_button_z_button_a_count;
    ub32 unk_ftcom_0x44;
    ub32 ftcom_flags_0x48_b0 : 1;
    ub32 ftcom_flags_0x48_b1 : 1;
    ub32 ftcom_flags_0x48_b2 : 1;
    ub32 ftcom_flags_0x48_b3 : 1;
    ub32 ftcom_flags_0x48_b4 : 1;
    ub32 ftcom_flags_0x48_b5 : 1;
    ub32 ftcom_flags_0x48_b6 : 1;
    ub32 ftcom_flags_0x48_b7 : 1;
    ub32 is_within_vertical_bounds : 1;
    ub32 ftcom_flags_0x49_b1 : 1;
    ub32 ftcom_flags_0x49_b2 : 1;
    ub32 ftcom_flags_0x49_b3 : 1;
    ub32 is_counterattack : 1;
    ub32 is_shield_item_weapon : 1;
    ub32 is_opponent_ra : 1;
    ub32 is_attempt_specialhi_recovery : 1;
    ub32 ftcom_flags_0x4A_b0 : 1;
    ub32 ftcom_flags_0x4A_b1 : 1;
    ub32 is_stop_stand : 1;
    Vec2f cliff_left_pos;
    Vec2f cliff_right_pos;
    s32 target_line_id;
    Vec2f target_pos;
    f32 target_dist;
    void *target_user;
    Vec2f origin_pos;
    Vec2f edge_pos;
    Vec2f stand_pos;
    s32 floor_line_id;
    f32 dash_predict;
    f32 jump_predict;
} FTComputer;

typedef struct FTKey {
    s32 input_wait;
    FTKeyEvent *script;
} FTKey;

typedef struct ftCommonDeadStatusVars {
    s32 wait;
    Vec3f pos;
} ftCommonDeadStatusVars;

typedef struct ftCommonRebirthStatusVars {
    Vec3f halo_offset;
    Vec3f pos;
    s32 halo_despawn_wait;
    s32 halo_number;
    s32 halo_lower_wait;
} ftCommonRebirthStatusVars;

typedef struct ftCommonSleepStatusVars {
    s32 stock_steal_wait;
} ftCommonSleepStatusVars;

typedef struct ftCommonEntryStatusVars {
    s32 entry_wait;
    s32 lr;
    s32 floor_line_id;
    sb32 is_rotate;
} ftCommonEntryStatusVars;

typedef struct ftCommonTurnStatusVars {
    sb32 is_allow_turn_direction;
    sb32 is_disable_sa_interrupts;
    u16 button_mask;
    s32 lr_dash;
    s32 lr_turn;
    s32 attacks4_buffer;
} ftCommonTurnStatusVars;

typedef struct ftCommonKneeBendStatusVars {
    f32 jump_force;
    f32 anim_frame;
    s32 input_source;
    sb32 is_shorthop;
} ftCommonKneeBendStatusVars;

typedef struct ftCommonJumpAerialStatusVars {
    f32 drift;
    f32 vel_x;
    s32 turn_tics;
} ftCommonJumpAerialStatusVars;

typedef struct ftCommonDamageStatusVars {
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
} ftCommonDamageStatusVars;

typedef struct ftCommonSquatStatusVars {
    sb32 is_allow_pass;
    s32 pass_wait;
    s32 unk_0x8;
} ftCommonSquatStatusVars;

typedef struct ftCommonDokanStatusVars {
    s32 material;
    s32 mapobj_kind;
    Vec3f pos_curr;
    Vec3f target_pos;
    s32 pos_adjust_wait;
    s32 playertag_wait;
    s32 turn_stop_wait;
} ftCommonDokanStatusVars;

typedef struct ftCommonLandingStatusVars {
    sb32 is_allow_interrupt;
} ftCommonLandingStatusVars;

typedef struct ftCommonFallSpecialStatusVars {
    f32 drift;
    sb32 is_allow_pass;
    sb32 is_goto_landing;
    f32 landing_lag;
    sb32 is_allow_interrupt;
    sb32 is_fall_accelerate;
} ftCommonFallSpecialStatusVars;

typedef struct ftCommonTwisterStatusVars {
    s32 release_wait;
    GObj *tornado_gobj;
} ftCommonTwisterStatusVars;

typedef struct ftCommonTaruCannStatusVars {
    s32 release_wait;
    s32 shoot_wait;
    GObj *tarucann_gobj;
} ftCommonTaruCannStatusVars;

typedef struct ftCommonDownWaITStatusVars {
    s32 stand_wait;
} ftCommonDownWaITStatusVars;

typedef struct ftCommonDownBounceStatusVars {
    s32 attack_buffer;
} ftCommonDownBounceStatusVars;

typedef struct ftCommonReboundStatusVars {
    f32 anim_speed;
    f32 rebound_timer;
} ftCommonReboundStatusVars;

typedef struct ftCommonCliffWaitStatusVars {
    sb32 is_allow_interrupt;
    s32 fall_wait;
} ftCommonCliffWaitStatusVars;

typedef struct ftCommonCliffMotionStatusVars {
    s32 status_id;
    s32 cliff_id;
} ftCommonCliffMotionStatusVars;

typedef struct ftCommonLiftStatusVars {
    s32 turn_tics;
} ftCommonLiftStatusVars;

typedef struct ftCommonItemThrowStatusVars {
    s32 turn_rotate_step;
    s32 turn_tics;
    s32 turn_invert_lr_wait;
    f32 throw_vel;
    s32 throw_angle;
    f32 throw_damage;
} ftCommonItemThrowStatusVars;

typedef struct ftCommonItemSwingStatusVars {
    s32 harisen_scale_reset_wait;
} ftCommonItemSwingStatusVars;

typedef struct ftCommonFireFlowerStatusVars {
    s32 flame_vel_index;
    s32 ammo_sub;
    s32 effect_make_int;
    s32 ammo_fire_count;
    sb32 is_release;
    s32 release_lag;
} ftCommonFireFlowerStatusVars;

typedef struct ftCommonHammerStatusVars {
    union {
        f32 landing_anim_frame;
        f32 jump_force;
    };
    f32 kneebend_anim_frame;
    s32 input_source;
    sb32 is_shorthop;
} ftCommonHammerStatusVars;

typedef struct ftCommonGuardStatusVars {
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
} ftCommonGuardStatusVars;

typedef struct ftCommonEscapeStatusVars {
    s32 itemthrow_buffer_tics;
} ftCommonEscapeStatusVars;

typedef struct ftCommonCatchStatusVars {
    f32 catch_pull_frame_begin;
    f32 catch_pull_anim_frames;
    u8 filler_0x8[0xB3C - 0xB20];
    u32 unk_0x24;
} ftCommonCatchStatusVars;

typedef struct ftCommonCatchWaITStatusVars {
    s32 throw_wait;
} ftCommonCatchWaITStatusVars;

typedef struct ftCommonCaptureStatusVars {
    sb32 is_goto_pulled_wait;
} ftCommonCaptureStatusVars;

typedef struct ftCommonThrownStatusVars {
    s32 status_id;
} ftCommonThrownStatusVars;

typedef struct ftCommonCaptureKirbyStatusVars {
    sb16 is_goto_capturewait;
    s16 lr;
    sb16 is_kirby;
} ftCommonCaptureKirbyStatusVars;

typedef struct ftCommonCaptureYoshiStatusVars {
    GObj *effect_gobj;
    s16 stage;
    s16 breakout_wait;
    s8 lr;
    sb8 is_damagefloor;
} ftCommonCaptureYoshiStatusVars;

typedef struct ftCommonCaptureCaptainStatusVars {
    u16 capture_flag;
} ftCommonCaptureCaptainStatusVars;

typedef struct ftCommonThrowFStatusVars {
    union {
        f32 landing_anim_frame;
        f32 jump_force;
    };
    f32 kneebend_anim_frame;
    s32 input_source;
    sb32 is_shorthop;
} ftCommonThrowFStatusVars;

typedef struct ftCommonThrowFFStatusVars {
    sb32 is_turn;
    s32 turn_tics;
} ftCommonThrowFFStatusVars;

typedef struct ftCommonThrowFDamageStatusVars {
    s32 hitstun_tics;
    s32 dust_effect_int;
    f32 public_knockback;
    u16 coll_mask;
    u8 filler_0xE[0x28 - 0xE];
    s32 unk_index;
    s32 status_id;
    sb32 is_knockback_over;
} ftCommonThrowFDamageStatusVars;

typedef struct ftCommonAttack1StatusVars {
    sb32 is_goto_followup;
    s32 interrupt_catch_timer;
} ftCommonAttack1StatusVars;

typedef struct ftCommonAttack100StatusVars {
    sb32 is_anim_end;
    sb32 is_goto_loop;
} ftCommonAttack100StatusVars;

typedef struct ftCommonAttackLw3StatusVars {
    sb32 is_goto_attacklw3;
} ftCommonAttackLw3StatusVars;

typedef struct ftCommonAttack4StatusVars {
    s32 gfx_id;
    s32 unk_0x4;
    sb32 is_goto_attacklw4;
    s32 unk_0xC;
    s32 lr;
} ftCommonAttack4StatusVars;

typedef struct ftCommonAttackAirStatusVars {
    s32 rehit_timer;
} ftCommonAttackAirStatusVars;

typedef union FTCommonStatusVars {
    ftCommonDeadStatusVars dead;
    ftCommonRebirthStatusVars rebirth;
    ftCommonSleepStatusVars sleep;
    ftCommonEntryStatusVars entry;
    ftCommonTurnStatusVars turn;
    ftCommonKneeBendStatusVars kneebend;
    ftCommonJumpAerialStatusVars jumpaerial;
    ftCommonDamageStatusVars damage;
    ftCommonSquatStatusVars squat;
    ftCommonDokanStatusVars dokan;
    ftCommonLandingStatusVars landing;
    ftCommonFallSpecialStatusVars fallspecial;
    ftCommonTwisterStatusVars twister;
    ftCommonTaruCannStatusVars tarucann;
    ftCommonDownWaITStatusVars downwait;
    ftCommonDownBounceStatusVars downbounce;
    ftCommonReboundStatusVars rebound;
    ftCommonCliffWaitStatusVars cliffwait;
    ftCommonCliffMotionStatusVars cliffmotion;
    ftCommonLiftStatusVars lift;
    ftCommonItemThrowStatusVars itemthrow;
    ftCommonItemSwingStatusVars itemswing;
    ftCommonFireFlowerStatusVars fireflower;
    ftCommonHammerStatusVars hammer;
    ftCommonGuardStatusVars guard;
    ftCommonEscapeStatusVars escape;
    ftCommonCatchStatusVars catchmain;
    ftCommonCatchWaITStatusVars catchwait;
    ftCommonCaptureStatusVars capture;
    ftCommonThrownStatusVars thrown;
    ftCommonCaptureKirbyStatusVars capturekirby;
    ftCommonCaptureYoshiStatusVars captureyoshi;
    ftCommonCaptureCaptainStatusVars capturecaptain;
    ftCommonThrowFStatusVars throwf;
    ftCommonThrowFFStatusVars throwff;
    ftCommonThrowFDamageStatusVars throwfdamage;
    ftCommonAttack1StatusVars attack1;
    ftCommonAttack100StatusVars attack100;
    ftCommonAttackLw3StatusVars attacklw3;
    ftCommonAttack4StatusVars attack4;
    ftCommonAttackAirStatusVars attackair;
} FTCommonStatusVars;

typedef struct FTMarioPassiveVars { sb32 is_expend_tornado; } FTMarioPassiveVars;
typedef struct FTDonkeyPassiveVars { s32 charge_level; } FTDonkeyPassiveVars;
typedef struct FTSamusPassiveVars { s32 charge_level; s32 charge_recoil; } FTSamusPassiveVars;
typedef struct FTLinkPassiveVars { GObj *boomerang_gobj; } FTLinkPassiveVars;
typedef struct FTCaptainPassiveVars { s32 falcon_punch_unk; } FTCaptainPassiveVars;
typedef struct FTKirbyPassiveVars {
    s32 copy_id;
    s32 copysamus_charge_level;
    s32 copysamus_charge_recoil;
    s32 copydonkey_charge_level;
    s32 copycaptain_falcon_punch_unk;
    s32 copypurin_unk;
    sb32 is_ignore_losecopy;
    GObj *copylink_boomerang_gobj;
} FTKirbyPassiveVars;
typedef struct FTPikachuPassiveVars { sb32 is_thunder_destroy; } FTPikachuPassiveVars;
typedef struct FTPurinPassiveVars { u32 unk_0x0; } FTPurinPassiveVars;

#define WPPKTHUNDER_PARTS_COUNT 5
#define FTNESS_PKTHUNDER_TRAIL_POS_COUNT \
    ((WPPKTHUNDER_PARTS_COUNT * 2) + \
     ((WPPKTHUNDER_PARTS_COUNT * 2) / WPPKTHUNDER_PARTS_COUNT))

typedef struct FTNessPassiveVars {
    sb32 is_thunder_destroy;
    s32 pkthunder_trail_id;
    s16 pkthunder_trail_x[FTNESS_PKTHUNDER_TRAIL_POS_COUNT];
    s16 pkthunder_trail_y[FTNESS_PKTHUNDER_TRAIL_POS_COUNT];
} FTNessPassiveVars;

typedef struct ftBossInfo {
    GObj *target_gobj;
    s32 current_line_id;
    s32 default_line_id;
    f32 wait_div;
    u32 wait_timer;
    s8 status_id;
    s8 status_id_random;
    u8 status_id_guard;
} ftBossInfo;

typedef struct FTBossPassiveVars {
    ftBossInfo *p, s;
} FTBossPassiveVars;

typedef union FTPassiveVars {
    FTMarioPassiveVars mario;
    FTDonkeyPassiveVars donkey;
    FTSamusPassiveVars samus;
    FTLinkPassiveVars link;
    FTCaptainPassiveVars captain;
    FTKirbyPassiveVars kirby;
    FTPikachuPassiveVars pikachu;
    FTPurinPassiveVars purin;
    FTNessPassiveVars ness;
    FTBossPassiveVars boss;
} FTPassiveVars;

typedef struct ftMarioSpecialHiStatusVars { sb32 is_air_bool; } ftMarioSpecialHiStatusVars;
typedef struct ftMarioSpecialLwStatusVars { f32 friction; s32 dust_effect_int; } ftMarioSpecialLwStatusVars;
typedef union FTMarioStatusVars {
    ftMarioSpecialHiStatusVars specialhi;
    ftMarioSpecialLwStatusVars speciallw;
} FTMarioStatusVars;

typedef struct ftFoxSpecialHiStatusVars {
    s32 launch_delay;
    s32 gravity_delay;
    f32 angle;
    s32 anim_frames;
    s32 decelerate_wait;
    s32 pass_timer;
} ftFoxSpecialHiStatusVars;
typedef struct ftFoxSpecialLwStatusVars {
    s32 release_lag;
    s32 turn_tics;
    sb32 is_release;
    GObj *effect_gobj;
    s32 gravity_delay;
} ftFoxSpecialLwStatusVars;
typedef union FTFoxStatusVars {
    ftFoxSpecialHiStatusVars specialhi;
    ftFoxSpecialLwStatusVars speciallw;
} FTFoxStatusVars;

typedef struct ftDonkeySpecialNStatusVars {
    sb32 is_release;
    s32 charge_level;
    sb32 is_charging;
    sb32 is_cancel;
} ftDonkeySpecialNStatusVars;
typedef struct ftDonkeySpecialHiStatusVars { s32 unk_0x0; } ftDonkeySpecialHiStatusVars;
typedef struct ftDonkeySpecialLwStatusVars { sb32 is_loop; } ftDonkeySpecialLwStatusVars;
typedef union FTDonkeyStatusVars {
    ftDonkeySpecialNStatusVars specialn;
    ftDonkeySpecialHiStatusVars specialhi;
    ftDonkeySpecialLwStatusVars speciallw;
} FTDonkeyStatusVars;

typedef struct ftSamusSpecialNStatusVars {
    sb32 is_release;
    s32 charge_int;
    GObj *charge_gobj;
} ftSamusSpecialNStatusVars;
typedef struct ftSamusSpecialLwStatusVars { sb32 unused; } ftSamusSpecialLwStatusVars;
typedef union FTSamusStatusVars {
    ftSamusSpecialNStatusVars specialn;
    ftSamusSpecialLwStatusVars speciallw;
} FTSamusStatusVars;

typedef struct ftLinkSpecialNStatusVars { sb32 is_smash; } ftLinkSpecialNStatusVars;
typedef struct ftLinkSpecialHiStatusVars { GObj *spin_attack_gobj; } ftLinkSpecialHiStatusVars;
typedef union FTLinkStatusVars {
    ftLinkSpecialNStatusVars specialn;
    ftLinkSpecialHiStatusVars specialhi;
} FTLinkStatusVars;

typedef struct ftYoshiSpecialNStatusVars { u32 unk_0x0; u16 unk_0x4; } ftYoshiSpecialNStatusVars;
typedef struct ftYoshiSpecialHiStatusVars { GObj *egg_gobj; s16 throw_force; } ftYoshiSpecialHiStatusVars;
typedef union FTYoshiStatusVars {
    ftYoshiSpecialHiStatusVars specialhi;
} FTYoshiStatusVars;

typedef struct ftCaptainSpecialHiStatusVars { u16 flags; Vec3f vel; } ftCaptainSpecialHiStatusVars;
typedef struct ftCaptainSpecialLwStatusVars { u16 scale_apply_timer; f32 vel_scale; } ftCaptainSpecialLwStatusVars;
typedef union FTCaptainStatusVars {
    ftCaptainSpecialHiStatusVars specialhi;
    ftCaptainSpecialLwStatusVars speciallw;
} FTCaptainStatusVars;

typedef struct ftKirbySpecialNStatusVars { s16 copy_id; s16 release_lag; Vec3f dist; } ftKirbySpecialNStatusVars;
typedef struct ftKirbySpecialLwStatusVars { s16 duration; s16 unk_0x2; s16 colanim_id; } ftKirbySpecialLwStatusVars;
typedef struct ftKirbyCopyDonkeySpecialNStatusVars {
    sb32 is_release;
    s32 charge_level;
    sb32 is_charging;
    sb32 is_cancel;
} ftKirbyCopyDonkeySpecialNStatusVars;
typedef struct ftKirbyCopySamusSpecialNStatusVars {
    sb32 is_release;
    s32 charge_int;
    GObj *charge_gobj;
} ftKirbyCopySamusSpecialNStatusVars;
typedef struct ftKirbyCopyLinkSpecialNStatusVars { sb32 is_smash; } ftKirbyCopyLinkSpecialNStatusVars;
typedef union FTKirbyStatusVars {
    ftKirbySpecialNStatusVars specialn;
    ftKirbySpecialLwStatusVars speciallw;
    ftKirbyCopyDonkeySpecialNStatusVars copydonkey_specialn;
    ftKirbyCopySamusSpecialNStatusVars copysamus_specialn;
    ftKirbyCopyLinkSpecialNStatusVars copylink_specialn;
} FTKirbyStatusVars;

typedef struct ftPikachuSpecialHiStatusVars {
    s32 anim_frames;
    sb32 is_subsequent_zip;
    u32 unk_0x8;
    Vec2i stick_range;
    s32 pass_timer;
    f32 vel_x_bak;
    f32 vel_y_bak;
    f32 vel_ground_bak;
} ftPikachuSpecialHiStatusVars;
typedef struct ftPikachuSpecialLwStatusVars { GObj *thunder_gobj; } ftPikachuSpecialLwStatusVars;
typedef union FTPikachuStatusVars {
    ftPikachuSpecialHiStatusVars specialhi;
    ftPikachuSpecialLwStatusVars speciallw;
} FTPikachuStatusVars;

typedef struct ftNessSpecialHiStatusVars {
    s32 pkjibaku_delay;
    s32 pkthunder_end_delay;
    s32 pkthunder_gravity_delay;
    GObj *pkthunder_gobj;
    s32 pkjibaku_anim_length;
    f32 pkjibaku_angle;
    Vec3f pkthunder_pos;
} ftNessSpecialHiStatusVars;
typedef struct ftNessSpecialLwStatusVars {
    s32 release_lag;
    sb32 is_release;
    s32 gravity_delay;
} ftNessSpecialLwStatusVars;
typedef union FTNessStatusVars {
    ftNessSpecialHiStatusVars specialhi;
    ftNessSpecialLwStatusVars speciallw;
} FTNessStatusVars;

typedef struct ftBossWaITStatusVars { Vec3f pos; } ftBossWaITStatusVars;
typedef struct ftBossMoveStatusVars { void (*proc_setstatus)(GObj *); Vec3f vel; f32 magnitude; } ftBossMoveStatusVars;
typedef struct ftBossOkuhikoukiStatusVars { Vec3f pos; } ftBossOkuhikoukiStatusVars;
typedef struct ftBossGootsubusuStatusVars { s16 wait_timer; f32 edgeleft_pos_x; f32 edgeright_pos_x; } ftBossGootsubusuStatusVars;
typedef struct ftBossTsutsukuStatusVars { s16 wait_timer; } ftBossTsutsukuStatusVars;
typedef struct ftBossDrillStatusVars { s16 follow_timer; f32 edgeleft_pos_x; f32 edgeright_pos_x; } ftBossDrillStatusVars;
typedef struct ftBossYubideppouStatusVars { s16 wait_timer; s16 bullet_count; u8 shoot_timer; } ftBossYubideppouStatusVars;
typedef struct ftBossOkupunchStatusVars { Vec3f pos; } ftBossOkupunchStatusVars;
typedef struct ftBossOkutsubushiStatusVars { Vec3f pos; u16 follow_timer; } ftBossOkutsubushiStatusVars;
typedef struct ftBossDeadStatusVars { s16 dead_timer; } ftBossDeadStatusVars;
typedef union FTBossStatusVars {
    ftBossWaITStatusVars wait;
    ftBossMoveStatusVars move;
    ftBossOkuhikoukiStatusVars okuhikouki;
    ftBossGootsubusuStatusVars gootsubu;
    ftBossTsutsukuStatusVars tsutsuku;
    ftBossDrillStatusVars drill;
    ftBossYubideppouStatusVars yubideppou;
    ftBossOkupunchStatusVars okupunch;
    ftBossOkutsubushiStatusVars okutsubushi;
    ftBossDeadStatusVars dead;
} FTBossStatusVars;

typedef union FTStatusVars {
    FTCommonStatusVars common;
    FTMarioStatusVars mario;
    FTFoxStatusVars fox;
    FTDonkeyStatusVars donkey;
    FTSamusStatusVars samus;
    FTLinkStatusVars link;
    FTYoshiStatusVars yoshi;
    FTCaptainStatusVars captain;
    FTKirbyStatusVars kirby;
    FTPikachuStatusVars pikachu;
    FTNessStatusVars ness;
    FTBossStatusVars boss;
} FTStatusVars;

typedef struct ftKirbyAttack100Effect {
    Vec3f offset;
    f32 rotate;
    f32 vel;
    f32 add;
} ftKirbyAttack100Effect;

typedef struct FTOpeningDesc {
    s32 motion_id;
    void (*proc_update)(GObj *);
} FTOpeningDesc;

typedef struct FTHiddenPart {
    s32 root_joint_id;
    s32 parent_joint_id;
    s32 partindex_0x8;
    s32 joint_kind;
} FTHiddenPart;

typedef struct FTAfterImage {
    s16 translate_x;
    s16 translate_y;
    s16 translate_z;
    Vec3f vec;
} FTAfterImage;

typedef struct FTModelPartStatus {
    s8 modelpart_id_base;
    s8 modelpart_id_curr;
} FTModelPartStatus;

typedef struct FTTexturePartStatus {
    s8 texture_id_base;
    s8 texture_id_curr;
} FTTexturePartStatus;

typedef struct FTTexturePart {
    u8 joint_id;
    u8 detail[2];
} FTTexturePart;

typedef struct FTTexturePartContainer {
    FTTexturePart textureparts[2];
} FTTexturePartContainer;

typedef struct FTStruct {
    struct FTStruct *next;
    GObj *fighter_gobj;
    s32 fkind;
    u8 team;
    u8 player;
    u8 detail_curr;
    u8 detail_base;
    u8 costume;
    u8 shade;
    u8 handicap;
    u8 level;
    s8 stock_count;
    u8 team_order;
    u8 dl_link;
    s32 player_num;
    u32 status_total_tics;
    s32 pkind;
    s32 status_id;
    s32 motion_id;
    s32 percent_damage;
    s32 damage_resist;
    s32 shield_health;
    f32 unk_ft_0x38;
    s32 unk_ft_0x3C;
    u32 hitlag_tics;
    s32 lr;
    FTPhysics physics;
    MPCollData coll_data;
    u8 jumps_used;
    u8 unk_ft_0x149;
    sb32 ga;
    f32 attack1_followup_frames;
    s32 attack1_status_id;
    s32 attack1_input_count;
    s32 cliffcatch_wait;
    s32 tics_since_last_z;
    s32 acid_wait;
    s32 twister_wait;
    s32 tarucann_wait;
    s32 damagefloor_wait;
    s32 playertag_wait;
    s32 card_anim_frame_id;
    FTMotionVars motion_vars;
    ub32 is_attack_active : 1;
    ub32 is_hitstatus_nodamage : 1;
    ub32 is_damage_coll_modify : 1;
    ub32 is_modelpart_modify : 1;
    ub32 is_texturepart_modify : 1;
    ub32 is_reflect : 1;
    s32 reflect_lr : 2;
    ub32 is_absorb : 1;
    s32 absorb_lr : 2;
    ub32 is_goto_attack100 : 1;
    ub32 is_fastfall : 1;
    ub32 is_magnify_show : 1;
    ub32 is_limit_map_bounds : 1;
    ub32 is_invisible : 1;
    ub32 is_shadow_hide : 1;
    ub32 is_rebirth : 1;
    ub32 is_magnify_ignore : 1;
    ub32 is_playertag_hide : 1;
    ub32 is_playertag_bossend : 1;
    ub32 is_effect_skip : 1;
    u32 effect_joint_array_id : 4;
    ub32 is_shield : 1;
    ub32 is_effect_attach : 1;
    ub32 is_jostle_ignore : 1;
    ub32 is_have_translate_scale : 1;
    ub32 is_control_disable : 1;
    ub32 is_hitstun : 1;
    u32 slope_contour : 3;
    ub32 is_use_animlocks : 1;
    ub32 is_muted : 1;
    ub32 unk_ft_0x190_b5 : 1;
    ub32 is_item_show : 1;
    ub32 is_cliff_hold : 1;
    ub32 is_events_forward : 1;
    ub32 is_ghost : 1;
    ub32 is_damage_resist : 1;
    ub32 is_menu_ignore : 1;
    u32 camera_mode : 4;
    ub32 is_special_interrupt : 1;
    ub32 is_ignore_dead : 1;
    ub32 is_catchstatus : 1;
    ub32 is_catch_or_capture : 1;
    ub32 is_use_fogcolor : 1;
    ub32 is_shield_catch : 1;
    ub32 is_knockback_paused : 1;
    u8 capture_immune_mask;
    u8 catch_mask;
    FTAnimDesc anim_desc;
    Vec3f anim_vel;
    Vec2f magnify_pos;
    FTInput input;
    FTComputer computer;
    Vec2f damage_coll_size;
    u8 tap_stick_x;
    u8 tap_stick_y;
    u8 hold_stick_x;
    u8 hold_stick_y;
    s32 breakout_wait;
    s8 breakout_lr;
    s8 breakout_ud;
    u8 shuffle_frame_index;
    u8 shuffle_index_max;
    ub8 is_shuffle_electric;
    u16 shuffle_tics;
    GObj *throw_gobj;
    s32 throw_fkind;
    u8 throw_team;
    u8 throw_player;
    s32 throw_player_num;
    u32 motion_attack_id;
    u16 motion_count;
    GMStatFlags stat_flags;
    u16 stat_count;
    FTAttackColl attack_colls[4];
    s32 invincible_tics;
    s32 intangible_tics;
    s32 special_hitstatus;
    s32 star_invincible_tics;
    s32 star_hitstatus;
    s32 hitstatus;
    FTDamageColl damage_colls[11];
    f32 unk_ft_0x7A0;
    f32 hitlag_mul;
    f32 shield_heal_wait;
    s32 unk_ft_0x7AC;
    s32 attack_damage;
    f32 attack_knockback;
    u16 attack_count;
    s32 attack_shield_push;
    f32 attack_rebound;
    s32 hit_lr;
    s32 shield_damage;
    s32 shield_damage_total;
    s32 shield_lr;
    s32 shield_player;
    s32 reflect_damage;
    s32 damage_lag;
    f32 damage_knockback;
    f32 knockback_resist_passive;
    f32 knockback_resist_status;
    f32 damage_knockback_stack;
    s32 damage_queue;
    s32 damage_angle;
    s32 damage_element;
    s32 damage_lr;
    s32 damage_index;
    s32 damage_joint_id;
    s32 damage_player_num;
    s32 damage_player;
    u16 damage_count;
    s32 damage_kind;
    s32 damage_heal;
    f32 damage_mul;
    s32 damage_object_class;
    s32 damage_object_kind;
    GMStatFlags damage_stat_flags;
    u16 damage_stat_count;
    f32 public_knockback;
    GObj *search_gobj;
    f32 search_gobj_dist;
    void (*proc_catch)(GObj *);
    void (*proc_capture)(GObj *, GObj *);
    GObj *catch_gobj;
    GObj *capture_gobj;
    FTThrowHitDesc *throw_desc;
    GObj *item_gobj;
    FTSpecialColl *special_coll;
    Vec3f entry_pos;
    f32 camera_zoom_frame;
    f32 camera_zoom_range;
    FTMotionScript motion_scripts[2][2];
    DObj *joints[FTPARTS_JOINT_NUM_MAX];
    FTModelPartStatus modelpart_status[FTPARTS_JOINT_NUM_MAX -
                                       nFTPartsJointCommonStart];
    FTTexturePartStatus texturepart_status[2];
    FTData *data;
    FTAttributes *attr;
    void **figatree;
    void **figatree_heap;
    void (*proc_update)(GObj *);
    void (*proc_accessory)(GObj *);
    void (*proc_interrupt)(GObj *);
    void (*proc_physics)(GObj *);
    void (*proc_map)(GObj *);
    void (*proc_slope)(GObj *);
    void (*proc_damage)(GObj *);
    void (*proc_trap)(GObj *);
    void (*proc_shield)(GObj *);
    void (*proc_hit)(GObj *);
    void (*proc_passive)(GObj *);
    void (*proc_lagupdate)(GObj *);
    void (*proc_lagstart)(GObj *);
    void (*proc_lagend)(GObj *);
    void (*proc_status)(GObj *);
    alSoundEffect *p_sfx;
    u16 sfx_id;
    alSoundEffect *p_voice;
    u16 voice_id;
    alSoundEffect *p_loop_sfx;
    u16 loop_sfx_id;
    GMColAnim colanim;
    SYColorRGBA fog_color;
    SYColorRGBA shade_color;
    FTKey key;
    struct {
        ub8 is_itemswing;
        s8 drawstatus;
        u8 desc_id;
        FTAfterImage desc[3];
    } afterimage;
    FTPassiveVars passive_vars;
    s32 hammer_tics;
    FTStatusVars status_vars;
    s32 display_mode;

    /* Port-only extension: keep all non-BattleShip fields after source layout. */
    s32 damage;
    s32 status_prev;
    f32 motion_frame;
    f32 anim_frame;
    f32 anim_speed;
    Vec3f vel_air;
    Vec3f vel_ground;
    Vec3f vel_push;
    s32 motion_script_id;
    s32 status_attack_id;
    s32 status_is_smash;
    s32 status_is_projectile;
    u32 status_flags;
    s32 stat_attack_id;
    sb32 is_wait_status_setup;
    sb32 is_wait_motion_setup;
    u32 nds_magic;
    u32 nds_slot;
    u32 nds_joint_count;
    u32 nds_common_joint_count;
    u32 nds_init_mask;
    u32 nds_init_floor_project_attempted;
    u32 nds_init_floor_project_result;
} FTStruct;

#define NDS_FTSTRUCT_SOURCE_SIZE 2896u
#define NDS_FTSTRUCT_LAYOUT_SIZE 3012u
#define NDS_FTSTRUCT_OFF_NEXT 0u
#define NDS_FTSTRUCT_OFF_FIGHTER_GOBJ 4u
#define NDS_FTSTRUCT_OFF_FKIND 8u
#define NDS_FTSTRUCT_OFF_TEAM 12u
#define NDS_FTSTRUCT_OFF_PLAYER 13u
#define NDS_FTSTRUCT_OFF_STATUS_ID 36u
#define NDS_FTSTRUCT_OFF_MOTION_ID 40u
#define NDS_FTSTRUCT_OFF_PERCENT_DAMAGE 44u
#define NDS_FTSTRUCT_OFF_HITLAG_TICS 64u
#define NDS_FTSTRUCT_OFF_PHYSICS 72u
#define NDS_FTSTRUCT_OFF_COLL_DATA 120u
#define NDS_FTSTRUCT_OFF_JUMPS_USED 328u
#define NDS_FTSTRUCT_OFF_GA 332u
#define NDS_FTSTRUCT_OFF_MOTION_VARS 380u
#define NDS_FTSTRUCT_OFF_INPUT 432u
#define NDS_FTSTRUCT_OFF_COMPUTER 460u
#define NDS_FTSTRUCT_OFF_MOTION_ATTACK_ID 648u
#define NDS_FTSTRUCT_OFF_STAT_FLAGS 654u
#define NDS_FTSTRUCT_OFF_ATTACK_COLLS 660u
#define NDS_FTSTRUCT_OFF_DAMAGE_COLLS 1468u
#define NDS_FTSTRUCT_OFF_DAMAGE_STAT_FLAGS 2088u
#define NDS_FTSTRUCT_OFF_MOTION_SCRIPTS 2152u
#define NDS_FTSTRUCT_OFF_JOINTS 2280u
#define NDS_FTSTRUCT_OFF_MODELPART_STATUS 2428u
#define NDS_FTSTRUCT_OFF_DATA 2500u
#define NDS_FTSTRUCT_OFF_ATTR 2504u
#define NDS_FTSTRUCT_OFF_FIGATREE 2508u
#define NDS_FTSTRUCT_OFF_FIGATREE_HEAP 2512u
#define NDS_FTSTRUCT_OFF_PROC_UPDATE 2516u
#define NDS_FTSTRUCT_OFF_PROC_ACCESSORY 2520u
#define NDS_FTSTRUCT_OFF_PROC_INTERRUPT 2524u
#define NDS_FTSTRUCT_OFF_PROC_PHYSICS 2528u
#define NDS_FTSTRUCT_OFF_PROC_MAP 2532u
#define NDS_FTSTRUCT_OFF_PROC_DAMAGE 2540u
#define NDS_FTSTRUCT_OFF_PROC_LAGUPDATE 2560u
#define NDS_FTSTRUCT_OFF_PROC_LAGSTART 2564u
#define NDS_FTSTRUCT_OFF_PROC_STATUS 2572u
#define NDS_FTSTRUCT_OFF_FOG_COLOR 2700u
#define NDS_FTSTRUCT_OFF_KEY 2708u
#define NDS_FTSTRUCT_OFF_PASSIVE_VARS 2780u
#define NDS_FTSTRUCT_OFF_HAMMER_TICS 2836u
#define NDS_FTSTRUCT_OFF_STATUS_VARS 2840u
#define NDS_FTSTRUCT_OFF_DISPLAY_MODE 2892u
#define NDS_FTSTRUCT_OFF_EXT_DAMAGE 2896u
#define NDS_FTSTRUCT_OFF_EXT_STATUS_PREV 2900u
#define NDS_FTSTRUCT_OFF_EXT_MOTION_FRAME 2904u
#define NDS_FTSTRUCT_OFF_EXT_ANIM_FRAME 2908u
#define NDS_FTSTRUCT_OFF_EXT_ANIM_SPEED 2912u
#define NDS_FTSTRUCT_OFF_EXT_VEL_AIR 2916u
#define NDS_FTSTRUCT_OFF_EXT_MOTION_SCRIPT_ID 2952u
#define NDS_FTSTRUCT_OFF_EXT_NDS_MAGIC 2984u

#define NDS_FTSTRUCT_ASSERT_OFF(field, expected)                           \
    _Static_assert(offsetof(FTStruct, field) == (expected),                 \
                   "FTStruct " #field " offset changed")

_Static_assert(sizeof(FTStruct) == NDS_FTSTRUCT_LAYOUT_SIZE,
               "FTStruct size changed");
NDS_FTSTRUCT_ASSERT_OFF(damage, NDS_FTSTRUCT_SOURCE_SIZE);
NDS_FTSTRUCT_ASSERT_OFF(next, NDS_FTSTRUCT_OFF_NEXT);
NDS_FTSTRUCT_ASSERT_OFF(fighter_gobj, NDS_FTSTRUCT_OFF_FIGHTER_GOBJ);
NDS_FTSTRUCT_ASSERT_OFF(fkind, NDS_FTSTRUCT_OFF_FKIND);
NDS_FTSTRUCT_ASSERT_OFF(team, NDS_FTSTRUCT_OFF_TEAM);
NDS_FTSTRUCT_ASSERT_OFF(player, NDS_FTSTRUCT_OFF_PLAYER);
NDS_FTSTRUCT_ASSERT_OFF(status_id, NDS_FTSTRUCT_OFF_STATUS_ID);
NDS_FTSTRUCT_ASSERT_OFF(motion_id, NDS_FTSTRUCT_OFF_MOTION_ID);
NDS_FTSTRUCT_ASSERT_OFF(percent_damage, NDS_FTSTRUCT_OFF_PERCENT_DAMAGE);
NDS_FTSTRUCT_ASSERT_OFF(hitlag_tics, NDS_FTSTRUCT_OFF_HITLAG_TICS);
NDS_FTSTRUCT_ASSERT_OFF(physics, NDS_FTSTRUCT_OFF_PHYSICS);
NDS_FTSTRUCT_ASSERT_OFF(coll_data, NDS_FTSTRUCT_OFF_COLL_DATA);
NDS_FTSTRUCT_ASSERT_OFF(jumps_used, NDS_FTSTRUCT_OFF_JUMPS_USED);
NDS_FTSTRUCT_ASSERT_OFF(ga, NDS_FTSTRUCT_OFF_GA);
NDS_FTSTRUCT_ASSERT_OFF(motion_vars, NDS_FTSTRUCT_OFF_MOTION_VARS);
NDS_FTSTRUCT_ASSERT_OFF(input, NDS_FTSTRUCT_OFF_INPUT);
NDS_FTSTRUCT_ASSERT_OFF(computer, NDS_FTSTRUCT_OFF_COMPUTER);
NDS_FTSTRUCT_ASSERT_OFF(motion_attack_id,
                        NDS_FTSTRUCT_OFF_MOTION_ATTACK_ID);
NDS_FTSTRUCT_ASSERT_OFF(stat_flags, NDS_FTSTRUCT_OFF_STAT_FLAGS);
NDS_FTSTRUCT_ASSERT_OFF(attack_colls, NDS_FTSTRUCT_OFF_ATTACK_COLLS);
NDS_FTSTRUCT_ASSERT_OFF(damage_colls, NDS_FTSTRUCT_OFF_DAMAGE_COLLS);
NDS_FTSTRUCT_ASSERT_OFF(damage_stat_flags,
                        NDS_FTSTRUCT_OFF_DAMAGE_STAT_FLAGS);
NDS_FTSTRUCT_ASSERT_OFF(motion_scripts, NDS_FTSTRUCT_OFF_MOTION_SCRIPTS);
NDS_FTSTRUCT_ASSERT_OFF(joints, NDS_FTSTRUCT_OFF_JOINTS);
NDS_FTSTRUCT_ASSERT_OFF(modelpart_status,
                        NDS_FTSTRUCT_OFF_MODELPART_STATUS);
NDS_FTSTRUCT_ASSERT_OFF(data, NDS_FTSTRUCT_OFF_DATA);
NDS_FTSTRUCT_ASSERT_OFF(attr, NDS_FTSTRUCT_OFF_ATTR);
NDS_FTSTRUCT_ASSERT_OFF(figatree, NDS_FTSTRUCT_OFF_FIGATREE);
NDS_FTSTRUCT_ASSERT_OFF(figatree_heap, NDS_FTSTRUCT_OFF_FIGATREE_HEAP);
NDS_FTSTRUCT_ASSERT_OFF(proc_update, NDS_FTSTRUCT_OFF_PROC_UPDATE);
NDS_FTSTRUCT_ASSERT_OFF(proc_accessory, NDS_FTSTRUCT_OFF_PROC_ACCESSORY);
NDS_FTSTRUCT_ASSERT_OFF(proc_interrupt, NDS_FTSTRUCT_OFF_PROC_INTERRUPT);
NDS_FTSTRUCT_ASSERT_OFF(proc_physics, NDS_FTSTRUCT_OFF_PROC_PHYSICS);
NDS_FTSTRUCT_ASSERT_OFF(proc_map, NDS_FTSTRUCT_OFF_PROC_MAP);
NDS_FTSTRUCT_ASSERT_OFF(proc_damage, NDS_FTSTRUCT_OFF_PROC_DAMAGE);
NDS_FTSTRUCT_ASSERT_OFF(proc_lagupdate, NDS_FTSTRUCT_OFF_PROC_LAGUPDATE);
NDS_FTSTRUCT_ASSERT_OFF(proc_lagstart, NDS_FTSTRUCT_OFF_PROC_LAGSTART);
NDS_FTSTRUCT_ASSERT_OFF(proc_status, NDS_FTSTRUCT_OFF_PROC_STATUS);
NDS_FTSTRUCT_ASSERT_OFF(fog_color, NDS_FTSTRUCT_OFF_FOG_COLOR);
NDS_FTSTRUCT_ASSERT_OFF(key, NDS_FTSTRUCT_OFF_KEY);
NDS_FTSTRUCT_ASSERT_OFF(passive_vars, NDS_FTSTRUCT_OFF_PASSIVE_VARS);
NDS_FTSTRUCT_ASSERT_OFF(hammer_tics, NDS_FTSTRUCT_OFF_HAMMER_TICS);
NDS_FTSTRUCT_ASSERT_OFF(status_vars, NDS_FTSTRUCT_OFF_STATUS_VARS);
NDS_FTSTRUCT_ASSERT_OFF(display_mode, NDS_FTSTRUCT_OFF_DISPLAY_MODE);
NDS_FTSTRUCT_ASSERT_OFF(damage, NDS_FTSTRUCT_OFF_EXT_DAMAGE);
NDS_FTSTRUCT_ASSERT_OFF(status_prev, NDS_FTSTRUCT_OFF_EXT_STATUS_PREV);
NDS_FTSTRUCT_ASSERT_OFF(motion_frame, NDS_FTSTRUCT_OFF_EXT_MOTION_FRAME);
NDS_FTSTRUCT_ASSERT_OFF(anim_frame, NDS_FTSTRUCT_OFF_EXT_ANIM_FRAME);
NDS_FTSTRUCT_ASSERT_OFF(anim_speed, NDS_FTSTRUCT_OFF_EXT_ANIM_SPEED);
NDS_FTSTRUCT_ASSERT_OFF(vel_air, NDS_FTSTRUCT_OFF_EXT_VEL_AIR);
NDS_FTSTRUCT_ASSERT_OFF(motion_script_id,
                        NDS_FTSTRUCT_OFF_EXT_MOTION_SCRIPT_ID);
NDS_FTSTRUCT_ASSERT_OFF(nds_magic, NDS_FTSTRUCT_OFF_EXT_NDS_MAGIC);

#undef NDS_FTSTRUCT_ASSERT_OFF

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

struct FTAccessPart {
    s32 joint_id;
    Gfx *dl;
    MObjSub **mobjsubs;
    AObjEvent32 **costume_matanim_joints;
};

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
    f32 shield_break_vel_y;
    f32 shadow_size;
    f32 jostle_width;
    f32 jostle_x;
    sb32 is_metallic;
    f32 cam_offset_y;
    f32 closeup_camera_zoom;
    f32 camera_zoom;
    f32 camera_zoom_base;
    MPObjectColl map_coll;
    Vec2f cliffcatch_coll;
    u16 dead_fgm_ids[2];
    u16 deadup_sfx;
    u16 damage_sfx;
    u16 smash_sfx[3];
    FTItemPickup item_pickup;
    u16 itemthrow_vel_scale;
    u16 itemthrow_damage_scale;
    u16 heavyget_sfx;
    f32 halo_size;
    SYColorRGBA shade_color[3];
    SYColorRGBA fog_color;
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
    u32 *setup_parts;
    u32 *animlock;
    s32 effect_joint_ids[5];
    s32 cliff_status_ga[5];
    s32 unused_0x2CC;
    FTHiddenPart *hiddenparts;
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
    FTAccessPart *accesspart;
    FTTexturePartContainer *textureparts_container;
    s32 joint_itemheavy_id;
    FTThrownStatusArray *thrown_status;
    s32 joint_itemlight_id;
    void *sprites;
    void *skeleton;
} FTAttributes;

extern size_t gFTManagerFigatreeHeapSize;
extern FTDesc dFTManagerDefaultFighterDesc;
extern FTData *dFTManagerDataFiles[nFTKindEnumCount + 1];
extern f32 dSCSubsysFighterScales[];
extern GObj *gGCCommonLinks[GC_COMMON_MAX_LINKS];
extern FTOpeningDesc *D_ovl1_80390D20[];
extern FTOpeningDesc D_ovl1_80390BE8;
extern GMColDesc dGMColScriptsDescs[];

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
void ftParamPlayVoice(FTStruct *fp, u16 voice_id);
void ftParamPlayLoopSFX(FTStruct *fp, u16 sfx_id);
void ftParamStopLoopSFX(FTStruct *fp);
void ftParamMoveDLLink(GObj *fighter_gobj, u8 dl_link);
void ftParamSetHitStatusPartAll(GObj *fighter_gobj, s32 hitstatus);
void ftParamSetHitStatusPartID(GObj *fighter_gobj, s32 joint_id,
                               s32 hitstatus);
void ftParamSetHitStatusAll(GObj *fighter_gobj, s32 hitstatus);
void ftParamResetFighterDamageCollsAll(GObj *fighter_gobj);
void ftParamModifyDamageCollID(GObj *fighter_gobj, s32 joint_id,
                               Vec3f *offset, Vec3f *size);
void ftParamResetModelPartAll(GObj *fighter_gobj);
void ftParamHideModelPartAll(GObj *fighter_gobj);
void ftParamSetModelPartID(GObj *fighter_gobj, s32 joint_id,
                           s32 modelpart_id);
void ftParamSetModelPartDefaultID(GObj *fighter_gobj, s32 joint_id,
                                  s32 modelpart_id);
void ftParamSetModelPartDetailAll(GObj *fighter_gobj, u8 detail);
void ftParamSetTexturePartID(GObj *fighter_gobj, s32 texturepart_id,
                             s32 texture_id);
void ftParamResetTexturePartAll(GObj *fighter_gobj);
void ftParamResetStatUpdateColAnim(GObj *fighter_gobj);
void ftParamProcStopEffect(GObj *fighter_gobj);
void ftParamUpdateAnimKeys(GObj *fighter_gobj);
void ftParamsUpdateFighterPartsTransform(DObj *joint);
void ftParamsUpdateFighterPartsTransformAll(DObj *joint);
void ftParamTryUpdateItemMusic(void);
void ftParamKirbyTryMakeMapStarEffect(GObj *fighter_gobj);
void ftParamTryPlayItemMusic(s32 bgm_id);
void ftParamSetStarHitStatusInvincible(FTStruct *fp, s32 invincible_tics);
void ftParamSetHealDamage(FTStruct *fp, s32 heal);
s32 ftParamGetJointID(FTStruct *fp, s32 joint_id);
s32 ftParamGetCapturedDamage(FTStruct *fp, s32 damage);
void ftParamStopVoiceRunProcDamage(GObj *fighter_gobj);
void ftParamSetCaptureImmuneMask(FTStruct *fp, u8 mask);
void ftParamSetMotionID(FTStruct *fp, s32 attack_id);
void ftParamSetStatUpdate(FTStruct *fp, u16 stat_flags);
void ftParamUpdate1PGameAttackStats(FTStruct *fp, u16 stat_flags);
void ftParamSetAnimLocks(FTStruct *fp);
void ftParamClearAnimLocks(FTStruct *fp);
void ftParamLockPlayerControl(GObj *fighter_gobj);
void gmRumbleStopRumbleID(s32 player, s32 rumble_id);
void ftComputerSetupAll(GObj *fighter_gobj);
void ftComputerSetFighterDamageDetectSize(GObj *fighter_gobj);
void ftComputerProcessAll(GObj *fighter_gobj);
void ftKeyProcessKeyEvents(GObj *fighter_gobj);
sb32 ftCommonDeadCheckInterruptCommon(GObj *fighter_gobj);
void ftBossCommonUpdateDamageStats(GObj *fighter_gobj);
void ftBossCommonSetNextAttackWait(GObj *fighter_gobj);
void ftBossCommonSetDefaultLineID(GObj *fighter_gobj);
void ftCommonShieldBreakFlyCommonSetStatus(GObj *fighter_gobj);
void ftCommonGuardSetOffSetStatus(GObj *fighter_gobj);
void ftCommonShieldBreakFlyReflectorSetStatus(GObj *fighter_gobj);
void ftFoxSpecialLwHitSetStatus(GObj *fighter_gobj);
void ftNessSpecialLwProcAbsorb(GObj *fighter_gobj);
void ftHammerUpdateStats(GObj *fighter_gobj);
void func_ovl2_800EDBA4(DObj *joint);
extern f32 dMPCollisionMaterialFrictions[];
extern u8 gSC1PGameBonusStarCount;
extern u8 gSC1PGameBonusGiantImpact;
void ftManagerSetPrevPartsAlloc(FTParts *parts);
FTParts *ftManagerGetNextPartsAlloc(void);
void *lbRelocGetForceExternHeapFile(const void *file_id, void *heap);
void lbCommonInitDObj(DObj *dobj, u8 tk1, u8 tk2, u8 tk3, u8 arg4);
void lbCommonInitDObj3Transforms(DObj *dobj, u8 tk1, u8 tk2, u8 tk3);
void lbCommonSetupFighterPartsDObjs(DObj *root_dobj,
                                    FTCommonPartContainer *commonparts_container,
                                    s32 detail_curr, DObj **dobjs,
                                    u32 *setup_parts, u8 tk1, u8 tk2,
                                    u8 tk3, f32 anim_frame, u8 arg9);
void lbCommonAddMObjForFighterPartsDObj(DObj *dobj, MObjSub **mobjsubs,
                                        AObjEvent32 **costume_matanim_joints,
                                        AObjEvent32 **main_matanim_joints,
                                        f32 anim_frame);
DObj *gcAddDObjForGObj(GObj *gobj, void *dvar);
DObj *gcAddChildForDObj(DObj *dobj, void *dvar);
XObj *gcAddXObjForDObjFixed(DObj *dobj, u8 kind, u8 arg2);
void gcEjectDObj(DObj *dobj);
void ftMainRunUpdateColAnim(GObj *fighter_gobj);
void ftMainProcUpdateInterrupt(GObj *fighter_gobj);
void ftMainProcPhysicsMapDefault(GObj *fighter_gobj);
void ftMainProcPhysicsMapCapture(GObj *fighter_gobj);
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
void ftCommonDamageCommonProcUpdate(GObj *fighter_gobj);
void ftCommonDamageAirCommonProcUpdate(GObj *fighter_gobj);
void ftCommonDamageCommonProcInterrupt(GObj *fighter_gobj);
void ftCommonDamageAirCommonProcInterrupt(GObj *fighter_gobj);
void ftCommonDamageCommonProcPhysics(GObj *fighter_gobj);
void ftCommonDamageAirCommonProcMap(GObj *fighter_gobj);
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
void ndsBaseFTCommonTurnRunProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonTurnRunProcInterrupt(GObj *fighter_gobj);
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
void ndsBaseFTCommonPassProcInterrupt(GObj *fighter_gobj);
sb32 ndsBaseFTCommonPassCheckInterruptCommon(GObj *fighter_gobj);
sb32 ndsBaseFTCommonPassCheckInterruptSquat(GObj *fighter_gobj);
void ndsBaseFTCommonSquatProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonSquatProcInterrupt(GObj *fighter_gobj);
sb32 ndsBaseFTCommonSquatCheckInterruptCommon(GObj *fighter_gobj);
void ndsBaseFTCommonSquatSetStatusPass(GObj *fighter_gobj);
sb32 ndsBaseFTCommonSquatCheckGotoPass(GObj *fighter_gobj);
void ndsBaseFTCommonSquatWaitProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonSquatWaitProcInterrupt(GObj *fighter_gobj);
void ndsBaseFTCommonSquatRvProcInterrupt(GObj *fighter_gobj);
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
void ndsBaseFTCommonAttackAirLwProcHit(GObj *fighter_gobj);
void ndsBaseFTCommonAttackAirLwProcUpdate(GObj *fighter_gobj);
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
void ndsBaseFTCommonGuardOnProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonGuardCommonProcInterrupt(GObj *fighter_gobj);
void ndsBaseFTCommonGuardProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonGuardOffProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonGuardSetOffProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonEscapeProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonEscapeProcInterrupt(GObj *fighter_gobj);
void ndsBaseFTCommonEscapeProcStatus(GObj *fighter_gobj);
extern void *gFTDataKirbyMainMotion;
extern uintptr_t llKirbyMainMotionftKirbyAttack100Effect;
void gmCollisionGetFighterPartsWorldPosition(DObj *main_dobj, Vec3f *vec);
void gmCollisionTransformMatrixAll(DObj *dobj, FTParts *parts, Mtx44f mtx);
sb32 gmCollisionCheckFighterInFighterRange(FTAttackColl *attack_coll,
                                           GObj *fighter_gobj);
sb32 gmCollisionCheckFighterAttacksCollide(FTAttackColl *attack_coll1,
                                           FTAttackColl *attack_coll2);
sb32 gmCollisionCheckFighterAttackDamageCollide(FTAttackColl *attack_coll,
                                                FTDamageColl *damage_coll);
sb32 gmCollisionCheckFighterAttackShieldCollide(FTAttackColl *attack_coll,
                                                GObj *fighter_gobj,
                                                DObj *dobj, f32 *p_angle);
void gmCollisionGetFighterAttackDamagePosition(Vec3f *dst,
                                               FTAttackColl *ft_attack_coll,
                                               FTDamageColl *damage_coll);
void gmCollisionGetFighterAttackShieldPosition(Vec3f *dst,
                                               FTAttackColl *ft_attack_coll,
                                               GObj *gobj, DObj *dobj);
void gmCollisionGetFighterAttacksPosition(Vec3f *dst,
                                          FTAttackColl *ft_attack_coll1,
                                          FTAttackColl *ft_attack_coll2);
f32 gmCollisionGetDamageSlashRotation(FTStruct *fp,
                                      FTAttackColl *attack_coll);
GObj *efManagerKirbyVulcanJabMakeEffect(Vec3f *pos, s32 lr, f32 rotate,
                                        f32 vel, f32 add);
sb32 ftCommonCatchCheckInterruptAttack11(GObj *fighter_gobj);
void ftParamSetMotionID(FTStruct *fp, s32 motion_attack_id);
void ftParamSetStatUpdate(FTStruct *fp, u16 flags);
void ftParamUpdate1PGameAttackStats(FTStruct *fp, u16 flags);
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
extern sb32 gFTMainIsDamageDetect[GMCOMMON_PLAYERS_MAX];
extern sb32 gFTMainIsAttackDetect[GMCOMMON_PLAYERS_MAX];
sb32 ftMainCheckGetUpdateDamage(FTStruct *fp, s32 *damage);
void ftMainPlayHitSFX(FTStruct *fp, FTAttackColl *attack_coll);
void ftMainSetHitInteractStats(FTStruct *fp, u32 attack_group_id,
                               GObj *victim_gobj, s32 attack_type,
                               u32 victim_group_id,
                               sb32 ignore_damage_or_hit);
void ftMainSetHitRebound(GObj *attacker_gobj, FTStruct *fp,
                         FTAttackColl *attack_coll, GObj *victim_gobj);
void ftMainUpdateAttackStatFighter(FTStruct *other_fp,
                                   FTAttackColl *other_hit,
                                   FTStruct *this_fp,
                                   FTAttackColl *this_hit,
                                   GObj *other_gobj,
                                   GObj *this_gobj);
void ftMainUpdateShieldStatFighter(FTStruct *attacker_fp,
                                   FTAttackColl *attack_coll,
                                   FTStruct *victim_fp,
                                   GObj *attacker_gobj,
                                   GObj *victim_gobj);
void ftMainUpdateCatchStatFighter(FTStruct *attacker_fp,
                                  FTAttackColl *attack_coll,
                                  FTStruct *victim_fp,
                                  GObj *attacker_gobj,
                                  GObj *victim_gobj);
void ftMainProcessHitCollisionStatsMain(GObj *fighter_gobj);
void ftMainSearchHitFighter(GObj *this_gobj);
void ftMainSearchFighterCatch(GObj *this_gobj);
void ftMainProcSearchCatch(GObj *fighter_gobj);
void ftMainSearchHitHazard(GObj *fighter_gobj);
void ftMainProcSearchHitAll(GObj *fighter_gobj);
void ftMainProcParams(GObj *fighter_gobj);
sb32 ftMainCheckAddGroundObstacle(GObj *gobj,
                                  sb32 (*proc_update)(GObj *, GObj *, s32 *));
void ftMainClearGroundObstacle(GObj *gobj);
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
extern u16 gMPCollisionUpdateTic;
f32 scSubsysFighterGetLightAngleX(void);
f32 scSubsysFighterGetLightAngleY(void);
void scSubsysFighterProcUpdate(GObj *fighter_gobj);
void scSubsysFighterSetStatus(GObj *fighter_gobj, s32 status_id);
void ftCommonEntrySetStatus(GObj *fighter_gobj);
GObj *ftShadowMakeShadow(GObj *fighter_gobj);
void ftKirbyCopyLinkSpecialNDestroyBoomerang(GObj *fighter_gobj);
void ftLinkSpecialNDestroyBoomerang(GObj *fighter_gobj);
s32 efParticleGetBankID(uintptr_t scripts_lo);
s32 efParticleGetLoadBankID(void *script_lo, void *script_hi,
                            void *texture_lo, void *texture_hi);

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
