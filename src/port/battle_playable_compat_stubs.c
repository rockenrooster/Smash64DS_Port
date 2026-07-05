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

NDS_WEAK void grWallpaperResumePerspUpdate(void)
{
}

NDS_WEAK void grWallpaperPausePerspUpdate(void)
{
}

NDS_WEAK void grWallpaperRunProcessAll(void)
{
}

NDS_WEAK void grWallpaperResumeProcessAll(void)
{
}

NDS_WEAK void sc1PGameSetCameraZoom(void)
{
}

NDS_WEAK s32 func_800264A4_270A4(void)
{
    return 0;
}

NDS_WEAK s32 func_80026594_27194(void)
{
    return 0;
}

NDS_WEAK void func_ovl2_800EB924(CObj *cobj, Mtx44f matrix, Vec3f *pos,
                                 f32 *dist_x, f32 *dist_y)
{
    (void)cobj;
    (void)matrix;
    if (dist_x != NULL) {
        *dist_x = (pos != NULL) ? pos->x : 0.0F;
    }
    if (dist_y != NULL) {
        *dist_y = (pos != NULL) ? pos->y : 0.0F;
    }
}

NDS_WEAK void func_ovl65_8018F6DC(void)
{
}

NDS_WEAK void gmRumbleResumeProcessAll(void)
{
}

NDS_WEAK void lbCommonEjectGObjLinkedList(GObj *gobj)
{
    (void)gobj;
}

NDS_WEAK void ftParamUnlockPlayerControl(GObj *fighter_gobj)
{
    (void)fighter_gobj;
}

NDS_WEAK void ftCommonAppearSetStatus(GObj *fighter_gobj)
{
    (void)fighter_gobj;
}

NDS_WEAK sb32 ftCommonSleepCheckIgnorePauseMenu(GObj *fighter_gobj)
{
    (void)fighter_gobj;
    return FALSE;
}

NDS_WEAK void ftPublicDefeatedAddID(u16 sfx_id)
{
    (void)sfx_id;
}

NDS_WEAK void efManagerStockSnapMakeEffect(f32 pos_x, f32 pos_y)
{
    (void)pos_x;
    (void)pos_y;
}

NDS_WEAK void efManagerStockStealStartMakeEffect(f32 pos_x, f32 pos_y)
{
    (void)pos_x;
    (void)pos_y;
}

NDS_WEAK void efManagerStockStealEndMakeEffect(f32 pos_x, f32 pos_y)
{
    (void)pos_x;
    (void)pos_y;
}

NDS_WEAK LBParticle *efManagerBattleScoreMakeEffect(Vec3f *pos, s32 score)
{
    (void)pos;
    (void)score;
    return NULL;
}

NDS_WEAK GObj *gEFParticleStructsGObj;
NDS_WEAK GObj *gEFParticleGeneratorsGObj;

NDS_WEAK void efParticleGObjSetSkipAll(void)
{
    if (gEFParticleStructsGObj != NULL) {
        gEFParticleStructsGObj->flags |= ~0xFFFFu;
    }
    if (gEFParticleGeneratorsGObj != NULL) {
        gEFParticleGeneratorsGObj->flags |= ~0xFFFFu;
    }
}

NDS_WEAK void efParticleGObjClearSkipID(u32 id)
{
    u32 mask = ~(0x10000u << id);

    if (gEFParticleStructsGObj != NULL) {
        gEFParticleStructsGObj->flags &= mask;
    }
    if (gEFParticleGeneratorsGObj != NULL) {
        gEFParticleGeneratorsGObj->flags &= mask;
    }
}
