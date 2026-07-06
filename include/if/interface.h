#ifndef SSB64_NDS_IF_INTERFACE_H
#define SSB64_NDS_IF_INTERFACE_H

#include <gm/generic.h>
#include <it/item.h>
#include <sys/objtypes.h>

#define IFCOMMON_PLAYERARROWS_MASK_LEFT (1 << 0)
#define IFCOMMON_PLAYERARROWS_MASK_RIGHT (1 << 1)

typedef enum IFPauseKind {
    nIFPauseKindPlayerNA,
    nIFPauseKindDefault,
    nIFPauseKindBonus
} IFPauseKind;

typedef enum IFPlayerTagKind {
    nIFPlayerTagKind1P,
    nIFPlayerTagKind2P,
    nIFPlayerTagKind3P,
    nIFPlayerTagKind4P,
    nIFPlayerTagKindCP,
    nIFPlayerTagKindHeart
} IFPlayerTagKind;

typedef struct LBGenerator {
    DObj *dobj;
} LBGenerator;

typedef struct IFPlayerCommon {
    ub8 is_magnify_display;
    u8 magnify_mode;
    f32 magnify_scale;
    s32 *player_pos_x;
    u16 player_pos_y;
    u8 arrows_flags;
    u8 arrows_left_status;
    u8 arrows_right_status;
} IFPlayerCommon;

typedef struct IFACharacter {
    Vec2h pos;
    intptr_t offset;
} IFACharacter;

typedef struct IFDCharacter {
    Vec2f pos;
    Vec2f vel;
    u8 image_id;
    u8 is_lock_movement;
} IFDCharacter;

typedef struct IFPlayerDamage {
    s32 damage;
    s32 pos_adjust_wait;
    s32 flash_reset_wait;
    f32 scale;
    IFDCharacter chars[4];
    GObj *interface_gobj;
    u8 color_id;
    u8 is_update_anim;
    u8 char_display_count;
    u8 break_anim_frame;
    u8 dead_stopupdate_wait;
    u8 is_show_interface;
} IFPlayerDamage;

typedef struct IFPlayerSteal {
    u8 anim_frames;
    u16 steal_pos_x;
    u16 steal_pos_y;
    u16 target_pos_x;
} IFPlayerSteal;

typedef struct IFPlayerMagnify {
    Vec2f pos;
    Vp viewport;
    GObj *interface_gobj;
    u8 color_id;
} IFPlayerMagnify;

typedef struct IFTraffic {
    Vec2h pos;
    u8 color_id;
} IFTraffic;

typedef struct IFPauseDecal {
    intptr_t offset;
    Vec2h pos;
    SYColorRGBPair colors;
} IFPauseDecal;

#define ifGetPlayer(interface_gobj) ((interface_gobj)->user_data.s)
#define ifSetPlayer(interface_gobj, player) ((interface_gobj)->user_data.s = (player))
#define ifGetSObj(interface_gobj) ((SObj *)(interface_gobj)->user_data.p)
#define ifSetSObj(interface_gobj, sobj) ((interface_gobj)->user_data.p = (void *)(sobj))
#define ifGetProc(interface_gobj) ((void (*)())(interface_gobj)->user_data.p)
#define ifSetProc(interface_gobj, proc) ((interface_gobj)->user_data.p = (void *)(proc))

extern u8 dIFCommonPlayerTeamColorIDs[];
extern intptr_t dIFCommonTimerDigitSpriteOffsets[];
extern u8 dIFCommonPlayerTagPrimColorsR[];
extern u8 dIFCommonPlayerTagPrimColorsG[];
extern u8 dIFCommonPlayerTagPrimColorsB[];
extern u8 dIFCommonPlayerTagEnvColorsR[];
extern u8 dIFCommonPlayerTagEnvColorsG[];
extern u8 dIFCommonPlayerTagEnvColorsB[];
extern IFPlayerCommon gIFCommonPlayerInterface;

void efParticleInitAll(void);
s32 efParticleGetLoadBankID(uintptr_t script_lo, uintptr_t script_hi,
                            uintptr_t texture_lo, uintptr_t texture_hi);
LBGenerator *lbParticleMakeGenerator(s32 bank_id, s32 generator_id);
void lbParticleDrawTextures(GObj *gobj);
void ifCommonPlayerDamageSetShowInterface(void);
s32 ifCommonPlayerDamageGetSpecialArrayID(s32 damage, u8 *digits);
s32 ifCommonPlayerDamageGetPercentArrayID(s32 damage, u8 *digits);
s32 ifCommonPlayerDamageGetHitPointsArrayID(s32 damage, u8 *digits);
s32 ifCommonPlayerDamageGetDigitOffset(s32 digit_count, u8 *digit_ids);
void ifCommonPlayerDamageUpdateDigits(GObj *interface_gobj);
void ifCommonPlayerDamageUpdateAnim(GObj *interface_gobj);
void ifCommonPlayerDamageProcUpdate(GObj *interface_gobj);
void ifCommonPlayerDamageProcDisplay(GObj *interface_gobj);
void ifCommonPlayerDamageSetDigitAttr(void);
void ifCommonPlayerDamageSetDigitPositions(void);
void ifCommonPlayerDamageInitInterface(void);
void ifCommonPlayerDamageStartBreakAnim(FTStruct *fp);
void ifCommonPlayerDamageStopBreakAnim(FTStruct *fp);
void ifCommonPlayerStockMultiProcDisplay(GObj *interface_gobj);
void ifCommonPlayerStockSetIconAttr(void);
void ifCommonPlayerStockMultiMakeInterface(s32 player);
void ifCommonPlayerStockSingleProcDisplay(GObj *interface_gobj);
void ifCommonPlayerStockSetLUT(s32 player, s32 lut_id, FTAttributes *attr);
void ifCommonPlayerStockSingleMakeInterface(s32 player);
void ifCommonPlayerStockStealProcUpdate(GObj *interface_gobj);
void ifCommonPlayerStockStealMakeInterface(s32 thief, s32 stolen);
void ifCommonPlayerStockInitInterface(void);
void ifCommonPlayerMagnifyGetPosition(f32 player_pos_x, f32 player_pos_y,
                                      Vec2f *magnify_pos);
void ifCommonPlayerMagnifyUpdateRender(Gfx **dls, s32 color_id, f32 ulx,
                                       f32 uly);
void ifCommonPlayerMagnifyUpdateViewport(Gfx **dls, FTStruct *fp);
void ifCommonPlayerMagnifyProcDisplay(FTStruct *fp);
void ifCommonPlayerMagnifyMakeInterface(void);
void ifCommonPlayerArrowsLeftProcDisplay(GObj *interface_gobj);
void ifCommonPlayerArrowsRightProcDisplay(GObj *interface_gobj);
void ifCommonPlayerArrowsAddAnim(GObj *interface_gobj);
void ifCommonPlayerArrowsLeftProcUpdate(GObj *interface_gobj);
void ifCommonPlayerArrowsRightProcUpdate(GObj *interface_gobj);
GObj *ifCommonPlayerArrowsMakeInterface(void (*proc_display)(GObj *),
                                        void (*proc_update)(GObj *));
void ifCommonPlayerArrowsFuncRun(GObj *interface_gobj);
void ifCommonPlayerArrowsMainProcDisplay(GObj *interface_gobj);
void ifCommonPlayerArrowsInitInterface(void);
void ifCommonPlayerArrowsUpdateFlags(f32 x, f32 y);
void ifCommonPlayerTagProcDisplay(GObj *interface_gobj);
void ifCommonPlayerTagMakeInterface(void);
void ifCommonItemArrowProcDisplay(GObj *interface_gobj);
GObj *ifCommonItemArrowMakeInterface(ITStruct *ip);
void ifCommonItemArrowSetAttr(void);
void ifCommonAnnounceThread(GObj *interface_gobj);
void ifCommonAnnounceSetAttr(GObj *interface_gobj, s32 file_id,
                             IFACharacter *character, s32 sprite_count);
void ifCommonAnnounceGoMakeInterface(void);
void ifCommonAnnounceGoSetStatus(void);
SObj *ifCommonTrafficMakeSObj(GObj *interface_gobj, s32 id);
void ifCommonCountdownThread(GObj *interface_gobj);
SObj *ifCommonCountdownMakeInterface(void);
GObj *ifCommonAnnounceTimeUpMakeInterface(void);
void ifCommonEntryFocusThread(GObj *interface_gobj);
void ifCommonEntryFocusMakeInterface(s32 id);
void ifCommonEntryAllThread(GObj *interface_gobj);
void ifCommonEntryAllMakeInterface(void);
void ifCommonSuddenDeathThread(GObj *interface_gobj);
void ifCommonAnnounceSetColors(GObj *interface_gobj, SYColorRGBPair *colors);
void ifCommonSuddenDeathMakeInterface(void);
void ifCommonTimerProcDisplay(GObj *interface_gobj);
void ifCommonTimerSetAttr(void);
void ifCommonTimerInitAnnouncedSeconds(void);
SObj *ifCommonTimerMakeDigits(void);
void ifCommonTimerFuncRun(GObj *interface_gobj);
void ifCommonTimerMakeInterface(void (*proc)(void));
GObj *ifCommonAnnounceGameSetMakeInterface(void);
void ifCommonBattleInitPlacement(void);
void ifCommonBattleInterfacePauseGObj(GObj *interface_gobj, u32 unused);
void ifCommonBattleInterfaceResumeGObj(GObj *interface_gobj, u32 unused);
void ifCommonBattleInterfaceProcUpdate(void);
void ifCommonBattleEndInitSoundNum(void);
void ifCommonBattleEndPlaySoundQueue(void);
void ifCommonBattleEndAddSoundQueueID(u16 sfx_id);
void ifCommonBattleEndSetBossDefeat(void);
void ifCommonBattleUpdateScoreStocks(FTStruct *fp);
void ifCommonBattlePauseProcDisplay(GObj *interface_gobj);
void ifCommonBattlePausePlayerNumMakeSObj(GObj *interface_gobj, s32 player);
void ifCommonBattlePauseDecalMakeSObjID(GObj *interface_gobj, s32 id);
void ifCommonBattlePauseMakeSObjsAll(GObj *interface_gobj);
void ifCommonBattlePauseMakeInterface(s32 player);
void ifCommonBattlePauseEjectGObjs(void);
void ifCommonInterfaceSetGObjFlagsAll(u32 flags);
void ifCommonBattlePauseSetGObjFlagsAll(u32 flags);
void ifCommonBattlePauseInitInterface(s32 player);
void ifCommonBattleGoUpdateInterface(void);
void ifCommonBattleInterfaceProcSet(void);
void ifCommonBattlePauseUpdateInterface(void);
void ifCommonBattlePauseRestoreInterfaceAll(void);
void ifCommonBattleEndUpdateInterface(void);
void ifCommonBattleBossDefeatUpdateInterface(void);
void ifCommonBattleSetUpdateInterface(void);
void ifCommonSetMaxNumGObj(void);
void ifCommonBattleUpdateInterfaceAll(void);
void ifCommonBattleSetGameStatusWait(void);
void ifCommonPlayerStockMakeStockSnap(FTStruct *fp);
void ifCommonPlayerScoreMakeEffect(FTStruct *fp, s32 score);
GObj *ifCommonAnnounceFailureMakeInterface(void);
GObj *ifCommonAnnounceCompleteMakeInterface(void);
void ifCommonBonusInterfaceProcUpdate(void);
void ifCommonBattleSetInterface(void (*proc_update)(void),
                                void (*proc_set)(void), u16 sfx_id,
                                u16 restore_wait);
void ifCommonBattleBossDefeatSetGameStatus(void);
void ifCommon1PGameInterfaceProcSet(void);
void ifCommonAnnounceEndMessage(void);
void ifCommonAnnounceCompleteInitInterface(u16 sfx_id);
void ifCommonAnnounceTimeUpInitInterface(void);
void ifCommonAnnounceFailureInitInterface(void);
void ifScreenFlashMakeInterface(u8 alpha);

#endif
