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

typedef struct GObj GObj;

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
    nFTPartsDetailHigh = 0,
    nFTPartsDetailLow = 1
};

#define FTKEY_EVENT_INSTRUCTION(k, t) \
    ((((u16)(k) << 12) & 0xF000u) | ((u16)(t) & 0x0FFFu))
#define FTKEY_EVENT_STICK(x, y, t) \
    FTKEY_EVENT_INSTRUCTION(nFTKeyEventStick, (t)), \
    ((((u16)(x) << 8) & 0xFF00u) | (((u16)(y) << 0) & 0x00FFu))
#define FTKEY_EVENT_BUTTON(b, t) \
    FTKEY_EVENT_INSTRUCTION(nFTKeyEventButton, (t)), ((u16)(b))
#define FTKEY_EVENT_END() FTKEY_EVENT_INSTRUCTION(nFTKeyEventEnd, 0)

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

typedef struct FTStruct {
    u8 player;
} FTStruct;

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
void ftParamCheckSetFighterColAnimID(GObj *fighter_gobj, s32 colanim_id,
                                     s32 unused);
FTStruct *ftGetStruct(GObj *fighter_gobj);
void ftDisplayLightsDrawReflect(Gfx **display_list, f32 light_angle_x,
                                f32 light_angle_y);
f32 scSubsysFighterGetLightAngleX(void);
f32 scSubsysFighterGetLightAngleY(void);
void scSubsysFighterSetStatus(GObj *fighter_gobj, s32 status_id);

enum {
    nGMColAnimFighterComPlayer = 0
};

#endif
