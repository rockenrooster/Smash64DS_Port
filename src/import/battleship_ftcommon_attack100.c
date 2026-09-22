/*
 * Bounded BattleShip ftcommonattack100.c import for the NDS Mario/Fox
 * Attack12 -> Fox rapid-jab proof. Public symbols are remapped so the port
 * backend can guard the original path and keep full rapid-jab gameplay
 * deferred.
 */
#include <PR/ultratypes.h>
#include <ft/fighter.h>
#include <sys/obj.h>

#define ftCommonAttack100StartProcUpdate \
    ndsBaseFTCommonAttack100StartProcUpdate
#define ftCommonAttack100StartSetStatus \
    ndsBaseFTCommonAttack100StartSetStatus
#define ftCommonAttack100LoopKirbyUpdateEffect \
    ndsBaseFTCommonAttack100LoopKirbyUpdateEffect
#define ftCommonAttack100LoopProcUpdate \
    ndsBaseFTCommonAttack100LoopProcUpdate
#define ftCommonAttack100LoopProcInterrupt \
    ndsBaseFTCommonAttack100LoopProcInterrupt
#define ftCommonAttack100LoopSetStatus \
    ndsBaseFTCommonAttack100LoopSetStatus
#define ftCommonAttack100EndSetStatus \
    ndsBaseFTCommonAttack100EndSetStatus
#define ftCommonAttack100StartCheckInterruptCommon \
    ndsBaseFTCommonAttack100StartCheckInterruptCommon

void ndsBaseFTCommonAttack100StartProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonAttack100StartSetStatus(GObj *fighter_gobj);
void ndsBaseFTCommonAttack100LoopKirbyUpdateEffect(FTStruct *fp);
void ndsBaseFTCommonAttack100LoopProcUpdate(GObj *fighter_gobj);
void ndsBaseFTCommonAttack100LoopProcInterrupt(GObj *fighter_gobj);
void ndsBaseFTCommonAttack100LoopSetStatus(GObj *fighter_gobj);
void ndsBaseFTCommonAttack100EndSetStatus(GObj *fighter_gobj);
sb32 ndsBaseFTCommonAttack100StartCheckInterruptCommon(GObj *fighter_gobj);

/* ftcommonattack100.c:91 addresses Kirby's rapid-jab effect table with raw
 * pointer arithmetic:
 *     gFTDataKirbyMainMotion + (intptr_t)&llKirbyMainMotionftKirbyAttack100Effect
 * In BattleShip that name is a link-time offset symbol, so `&name` IS the
 * offset. In this port it is an ordinary uintptr_t object (declared in
 * include/ft/fighter.h, defined in src/port/diagnostics_mp_taskman_state.c)
 * used only as an identity TOKEN by ndsRelocGetFileData's address-keyed
 * registry, so `&name` is a .bss address and the effect table is read from far
 * outside KirbyMainMotion -- garbage offset, rotate, vel and add for every one
 * of efManagerKirbyVulcanJabMakeEffect's five arguments. That is BUGS.md K03:
 * the jab flurry is created, and drawn from nonsense.
 *
 * Restore the source offset the way the stage imports already do (see
 * battleship_gryamabuki_ground.c:54-58). Offset from decomp
 * reloc_data.us.h:3714; the Makefile builds -DREGION_US. */
#define NDS_RELOC_LVALUE(offset) (*(uintptr_t *)(uintptr_t)(offset))
#define llKirbyMainMotionftKirbyAttack100Effect NDS_RELOC_LVALUE(0x1220u)

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonattack100.c"

#undef ftCommonAttack100StartProcUpdate
#undef ftCommonAttack100StartSetStatus
#undef ftCommonAttack100LoopKirbyUpdateEffect
#undef ftCommonAttack100LoopProcUpdate
#undef ftCommonAttack100LoopProcInterrupt
#undef ftCommonAttack100LoopSetStatus
#undef ftCommonAttack100EndSetStatus
#undef ftCommonAttack100StartCheckInterruptCommon
#undef llKirbyMainMotionftKirbyAttack100Effect
#undef NDS_RELOC_LVALUE
