/*
 * P2-6 Metal Mario (nFTKindMMario, kind 13) runtime.
 *
 * Source: decomp/BattleShip-main/decomp/src/ft/ftchar/ftmmario/ (storage only:
 * ftmmario.c + ftmmario.h, already imported by battleship_ftchar_data_slots.c),
 * FTData dFTMMarioData (ftdata.c:594-626), Main 206_MMarioMain.c (file 0xce),
 * MainMotion 205_MMarioMainMotion.c (file 0xcd), Model 300_MMarioModel.c
 * (file 0x12c, reloc_data_symbols.us.txt:230/229/324).
 *
 * Mechanical equivalence, all data-driven through the existing seams:
 * - Own Main 0xce + own MainMotion 0xcd + own Model 0x12c; reuses Mario
 *   ShieldPose + Special1/2/3 (ftdata.c:596-603). Cannot reuse the Mario owner
 *   with a material swap: the source ships a distinct model/texture file.
 * - knockback_resist_passive 30.0 (ftmanager.c:577-578), fallthrough Mario /
 *   NMario tornado init (:582-584). Compiled via battleship_ftmanager.c whole
 *   import; the port shim at reloc_backend_compat_shims.c:7241-7245 already
 *   carries the Mario/MMario/NMario tornado triple.
 * - is_metallic TRUE (206_MMarioMain.c:185 vs Mario FALSE) selects MDust over
 *   Sparks via the existing switch (ftmain.c:2742-2750), compiled via
 *   battleship_ftmain.c whole import. No metal shader is invented.
 * - Footsteps are MMMarioFoot (205_MMarioMainMotion.c:10+,
 *   gmsound.h:215 id 122, already in include/gm/gmsound.h:476); Mario uses
 *   MMarioFoot. Voice off is attribute data: dead/damage/smash/heavyget all
 *   VoiceEnd (206:192-199) plus is_have_voice 0 (:224).
 * - Status tables shared verbatim: dFTMainSpecialStatusDescs puts the Mario
 *   table in the MMario slot (ftmain.c:80,93).
 * - Entry reuses the Mario AppearR/AppearL pair and the Dokan pipe effect
 *   (ftcommonentry.c table :26 + switch :194-197; port arm in
 *   battleship_ftcommon_entry.c under NDS_P2_MMARIO; the old comment that
 *   deliberately skipped Metal is replaced by the real arm).
 *
 * Visual delta (recorded, not hidden): the owner generator has no metal
 * material path, so the Metal Mario owner is generated from its own
 * MMarioModel file with its own textures exactly as any fighter
 * (admit_fighter.py P2_O2R_ASSETS row). The DS metallic look is therefore the
 * source textures under the standard fighter lighting, not an env-mapped or
 * reflective approximation. A native metal material remains the visual owner's
 * work; this file claims only geometry/texture parity plus the attr/SFX gates
 * above.
 *
 * No behavior bodies live here: ftmmario/ holds no specials of its own.
 */
#include <ft/fighter.h>
