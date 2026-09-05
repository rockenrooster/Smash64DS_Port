/* P2-6 step 7 (Boss). Master Hand finger-gun bullets.
 *
 * Source import: textual include of
 * decomp/BattleShip-main/decomp/src/wp/wpboss/wpbossbullet.c whole
 * (188 lines: dWPBossBulletNormal/HardWeaponDesc + explode/map/hit/hop/
 * reflector procs + Normal/HardMakeWeapon), following battleship_yoshi.c
 * (behavior verbatim; DS adapts only surrounding seams).
 * NOTE: brief path wp/wpchar/wpbossbullet.c exists nowhere; real file is
 * decomp/.../src/wp/wpboss/wpbossbullet.c (+ wpbossbullet.h decls above).
 *
 * Unified-owner rule (stated in battleship_sc1pgame_runtime.c): the include
 * owns every symbol it defines under its source name; no renamed private
 * copies. Fired by ftboss_status_4.c (decls there, bodies here).
 *
 * Shims vs unresolved, by reading (no compile per owner directive):
 * - WPYUBIBULLET_EXPLODE_LIFETIME / EXPLODE_SIZE / VEL_X / VEL_Y: shimmed
 *   below, verbatim from decomp wp/wpvars.h:116-119 (port include/wp/weapon.h
 *   carries nWPKindBulletNormal/Hard but not these).
 * - DObjGetStruct: same local macro as battleship_fox_blaster.c:11.
 * - llBossMainMotionBulletNormalWeaponAttributes /
 *   llBossMainMotionBulletHardWeaponAttributes: local externs. Real ids in
 *   decomp tools/reloc_data_symbols.us.txt:3882-3883 (0x774/0x7A8); port has
 *   no definition under include/ or src/ (header edit out of scope). Left
 *   unresolved at link, never stubbed; invented offsets = fabricated data.
 * - gFTDataBossMainMotion: owned by battleship_ftchar_data_slots.c (ftboss.c:7).
 * - Engine (wpManagerMakeWeapon, wpGetStruct, wpMain* , wpMap*, func_800269C0,
 *   efManagerSparkleWhiteMultiExplodeMakeEffect) comes from port weapon/effect
 *   seams; compile reveals any gap first.
 */

#if NDS_P2_1P_GAME

#include <ft/fighter.h>
#include <reloc_data.h>
#include <wp/weapon.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

/* decomp wp/wpvars.h:116-119 verbatim. Port include/wp/weapon.h lacks them. */
#ifndef WPYUBIBULLET_EXPLODE_LIFETIME
#define WPYUBIBULLET_EXPLODE_LIFETIME 6
#endif
#ifndef WPYUBIBULLET_EXPLODE_SIZE
#define WPYUBIBULLET_EXPLODE_SIZE 180.0F
#endif
#ifndef WPYUBIBULLETVEL_X
#define WPYUBIBULLETVEL_X 160.0F
#endif
#ifndef WPYUBIBULLETVEL_Y
#define WPYUBIBULLETVEL_Y -25.0F
#endif

/* Real offsets 0x774/0x7A8 per tools/reloc_data_symbols.us.txt:3882-3883;
 * port has no definition (header edit out of scope). Declared so the TU
 * compiles; link stays honestly open until orchestrator stages definitions. */
extern uintptr_t llBossMainMotionBulletNormalWeaponAttributes;
extern uintptr_t llBossMainMotionBulletHardWeaponAttributes;

#include "../../decomp/BattleShip-main/decomp/src/wp/wpboss/wpbossbullet.c"

#endif /* NDS_P2_1P_GAME */
