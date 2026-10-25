/* Weak stand-ins for BattleShip functions whose translation units the port
 * does not import: camera preparation, stage hazard queries (Zebes acid,
 * Hyrule's twister, Congo's barrel), 1P game hooks, effect and particle
 * helpers. Each does what the DS needs: nothing, the source's value, or the
 * source's own body copied verbatim (lbCommonEjectGObjLinkedList). An
 * imported unit that defines the real function wins the link (NDS_WEAK).
 * Compiled on its own; the imported units call these by their source names. */

#include <ef/effect.h>
#include <ft/fighter.h>
#include <gm/generic.h>
#include <if/interface.h>
#include <it/item.h>
#include <nds/nds_audio_fgm.h>
#include <nds/nds_effects.h>
#include <nds/nds_fx_effect_census.h>
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

NDS_WEAK sb32 grHyruleTwisterCheckGetPosition(Vec3f *position)
{
    if (position != NULL)
    {
        position->x = 0.0F;
        position->y = 0.0F;
        position->z = 0.0F;
    }
    return FALSE;
}

NDS_WEAK void grJungleTaruCannGetPosition(Vec3f *position)
{
    if (position != NULL)
    {
        position->x = 0.0F;
        position->y = 0.0F;
        position->z = 0.0F;
    }
}

NDS_WEAK f32 grJungleTaruCannGetRotate(void)
{
    return 0.0F;
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

NDS_WEAK void itMainDestroyItem(GObj *item_gobj)
{
    (void)item_gobj;
}

NDS_WEAK GObj *efManagerDeadExplodeMakeEffect(Vec3f *pos, s32 player,
                                              u32 kind)
{
    ndsFxEffectCensusRecord(
        NDS_FX_EF_MANAGER_DEAD_EXPLODE_MAKE_EFFECT,
        NDS_FX_SUBSTITUTE);
    (void)player;
    return ndsEFManagerMakeVisualEffect(nNDSVisualEffectDeath, pos,
                                        2.0F + ((f32)(kind & 3u) * 0.2F),
                                        1, NULL);
}

NDS_WEAK LBParticle *efManagerSparkleWhiteDeadMakeEffect(Vec3f *pos,
                                                         f32 scale)
{
    ndsFxEffectCensusRecord(
        NDS_FX_EF_MANAGER_SPARKLE_WHITE_DEAD_MAKE_EFFECT,
        NDS_FX_SUBSTITUTE);
    (void)ndsEFManagerMakeVisualEffect(nNDSVisualEffectSparkle, pos,
                                       scale, 1, NULL);
    return NULL;
}

NDS_WEAK void sc1PTrainingModeLoadWallpaper(void)
{
}

NDS_WEAK void sc1PGameBossInitWallpaper(void)
{
}

NDS_WEAK void sc1PGameSetCameraZoom(void)
{
}

NDS_WEAK s32 func_800264A4_270A4(void)
{
    ndsAudioFgmResumeGame();
    return 0;
}

NDS_WEAK s32 func_80026594_27194(void)
{
    ndsAudioFgmPauseGame();
    return 0;
}

extern void ndsProjectToViewport(CObj *cobj, Mtx44f matrix, Vec3f *pos,
                                 f32 *dist_x, f32 *dist_y);

NDS_WEAK void func_ovl2_800EB924(CObj *cobj, Mtx44f matrix, Vec3f *pos,
                                 f32 *dist_x, f32 *dist_y)
{
    /* This is gameplay-visible: Link's Boomerang feeds the result into its
     * delayed off-camera lifetime check (the old compatibility stub returned
     * raw world X/Y and killed live Boomerangs). gGMCameraMatrix/CObj
     * viewport remain the gameplay camera contract consumed by BattleShip. */
    ndsProjectToViewport(cobj, matrix, pos, dist_x, dist_y);
}

NDS_WEAK void func_ovl65_8018F6DC(void)
{
}

NDS_WEAK void gmRumbleResumeProcessAll(void)
{
}

/* BattleShip lb/lbcommon.c:2113-2120, verbatim. This used to be an empty
 * weak stub, so ifCommonBattlePauseEjectGObjs ejected nothing and the pause
 * decals (blue A, green B) stayed on the top screen after every unpause
 * (docs/BUGS.md, 2026-09-06). */
NDS_WEAK void lbCommonEjectGObjLinkedList(GObj *gobj)
{
    if (gobj == NULL)
    {
        return;
    }
    if (gobj->link_next != NULL)
    {
        lbCommonEjectGObjLinkedList(gobj->link_next);
    }
    gcEjectGObj(gobj);
}

NDS_WEAK void ftParamUnlockPlayerControl(GObj *fighter_gobj)
{
    (void)fighter_gobj;
}

NDS_WEAK void ftCommonAppearSetStatus(GObj *fighter_gobj)
{
    (void)fighter_gobj;
}

NDS_WEAK void ftPublicDefeatedAddID(u16 sfx_id)
{
    (void)sfx_id;
}

NDS_WEAK GObj *gEFParticleStructsGObj;
NDS_WEAK GObj *gEFParticleGeneratorsGObj;

void ndsEFParticleEnsureGObjPlaceholders(void)
{
    /* The real GObjs exist; a placeholder here could only shadow them. */
}

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
