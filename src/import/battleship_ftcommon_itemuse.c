/* The fighter half of USING a held item: shooting one and swinging one.
 * Includes decomp src/ft/ftcommon/ftcommonitemshoot.c and ftcommonitemswing.c
 * whole, the way battleship_ftcommon_itemthrow.c includes its own file.
 *
 * Third instance of the same shape in one night, and by now the pattern is the
 * finding rather than the fix. The Ray Gun's ammo maker, the Fire Flower's
 * flame and the Star Rod's star were all ported, all correct, and all
 * unreachable: `ftCommonItemShootSetStatus` and `ftCommonItemSwingSetStatus`
 * were empty compat shims, and the three ProcUpdate callbacks the status table
 * names were weak NDS_INACTIVE_STATUS_STUBs. So a fighter holding a Ray Gun
 * pressed A and nothing came out.
 *
 * The nine kinds that shoot split cleanly on this: the six that fire
 * themselves -- Lizardon's flame, Kamex's hydro, Nyars' coin, Dogas' smog,
 * Spear's swarm, Starmie's swift -- all worked already, because their own
 * ProcUpdate is the trigger and no fighter seam sits in the way. Only the
 * three a FIGHTER fires were mute.
 *
 * Gated on NDS_P2_ITEM_CORE: with no items there is nothing to shoot, and the
 * weak stubs remain the right answer. The stubs it replaces are weak, so a
 * strong definition here wins the link; the two SetStatus shims are not, and
 * are fenced by the same condition in reloc_backend_compat_shims.c.
 */
#if NDS_P2_ITEM_CORE

#include <ft/fighter.h>
#include <gm/gmsound.h>
#include <it/item.h>
#include <nds/nds_obj_anim.h>

/* NOT <sys/audio.h>. ftcommonitemswing.c:4 declares func_800269C0_275C0
 * itself, returning alSoundEffect*, and the port header declares it void* --
 * two declarations of one symbol that the compiler rejects. The included
 * source's own declaration is the one that has to stand, so the port header
 * stays out and only the type it needs is forward-declared. */
typedef struct alSoundEffect alSoundEffect;
/* ftcommonitemshoot.c uses it and does not declare it; ftcommonitemswing.c
 * declares it at :4. Both are included here, shoot first, so the declaration
 * has to come from this file -- spelled exactly as the swing file spells it so
 * the two agree. */
extern alSoundEffect *func_800269C0_275C0(u16 fgm_id);

/* The effect-kind names come from the decomp header through the include path,
 * exactly as reloc_backend_compat_shims.c reaches it for nEFKindImpactWave --
 * forty-five enumerators is not a list worth copying to use three.
 *
 * ft/ftcommon.h and ft/ftcommondata.h CANNOT be reached the same way: both
 * pull decomp ft/ftdef.h, which redeclares every enumerator the port's own
 * ft/fighter.h already defines. So the handful of constants and the one table
 * this file needs are transcribed below with their source lines. */
#include <ef/efdef.h>

/* decomp ft/ftcommon.h:208-221. */
#define FTCOMMON_HARISENSWING_SCALE_RESET_WAIT 2
#define FTCOMMON_HARISENSWING_SCALE_HIT 1.5F
#define FTCOMMON_LGUNSHOOT_AMMO_SPAWN_OFF_X 0.0F
#define FTCOMMON_LGUNSHOOT_AMMO_SPAWN_OFF_Y 60.0F
#define FTCOMMON_LGUNSHOOT_AMMO_SPAWN_OFF_Z 180.0F
#define FTCOMMON_FIREFLOWERSHOOT_EFFECT_SPAWN_INT 12
#define FTCOMMON_FIREFLOWERSHOOT_RELEASE_LAG 20
#define FTCOMMON_FIREFLOWERSHOOT_AMMO_INDEX_MAX 8
#define FTCOMMON_FIREFLOWERSHOOT_AMMO_INDEX_LOOP 5
#define FTCOMMON_FIREFLOWERSHOOT_AMMO_SPAWN_OFF_X 60.0F
#define FTCOMMON_FIREFLOWERSHOOT_AMMO_SPAWN_OFF_Y 100.0F
#define FTCOMMON_FIREFLOWERSHOOT_AMMO_SPAWN_OFF_Z 0.0F

/* decomp ft/fttypes.h:647-650 and ft/ftcommondata.c:326-345. Four swing kinds
 * x four swing types; only the Bat's smash (75) and the Harisen's three
 * non-dash swings (200) differ from 100. */
typedef struct FTItemSwing {
    u32 anim_speed : 10;
} FTItemSwing;

FTItemSwing dFTCommonDataItemSwingAnimSpeeds[] = {
    100, 100, 100, 100, /* Sword  1/3/4/Dash */
    100, 100,  75, 100, /* Bat    1/3/4/Dash */
    200, 200, 200, 100, /* Harisen 1/3/4/Dash */
    100, 100, 100, 100  /* StarRod 1/3/4/Dash */
};

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif
#ifndef U16_MAX
#define U16_MAX 0xffffu
#endif

/* The three item makers these procs fire, each defined in its own item TU. */
extern void itLGunMakeAmmo(GObj *fighter_gobj, Vec3f *pos);
extern void itFFlowerShootFlame(GObj *fighter_gobj, Vec3f *pos, s32 index,
                                s32 ammo_sub);
extern void itStarRodMakeStar(GObj *fighter_gobj, Vec3f *pos, ub8 is_smash);
extern void itHarisenCommonSetScale(GObj *item_gobj, f32 scale);
extern void gcSetAnimSpeed(GObj *gobj, f32 anim_speed);

/* Defined further down the included source than their first use. */
void ftCommonLGunShootAirSwitchStatusGround(GObj *fighter_gobj);
void ftCommonLGunShootSwitchStatusAir(GObj *fighter_gobj);
void ftCommonFireFlowerShootAirSwitchStatusGround(GObj *fighter_gobj);
void ftCommonFireFlowerShootSwitchStatusAir(GObj *fighter_gobj);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonitemshoot.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonitemswing.c"

#endif /* NDS_P2_ITEM_CORE */
