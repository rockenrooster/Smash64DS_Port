/*
 * Bounded BattleShip ftcommondamage.c import.
 * Public names are remapped so project-owned damage seams remain in control.
 */
#include <ft/fighter.h>
#include <ef/effect.h>
#include <gm/gmsound.h>
#include <it/item.h>
#include <sys/obj.h>

typedef struct alSoundEffect alSoundEffect;

f32 syVectorMag3D(Vec3f *vec);
f32 syUtilsRandFloat(void);
void ftParamsUpdateFighterPartsTransformAll(DObj *joint);
sb32 mpCommonCheckFighterDamageCollision(GObj *fighter_gobj);
sb32 ftCommonWallDamageCheckGoto(GObj *fighter_gobj);
GObj *ftParamGetPlayerNumGObj(s32 player);
void ftPublicCommonCheck(GObj *fighter_gobj, f32 knockback,
                         sb32 is_force_curr_knockback);
void ifScreenFlashSetColAnimID(s32 colanim_id, s32 colanim_duration);
void ftKirbySpecialNDamageCheckLoseCopy(GObj *fighter_gobj);
#if NDS_P2_DONKEY
/* ftdonkeythrowfdamage.c, compiled by src/import/battleship_donkey.c.  Declared
 * here rather than in include/ft/fighter.h so the DK cargo ABI stays inside the
 * import TUs that own it, exactly as battleship_ftcommon_catch.c declares
 * ftDonkeyThrowFWaitSetStatus. */
void ftDonkeyThrowFDamageSetStatus(GObj *fighter_gobj);
#else
void ndsCompatFTDonkeyThrowFDamageSetStatus(GObj *fighter_gobj);
#endif

#ifndef FTCOMMON_DAMAGE_EFFECT_WAIT_LOW
#define FTCOMMON_DAMAGE_EFFECT_WAIT_LOW 0
#define FTCOMMON_DAMAGE_EFFECT_WAIT_MID_LOW 8
#define FTCOMMON_DAMAGE_EFFECT_WAIT_MID 5
#define FTCOMMON_DAMAGE_EFFECT_WAIT_MID_HIGH 3
#define FTCOMMON_DAMAGE_EFFECT_WAIT_HIGH 2
#define FTCOMMON_DAMAGE_EFFECT_WAIT_DEFAULT 1
#define FTCOMMON_DAMAGE_EFFECT_KNOCKBACK_LOW 120.0F
#define FTCOMMON_DAMAGE_EFFECT_KNOCKBACK_MID_LOW 150.0F
#define FTCOMMON_DAMAGE_EFFECT_KNOCKBACK_MID 200.0F
#define FTCOMMON_DAMAGE_EFFECT_KNOCKBACK_MID_HIGH 300.0F
#define FTCOMMON_DAMAGE_EFFECT_KNOCKBACK_HIGH 600.0F
#define FTCOMMON_DAMAGE_SAKURAI_KNOCKBACK_LOW 32.0F
#define FTCOMMON_DAMAGE_SAKURAI_ANGLE_LOW_GR 0.0F
#define FTCOMMON_DAMAGE_SAKURAI_ANGLE_HIGH_GD 42.5F
#define FTCOMMON_DAMAGE_SAKURAI_ANGLE_HIGH_GR \
    F_CLC_DTOR32(FTCOMMON_DAMAGE_SAKURAI_ANGLE_HIGH_GD)
#define FTCOMMON_DAMAGE_SAKURAI_ANGLE_DEFAULT_AR F_CLC_DTOR32(43.0F)
#define FTCOMMON_DAMAGE_LEVEL_HITSTUN_LOW 12.0F
#define FTCOMMON_DAMAGE_LEVEL_HITSTUN_MID 24.0F
#define FTCOMMON_DAMAGE_LEVEL_HITSTUN_HIGH 32.0F
#define FTCOMMON_DAMAGE_PUBLIC_REACT_GASP_ANGLE_LOW F_CLC_DTOR32(75.0F)
#define FTCOMMON_DAMAGE_PUBLIC_REACT_GASP_ANGLE_HIGH F_CLC_DTOR32(115.0F)
#define FTCOMMON_DAMAGE_PUBLIC_REACT_GASP_KNOCKBACK_MUL 0.8F
#define FTCOMMON_DAMAGE_KNOCKBACK_VERYHIGH 160.0F
#define FTCOMMON_DAMAGE_FIGHTER_FLYTOP_ANGLE_LOW F_CLC_DTOR32(70.0F)
#define FTCOMMON_DAMAGE_FIGHTER_FLYTOP_ANGLE_HIGH F_CLC_DTOR32(110.0F)
#define FTCOMMON_DAMAGE_FIGHTER_FLYROLL_DAMAGE_MIN 100
#define FTCOMMON_DAMAGE_FIGHTER_FLYROLL_RANDOM_CHANCE 0.5F
#define FTCOMMON_DAMAGE_FIGHTER_DAMAGEVOICE_MIN 80.0F
#define FTCOMMON_DAMAGE_FIGHTER_PLAYERTAG_KNOCKBACK_MIN 130.0F
#define FTCOMMON_DAMAGE_FIGHTER_PLAYERTAG_HIDE_FRAMES 10
#endif

#ifndef nFTDonkeyStatusThrowFStart
#define nFTDonkeyStatusThrowFStart (nFTCommonStatusSpecialStart + 15)
#define nFTDonkeyStatusThrowFEnd (nFTDonkeyStatusThrowFStart + 8)
#endif

#define dFTCommonDamageStatusGroundIDs dNDSBaseFTCommonDamageStatusGroundIDs
#define dFTCommonDamageStatusAirIDs dNDSBaseFTCommonDamageStatusAirIDs
#define ftCommonDamageSetDustEffectInterval \
    ndsBaseFTCommonDamageSetDustEffectInterval
#define ftCommonDamageUpdateDustEffect ndsBaseFTCommonDamageUpdateDustEffect
#define ftCommonDamageDecHitStunSetPublic \
    ndsBaseFTCommonDamageDecHitStunSetPublic
#define ftCommonDamageCommonProcUpdate \
    ndsBaseFTCommonDamageCommonProcUpdate
#define ftCommonDamageAirCommonProcUpdate \
    ndsBaseFTCommonDamageAirCommonProcUpdate
#define ftCommonDamageCheckSetInvincible \
    ndsBaseFTCommonDamageCheckSetInvincible
#define ftCommonDamageSetStatus ndsBaseFTCommonDamageSetStatus
#define ftCommonDamageCommonProcInterrupt \
    ndsBaseFTCommonDamageCommonProcInterrupt
#define ftCommonDamageAirCommonProcInterrupt \
    ndsBaseFTCommonDamageAirCommonProcInterrupt
#define ftCommonDamageFlyRollUpdateModelPitch \
    ndsBaseFTCommonDamageFlyRollUpdateModelPitch
#define ftCommonDamageCommonProcPhysics \
    ndsBaseFTCommonDamageCommonProcPhysics
#define ftCommonDamageCommonProcLagUpdate \
    ndsBaseFTCommonDamageCommonProcLagUpdate
#define func_ovl3_80140934 ndsBaseFTCommonDamageUnused
#define ftCommonDamageAirCommonProcMap \
    ndsBaseFTCommonDamageAirCommonProcMap
#define ftCommonDamageGetKnockbackAngle \
    ndsBaseFTCommonDamageGetKnockbackAngle
#define ftCommonDamageGetDamageLevel ndsBaseFTCommonDamageGetDamageLevel
#define ftCommonDamageSetPublic ndsBaseFTCommonDamageSetPublic
#define ftCommonDamageCheckElementSetColAnim \
    ndsBaseFTCommonDamageCheckElementSetColAnim
#define ftCommonDamageCheckMakeScreenFlash \
    ndsBaseFTCommonDamageCheckMakeScreenFlash
#define ftCommonDamageCheckCatchResist \
    ndsBaseFTCommonDamageCheckCatchResist
#define ftCommonDamageUpdateCatchResist \
    ndsBaseFTCommonDamageUpdateCatchResist
#define ftCommonDamageCheckCaptureKeepHold \
    ndsBaseFTCommonDamageCheckCaptureKeepHold
#define ftCommonDamageInitDamageVars ndsBaseFTCommonDamageInitDamageVars
#define ftCommonDamageGotoDamageStatus ndsBaseFTCommonDamageGotoDamageStatus
#define ftCommonDamageUpdateDamageColAnim \
    ndsBaseFTCommonDamageUpdateDamageColAnim
#define ftCommonDamageSetDamageColAnim \
    ndsBaseFTCommonDamageSetDamageColAnim
#define ftCommonDamageUpdateMain ndsBaseFTCommonDamageUpdateMain
#if !NDS_P2_DONKEY
/* Without Donkey Kong in the build `battleship_donkey.c` is not compiled, so
 * `ftdonkeythrowfdamage.c` supplies no definition and the reference below has
 * to resolve somewhere: the compat seam keeps the TU linkable. It is NOT a
 * behavioral fallback -- the only caller, ftCommonDamageUpdateCatchResist, is
 * reachable with knockback only for a Donkey Kong carrying a fighter or a
 * heavy item (every other fighter's ftCommonDamageCheckCatchResist arm implies
 * the zero-knockback/hitlag test that UpdateCatchResist re-applies, which takes
 * the colour-anim branch instead).  With NDS_P2_DONKEY=1 the source setter is
 * linked and must be called directly: routing it through
 * ftCommonDamageGotoDamageStatus dropped DK out of the cargo ladder into an
 * ordinary Damage status while `catch_gobj` still pointed at the shouldered
 * victim -- board row P2-3r10. */
#define ftDonkeyThrowFDamageSetStatus \
    ndsCompatFTDonkeyThrowFDamageSetStatus
#endif

void ndsBaseFTCommonDamageUpdateCatchResist(GObj *fighter_gobj);
void ndsBaseFTCommonDamageFlyRollUpdateModelPitch(GObj *fighter_gobj);
void ndsBaseFTCommonDamageSetDamageColAnim(GObj *fighter_gobj);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommondamage.c"

#undef dFTCommonDamageStatusGroundIDs
#undef dFTCommonDamageStatusAirIDs
#undef ftCommonDamageSetDustEffectInterval
#undef ftCommonDamageUpdateDustEffect
#undef ftCommonDamageDecHitStunSetPublic
#undef ftCommonDamageCommonProcUpdate
#undef ftCommonDamageAirCommonProcUpdate
#undef ftCommonDamageCheckSetInvincible
#undef ftCommonDamageSetStatus
#undef ftCommonDamageCommonProcInterrupt
#undef ftCommonDamageAirCommonProcInterrupt
#undef ftCommonDamageFlyRollUpdateModelPitch
#undef ftCommonDamageCommonProcPhysics
#undef ftCommonDamageCommonProcLagUpdate
#undef func_ovl3_80140934
#undef ftCommonDamageAirCommonProcMap
#undef ftCommonDamageGetKnockbackAngle
#undef ftCommonDamageGetDamageLevel
#undef ftCommonDamageSetPublic
#undef ftCommonDamageCheckElementSetColAnim
#undef ftCommonDamageCheckMakeScreenFlash
#undef ftCommonDamageCheckCatchResist
#undef ftCommonDamageUpdateCatchResist
#undef ftCommonDamageCheckCaptureKeepHold
#undef ftCommonDamageInitDamageVars
#undef ftCommonDamageGotoDamageStatus
#undef ftCommonDamageUpdateDamageColAnim
#undef ftCommonDamageSetDamageColAnim
#undef ftCommonDamageUpdateMain
#undef ftDonkeyThrowFDamageSetStatus
