/*
 * Bounded BattleShip ftcommontwister.c import for ground-obstacle hazard
 * dispatch. Public names stay project-owned until the continuous Twister
 * runtime is widened.
 */
#include <ft/fighter.h>
#include <gm/gmsound.h>
#include <macros.h>
#include <sys/obj.h>

#ifndef FTCOMMON_TORNADO_RELEASE_WAIT
#define FTCOMMON_TORNADO_RELEASE_WAIT 60.0F
#define FTCOMMON_TORNADO_PICKUP_WAIT 60
#endif

f32 lbCommonCos(f32 angle);
f32 lbCommonSin(f32 angle);
f32 syVectorMag3D(Vec3f *vec);
Vec3f *syVectorDiff3D(Vec3f *dst, Vec3f *src, Vec3f *sub);
Vec3f *syVectorScale3D(Vec3f *vec, f32 scale);
void *func_800269C0_275C0(u16 fgm_id);

#define ftCommonTwisterProcUpdate ndsBaseFTCommonTwisterProcUpdate
#define ftCommonTwisterProcPhysics ndsBaseFTCommonTwisterProcPhysics
#define ftCommonTwisterSetStatus ndsBaseFTCommonTwisterSetStatus
#define ftCommonTwisterShootFighter ndsBaseFTCommonTwisterShootFighter

void ndsBaseFTCommonTwisterProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonTwisterProcPhysics(GObj *fighter_gobj);
void ndsBaseFTCommonTwisterSetStatus(GObj *fighter_gobj, GObj *tornado_gobj);
void ndsBaseFTCommonTwisterShootFighter(GObj *fighter_gobj);

#include <reloc_data.h>

/* decomp ftcommontwister.c:92 recovers the Hyrule map file base as
 * gMPCollisionGroundData - &llGRHyruleMapMapHeader and then adds
 * &llGRHyruleMapTwisterThrowHitDesc, which works there because both symbols
 * are link-time constants equal to their file offsets (0x14 and 0xBC,
 * reloc_data.us.h). Here they are real uintptr_t objects in .data four bytes
 * apart, so the source arithmetic read the tornado's damage, angle and
 * knockback from the word before the ground data: the owner's "tornadoes do
 * too much damage and throw horizontally" (docs/BUGS.md). Shadowing the two
 * symbols with their offsets restores the source's arithmetic, exactly as
 * battleship_grzebes_ground.c does for the acid. The ground-actor probe of
 * 2026-09-07 (agents-0906/hyrule_tornado_contract.final.md) named this site;
 * the same class survives in ftcommonattack100.c (Kirby, P2-3) and
 * efmanager.c (Kirby star, Poke Ball, P2-5). */
#define NDS_RELOC_LVALUE(offset) (*(uintptr_t *)(uintptr_t)(offset))
#define llGRHyruleMapMapHeader NDS_RELOC_LVALUE(0x14u)
#define llGRHyruleMapTwisterThrowHitDesc NDS_RELOC_LVALUE(0xbcu)

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommontwister.c"

#undef llGRHyruleMapTwisterThrowHitDesc
#undef llGRHyruleMapMapHeader
#undef NDS_RELOC_LVALUE

#undef ftCommonTwisterProcUpdate
#undef ftCommonTwisterProcPhysics
#undef ftCommonTwisterSetStatus
#undef ftCommonTwisterShootFighter
