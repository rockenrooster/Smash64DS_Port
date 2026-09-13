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
 *   llBossMainMotionBulletHardWeaponAttributes: local externs. Real offsets
 *   in decomp include/reloc_data.us.h:3729-3730 (0x774/0x7A8), defined by the
 *   BossMainMotion staging rows (admit_fighter.py --fighter boss:
 *   NDS_BOSS_MAIN_MOTION_RELOC_SYMBOLS in include/reloc_data.h, defined in
 *   src/port/diagnostics_mp_taskman_state.c); never stubbed, never invented.
 * - gFTDataBossMainMotion: owned by battleship_ftchar_data_slots.c (ftboss.c:7).
 * - Engine (wpManagerMakeWeapon, wpGetStruct, wpMain*, wpMap*): prototypes in
 *   include/wp/weapon.h (wpMainReflectorRotateWeaponModel and
 *   wpMapTestAllCheckCollEnd added there 2026-09-05, decomp wpmain.h:41 /
 *   wpmap.h:28); the boss-bullet proc table itself is decomp
 *   wp/wpboss/wpbossbullet.h:8-15, also published there.
 * - nSYAudioFGMExplodeL/S (include/gm/gmsound.h) + func_800269C0_275C0
 *   (include/sys/audio.h:71, body in src/port/reloc_backend_compat_shims.c):
 *   same audio seam as battleship_item_capsule.c:24,28.
 * - syVectorRotateAbout3D (decomp sys/vector.h:42) and
 *   efManagerSparkleWhiteMultiExplodeMakeEffect (decomp ef/efmanager.h:64):
 *   local externs below, same seams as battleship_item_iwark.c:93 /
 *   battleship_item_capsule.c:73; proven present in the linked ROM text.
 */

#if NDS_P2_1P_GAME

#include <ef/effect.h>
#include <ft/fighter.h>
#include <gm/gmsound.h>
#include <reloc_data.h>
#include <sys/audio.h>
#include <wp/weapon.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

/* Real offsets 0x774/0x7A8 per decomp include/reloc_data.us.h:3729-3730;
 * defined by the BossMainMotion staging rows (see above). */
extern uintptr_t llBossMainMotionBulletNormalWeaponAttributes;
extern uintptr_t llBossMainMotionBulletHardWeaponAttributes;

/* decomp sys/vector.h:42. Same seam as battleship_item_iwark.c:93. */
extern Vec3f *syVectorRotateAbout3D(Vec3f *dst, Vec3f *dir, f32 angle);

/* decomp ef/efmanager.h:64. Same seam as battleship_item_capsule.c:73. */
extern LBParticle *efManagerSparkleWhiteMultiExplodeMakeEffect(Vec3f *pos);

#include "../../decomp/BattleShip-main/decomp/src/wp/wpboss/wpbossbullet.c"

#endif /* NDS_P2_1P_GAME */
