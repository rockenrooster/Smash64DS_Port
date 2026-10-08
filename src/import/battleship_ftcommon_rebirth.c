#include <ef/effect.h>
#include <ft/fighter.h>
#include <if/interface.h>
#include <mp/map.h>
#include <sc/scene.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

extern FTDesc dFTManagerDefaultFighterDesc;

void ifCommonPlayerDamageStopBreakAnim(FTStruct *fp);
void mpCommonSetFighterGround(FTStruct *fp);
void mpCollisionGetMapObjIDsKind(s32 kind, s32 *ids);
void mpCollisionGetMapObjPositionID(s32 id, Vec3f *pos);
void ftManagerInitFighter(GObj *fighter_gobj, FTDesc *desc);
void ftMainPlayAnimEventsAll(GObj *fighter_gobj);
GObj *efManagerRebirthHaloMakeEffect(GObj *fighter_gobj, f32 size);
void ftParamSetTimedHitStatusInvincible(FTStruct *fp, s32 invincible_tics);
void ftCommonFallSetStatus(GObj *fighter_gobj);
sb32 ftCommonGroundCheckInterrupt(GObj *fighter_gobj);
void ftCommonRebirthStandSetStatus(GObj *fighter_gobj);
void ftCommonRebirthWaitSetStatus(GObj *fighter_gobj);

#include <nds/nds_p4_contents.h>
#if NDS_P4
#include <nds/nds_p4.h>
/* P4: a content's Remix initial_script runs in ftManagerInitFighter's kind
 * switch on every stock (Dedede.initial_script_). */
#define ftManagerInitFighter(g_, d_) (ftManagerInitFighter((g_), (d_)), ndsP4OnInitFighter(g_))
#endif
#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonrebirth.c"
#if NDS_P4
#undef ftManagerInitFighter
#endif
