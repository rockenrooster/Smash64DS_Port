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
                                                f32 frame_begin);

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

#ifdef lbRelocGetFileData
#undef lbRelocGetFileData
#endif
#define lbRelocGetFileData(type, file, offset) \
    ((type)((uintptr_t)(file) + (intptr_t)(offset)))

#define ftMainPlayAnimEventsAll battleship_ftMainPlayAnimEventsAll
#define ftMainSetStatus battleship_ftMainSetStatus
#include "../../decomp/BattleShip-main/decomp/src/ft/ftmain.c"
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
    battleship_ftMainSetStatus(fighter_gobj, status_id, frame_begin,
                               anim_speed, flags);
    ndsDiagnosticsRecordImportedFTMainSetStatus(fighter_gobj, status_id,
                                                frame_begin);
}
