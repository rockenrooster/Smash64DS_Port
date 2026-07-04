/*
 * Whole BattleShip ft/ftmain.c import.
 *
 * Compatibility declarations live in include/. Keep gameplay behavior in the
 * original TU instead of extending the old DS-side ftMain seam copies.
 */
#include <PR/ultratypes.h>

typedef struct alSoundEffect {
    u8 filler_0x00[0x26];
    u16 sfx_id;
} alSoundEffect;

#include <ef/effect.h>
#include <ft/fighter.h>
#include <gm/gmsound.h>

void lbCommonAddFighterPartsFigatree(DObj *root_dobj, void *figatree,
                                     f32 anim_frame);
void gcSetAnimSpeed(GObj *gobj, f32 anim_speed);
void ftParamSetAnimLocks(FTStruct *fp);
void ftParamClearAnimLocks(FTStruct *fp);
void mpCommonUpdateFighterSlopeContour(GObj *fighter_gobj);
void func_80026738_27338(alSoundEffect *sfx);
alSoundEffect *lbCommonMakePositionFGM(u16 fgm, f32 pos);
void ndsDiagnosticsRecordImportedFTMainAnimEvents(GObj *fighter_gobj);
void ndsDiagnosticsRecordImportedFTMainSetStatus(GObj *fighter_gobj,
                                                s32 status_id,
                                                f32 frame_begin,
                                                f32 anim_speed, u32 flags);
sb32 ndsDiagnosticsHandleImportedFTMainSetStatusBefore(GObj *fighter_gobj,
                                                       s32 status_id,
                                                       f32 frame_begin,
                                                       f32 anim_speed,
                                                       u32 flags);

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

#ifdef lbRelocGetFileData
#undef lbRelocGetFileData
#endif
#define lbRelocGetFileData(type, file, offset) \
    ((type)((uintptr_t)(file) + (intptr_t)(offset)))

#define ftMainCheckGetUpdateDamage battleship_ftMainCheckGetUpdateDamage
#define ftMainPlayHitSFX battleship_ftMainPlayHitSFX
#define ftMainUpdateDamageStatFighter battleship_ftMainUpdateDamageStatFighter
#define ftMainSetHitInteractStats battleship_ftMainSetHitInteractStats
#define ftMainSetHitRebound battleship_ftMainSetHitRebound
#define ftMainUpdateAttackStatFighter battleship_ftMainUpdateAttackStatFighter
#define ftMainUpdateShieldStatFighter battleship_ftMainUpdateShieldStatFighter
#define ftMainUpdateCatchStatFighter battleship_ftMainUpdateCatchStatFighter
#define ftMainProcessHitCollisionStatsMain battleship_ftMainProcessHitCollisionStatsMain
#define ftMainCheckAddGroundObstacle battleship_ftMainCheckAddGroundObstacle
#define ftMainClearGroundObstacle battleship_ftMainClearGroundObstacle
#define ftMainSetHitHazard battleship_ftMainSetHitHazard
#define ftMainSearchHitHazard battleship_ftMainSearchHitHazard
#define ftMainSearchHitFighter battleship_ftMainSearchHitFighter
#define ftMainSearchFighterCatch battleship_ftMainSearchFighterCatch
#define ftMainProcSearchCatch battleship_ftMainProcSearchCatch
#define ftMainSearchHitItem battleship_ftMainSearchHitItem
#define ftMainSearchHitWeapon battleship_ftMainSearchHitWeapon
#define ftMainSearchGroundHit battleship_ftMainSearchGroundHit
#define ftMainProcSearchHitAll battleship_ftMainProcSearchHitAll
#define ftMainProcParams battleship_ftMainProcParams
#define ftMainRunUpdateColAnim battleship_ftMainRunUpdateColAnim
#define ftMainPlayAnimEventsAll battleship_ftMainPlayAnimEventsAll
#define ftMainSetStatus battleship_ftMainSetStatus
#include "../../decomp/BattleShip-main/decomp/src/ft/ftmain.c"
#undef ftMainCheckGetUpdateDamage
#undef ftMainPlayHitSFX
#undef ftMainUpdateDamageStatFighter
#undef ftMainSetHitInteractStats
#undef ftMainSetHitRebound
#undef ftMainUpdateAttackStatFighter
#undef ftMainUpdateShieldStatFighter
#undef ftMainUpdateCatchStatFighter
#undef ftMainProcessHitCollisionStatsMain
#undef ftMainCheckAddGroundObstacle
#undef ftMainClearGroundObstacle
#undef ftMainSetHitHazard
#undef ftMainSearchHitHazard
#undef ftMainSearchHitFighter
#undef ftMainSearchFighterCatch
#undef ftMainProcSearchCatch
#undef ftMainSearchHitItem
#undef ftMainSearchHitWeapon
#undef ftMainSearchGroundHit
#undef ftMainProcSearchHitAll
#undef ftMainProcParams
#undef ftMainRunUpdateColAnim
#undef ftMainPlayAnimEventsAll
#undef ftMainSetStatus

void ftMainPlayAnimEventsAll(GObj *fighter_gobj)
{
    battleship_ftMainPlayAnimEventsAll(fighter_gobj);
    ndsDiagnosticsRecordImportedFTMainAnimEvents(fighter_gobj);
}

void ftMainSetStatus(GObj *fighter_gobj, s32 status_id,
                     f32 frame_begin, f32 anim_speed, u32 flags)
{
    if (ndsDiagnosticsHandleImportedFTMainSetStatusBefore(fighter_gobj,
            status_id, frame_begin, anim_speed, flags) != FALSE)
    {
        return;
    }
    battleship_ftMainSetStatus(fighter_gobj, status_id, frame_begin,
                               anim_speed, flags);
    ndsDiagnosticsRecordImportedFTMainSetStatus(fighter_gobj, status_id,
                                                 frame_begin, anim_speed,
                                                 flags);
}
