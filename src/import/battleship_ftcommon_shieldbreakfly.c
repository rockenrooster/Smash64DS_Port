#include <ef/effect.h>
#include <ft/fighter.h>
#include <gm/gmsound.h>
#include <sc/scene.h>
#include <sys/audio.h>

/* BattleShip's common-function header is intentionally not part of the port
 * ABI mirror, so declare the cross-TU links this source file reaches. */
void ftCommonShieldBreakFallSetStatus(GObj *fighter_gobj);
void ftCommonShieldBreakDownSetStatus(GObj *fighter_gobj);

/* The port keeps NULL-safe public entry seams because several imported combat
 * callers may reach them while a partially constructed fighter is being
 * diagnosed.  Rename only those two source entry points; their bodies remain
 * the behavioral authority once the guard in compat_shims has passed. */
#define ftCommonShieldBreakFlyCommonSetStatus \
    ndsBaseFTCommonShieldBreakFlyCommonSetStatus
#define ftCommonShieldBreakFlyReflectorSetStatus \
    ndsBaseFTCommonShieldBreakFlyReflectorSetStatus

void ndsBaseFTCommonShieldBreakFlyCommonSetStatus(GObj *fighter_gobj);
void ndsBaseFTCommonShieldBreakFlyReflectorSetStatus(GObj *fighter_gobj);

/* 1P scoring is outside the current VS slice.  The source branch is unreachable
 * in VS but must still link; with NDS_P2_1P_GAME the strong owner is the
 * included sc1pgame.c:720 (battleship_sc1pgame_runtime.c). ub8 as in the
 * source, so both owners have the same width. */
__attribute__((weak)) ub8 gSC1PGameBonusShieldBreaker;

/* Current effect-manager builds provide these from the imported source.  Weak
 * fallbacks keep non-effect-manager proof configurations linkable without
 * changing the gameplay state machine; all three source callers ignore the
 * return value. */
__attribute__((weak)) LBGenerator *efManagerShieldBreakMakeEffect(Vec3f *pos)
{
    (void)pos;
    return NULL;
}

__attribute__((weak)) LBParticle *efManagerYoshiEggExplodeMakeEffect(Vec3f *pos)
{
    (void)pos;
    return NULL;
}

__attribute__((weak)) GObj *efManagerReflectBreakMakeEffect(Vec3f *pos, s32 lr)
{
    (void)pos;
    (void)lr;
    return NULL;
}

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonshieldbreakfly.c"

#undef ftCommonShieldBreakFlyCommonSetStatus
#undef ftCommonShieldBreakFlyReflectorSetStatus
