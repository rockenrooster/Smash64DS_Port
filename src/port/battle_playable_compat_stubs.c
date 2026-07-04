#include <ef/effect.h>
#include <ft/fighter.h>
#include <gm/generic.h>
#include <if/interface.h>
#include <it/item.h>
#include <sc/scene.h>
#include <sys/objtypes.h>

#define NDS_WEAK __attribute__((weak))

f32 lbCommonSin(f32 angle);
f32 lbCommonCos(f32 angle);

NDS_WEAK void func_8001663C(Gfx **dls, CObj *cobj, s32 buffer_id)
{
    (void)dls;
    (void)cobj;
    (void)buffer_id;
}

NDS_WEAK void gcPrepCameraMatrix(Gfx **dls, CObj *cobj)
{
    (void)dls;
    (void)cobj;
}

NDS_WEAK void gcRunFuncCamera(CObj *cobj, s32 arg)
{
    (void)cobj;
    (void)arg;
}

NDS_WEAK void func_80017CC8(CObj *cobj)
{
    (void)cobj;
}

NDS_WEAK f32 lbCommonTan(f32 angle)
{
    f32 cos = lbCommonCos(angle);

    return (cos != 0.0F) ? (lbCommonSin(angle) / cos) : 0.0F;
}

NDS_WEAK void lbCommonInitCameraVec(CObj *cobj, u8 tk, u8 arg2)
{
    (void)cobj;
    (void)tk;
    (void)arg2;
}

NDS_WEAK void lbCommonInitCameraOrtho(CObj *cobj, u8 tk, u8 arg2)
{
    (void)cobj;
    (void)tk;
    (void)arg2;
}

NDS_WEAK void grZebesAcidGetLevelInfo(f32 *current, f32 *step)
{
    if (current != NULL) {
        *current = 0.0F;
    }
    if (step != NULL) {
        *step = 0.0F;
    }
}

NDS_WEAK void ifCommonBattleEndAddSoundQueueID(u16 sfx_id)
{
    (void)sfx_id;
}

NDS_WEAK void ifCommonPlayerDamageStartBreakAnim(FTStruct *fp)
{
    (void)fp;
}

NDS_WEAK void ifCommonPlayerDamageStopBreakAnim(FTStruct *fp)
{
    (void)fp;
}

NDS_WEAK void ifCommonPlayerStockMakeStockSnap(FTStruct *fp)
{
    (void)fp;
}

NDS_WEAK void ifCommonPlayerScoreMakeEffect(FTStruct *fp, s32 score)
{
    (void)fp;
    (void)score;
}

NDS_WEAK void ifCommonBattleUpdateScoreStocks(FTStruct *fp)
{
    (void)fp;
}

NDS_WEAK void ifCommonAnnounceEndMessage(void)
{
}

NDS_WEAK void sc1PGameSetPlayerDefeatStats(s32 player, s32 team_order)
{
    (void)player;
    (void)team_order;
}

NDS_WEAK void sc1PGameSpawnEnemyTeamNext(GObj *fighter_gobj)
{
    (void)fighter_gobj;
}

NDS_WEAK void ftCommonSleepSetStatus(GObj *fighter_gobj)
{
    (void)fighter_gobj;
}

NDS_WEAK void itMainDestroyItem(GObj *item_gobj)
{
    (void)item_gobj;
}

NDS_WEAK void efManagerDeadExplodeMakeEffect(Vec3f *pos, s32 player, s32 kind)
{
    (void)pos;
    (void)player;
    (void)kind;
}

NDS_WEAK void efManagerSparkleWhiteDeadMakeEffect(Vec3f *pos, f32 scale)
{
    (void)pos;
    (void)scale;
}

NDS_WEAK GObj *efManagerRebirthHaloMakeEffect(GObj *fighter_gobj, f32 size)
{
    (void)fighter_gobj;
    (void)size;
    return NULL;
}
