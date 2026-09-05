/* P2-7 item 4. Characters screen, source import: textual include of
 * decomp/BattleShip-main/decomp/src/mn/mndata/mncharacters.c whole,
 * following src/import/battleship_mnsoundtest.c (same mndata/ directory,
 * same slice) and src/import/battleship_sc1pbonusstage.c (scene TU with the
 * scene entry imported as ndsBase* and re-exported under its source name, so
 * a later measured DS arena rebudget has a seam and the diff stays
 * reviewable). The adapter is a verbatim pass-through; no behaviour invented
 * here.
 *
 * Source pins (docs/p2/P2-7-modes-meta.md RECORDS/HISCORE rows):
 * - Init reads scene_prev: from nSCKindData it pages to the saved
 *   characters_fkind, otherwise it runs the two demo fighters
 *   (mnCharactersInitVars :2348-2380).
 * - B writes the current page back to characters_fkind and calls
 *   lbBackupWrite before returning to nSCKindData
 *   (mnCharactersBackupFighterKind :2383-2388, called :2475).
 * - The per-fighter special-motion tables (dMNCharactersSpecialMotion*,
 *   :37-555) plus the common motion descs (:558-1027) drive the fighter
 *   preview; locked newcomers are gated on fighter_mask
 *   (mnCharactersCheckHaveFighterKind :2328-2345).
 *
 * Gated on NDS_P2_1P_GAME: the Makefile has no NDS_P2_MODES_META flag
 * (verified 2026-09-05; only NDS_P2_1P_GAME gates the P2-6/P2-7 imports), so
 * this rides the campaign flag like the item-5 SoundTest TU until P2-7 mints
 * its own gate.
 *
 * Shell status: the shell requires a native module rather than a source
 * scene. NDS_MENU_SHELL_SCREEN_* covers Title/Mode/VSMode/CSS/SSS/ItemSwitch/
 * VSOptions only, and src/nds/nds_menu_shell_vsoptions.c is the port-native
 * shape for a menu screen -- no native Characters module exists and the shell
 * cannot reach this kind today. Stops at the import by design; wiring is
 * P2-7 item 9 (Menu completion), not this slice.
 *
 * Shims vs unresolved, see handoff report:
 * - FTSTATUS_CHARACTERS_DEMO / FTSTATUS_CHARACTERS_NULL: shimmed below,
 *   verbatim from decomp ft/ftdef.h:56-57 (port include/ft/fighter.h carries
 *   the FTSTATUS_PRESERVE_* bits but not these two).
 * - MNCharactersMotion / MNCharactersSpecialMotion (decomp mn/mntypes.h:
 *   25-35): in include/mn/mntypes.h since the 2026-09-05 header widening;
 *   the local copies were removed with it.
 * - Menu enum nMNCharactersMotionKind* (decomp mn/mndef.h:23-69) and the
 *   audio ordinal nSYAudioBGMData: in include/mn/mndef.h and
 *   include/gm/gmsound.h since the same widening.
 * - The ~85 ll* rows (llMNCharacters*, llMNDataCommon*): staged in
 *   include/reloc_data.h (census 2026-09-05: 0 unstaged symbols in this TU).
 * - Resolved port-side, no action: gSCManagerBackupData.characters_fkind +
 *   fighter_mask (include/sc/scene.h), LBBACKUP_MASK_FIGHTER +
 *   ftManagerMakeFighter/ftManagerDestroyFighter (include/ft/fighter.h:143,
 *   :4171-4172), dSCSubsysFighterScales (include/ft/fighter.h:4160),
 *   lbBackupWrite (include/mn/menu.h:40, src/import/battleship_lbbackup.c),
 *   F_CLC_DTOR32/F_CST_DTOR32 (include/macros.h), nFTDemoStatusWin*/Lose
 *   (include/ft/fighter.h:167-172), ovl33 + ovl1_VRAM (DECLARE_OVL in
 *   include/sc/scene.h), SObj/CObj helpers (decomp sys/obj.h, which the port
 *   does not shadow).
 * - Collisions needing reported gating (not renamed away, behaviour must win):
 *   mnCharactersStartScene (adapter below) vs
 *   src/port/title_backend.c:414 NDS_SCENE_STUB.
 */

#if NDS_P2_1P_GAME

#include <stdint.h>
#include <PR/gbi.h>
#include <PR/ultratypes.h>
#include <ft/fighter.h>
#include <gm/gmsound.h>
#include <mn/menu.h>
#include <reloc_data.h>
#include <sc/scene.h>
#include <sys/audio.h>
#include <sys/controller.h>
#include <sys/obj.h>
#include <sys/objhelper.h>
#include <sys/objman.h>
#include <sys/rdp.h>
#include <sys/taskman.h>
#include <sys/video.h>

/* decomp ft/ftdef.h:56-57 verbatim. Port include/ft/fighter.h has the
 * FTSTATUS_PRESERVE_* bits but not the Characters demo wrapper. */
#ifndef FTSTATUS_CHARACTERS_DEMO
#define FTSTATUS_CHARACTERS_DEMO(status_id) (0x20000 + (status_id))
#endif
#ifndef FTSTATUS_CHARACTERS_NULL
#define FTSTATUS_CHARACTERS_NULL 0xA2C2A
#endif


#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

extern f32 syUtilsArcTan2(f32 y, f32 x);

#define mnCharactersStartScene ndsBaseMNCharactersStartScene
void ndsBaseMNCharactersStartScene(void);

#include "../../decomp/BattleShip-main/decomp/src/mn/mndata/mncharacters.c"

#undef mnCharactersStartScene

void mnCharactersStartScene(void)
{
    ndsBaseMNCharactersStartScene();
}

#endif /* NDS_P2_1P_GAME */
