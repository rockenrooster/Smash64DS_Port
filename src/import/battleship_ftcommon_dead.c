#include <ef/effect.h>
#include <ft/fighter.h>
#include <gm/gmsound.h>
#include <if/interface.h>
#include <it/item.h>
#include <sc/scene.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

#ifndef CObjGetStruct
#define CObjGetStruct(gobj) ((CObj *)((gobj)->obj))
#endif

extern GObj *gGMCameraGObj;
typedef struct alSoundEffect alSoundEffect;

alSoundEffect *func_800269C0_275C0(u16 sfx_id);
void ifCommonBattleEndAddSoundQueueID(u16 sfx_id);
void ifCommonPlayerDamageStartBreakAnim(FTStruct *fp);
void ifCommonPlayerStockMakeStockSnap(FTStruct *fp);
void ifCommonPlayerScoreMakeEffect(FTStruct *fp, s32 score);
void ifCommonBattleUpdateScoreStocks(FTStruct *fp);
void ifCommonAnnounceEndMessage(void);
void sc1PGameSetPlayerDefeatStats(s32 player, s32 team_order);
void sc1PGameSpawnEnemyTeamNext(GObj *fighter_gobj);
void ftCommonSleepSetStatus(GObj *fighter_gobj);
void ftCommonRebirthDownSetStatus(GObj *fighter_gobj);
void ftManagerDestroyFighterWeapons(GObj *fighter_gobj);
void itMainDestroyItem(GObj *item_gobj);
GObj *efManagerQuakeMakeEffect(s32 quake_id);
GObj *efManagerDeadExplodeMakeEffect(Vec3f *pos, s32 player, u32 kind);
LBParticle *efManagerSparkleWhiteDeadMakeEffect(Vec3f *pos, f32 scale);
void ifScreenFlashSetColAnimID(s32 colanim_id, s32 colanim_duration);
void ftParamStopVoiceRunProcDamage(GObj *fighter_gobj);
void ftParamTryUpdateItemMusic(void);

#define ftCommonDeadCheckInterruptCommon \
    ndsBaseFTCommonDeadCheckInterruptCommon

sb32 ndsBaseFTCommonDeadCheckInterruptCommon(GObj *fighter_gobj);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommondead.c"

#undef ftCommonDeadCheckInterruptCommon

sb32 ftCommonDeadCheckInterruptCommon(GObj *fighter_gobj)
{
    return ndsBaseFTCommonDeadCheckInterruptCommon(fighter_gobj);
}
