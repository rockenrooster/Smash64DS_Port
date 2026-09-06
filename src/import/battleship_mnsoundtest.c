/* P2-7 item 5. Sound Test screen, source import: textual include of
 * decomp/BattleShip-main/decomp/src/mn/mndata/mnsoundtest.c whole,
 * following src/import/battleship_sc1pbonusstage.c (scene TU with the scene
 * entry imported as ndsBase* and re-exported under its source name, so a
 * later measured DS arena rebudget has a seam and the diff stays reviewable).
 * The adapter is a verbatim pass-through; no behaviour invented here.
 *
 * Source pins (docs/p2/P2-7-modes-meta.md OPTIONS rows):
 * - 3 rows Music/Sound/Voice (:692, option enum decomp mn/mndef.h:98-108);
 *   U/D walks rows with wraparound (:789-832), L/R walks the row's ID with
 *   wraparound (:833-934).
 * - A plays the row's ID (:954-977: BGM via syAudioPlayBGM, Sound/Voice via
 *   func_800266A0_272A0 + func_800269C0_275C0); Z stops all (:978-982);
 *   START fades out over 120 tics (:983-988); B returns to nSCKindData
 *   (:996-1005).
 * - ID tables dMNSoundTestMusicIDs :40-87, dMNSoundTestSoundIDs :90-286,
 *   dMNSoundTestVoiceIDs :288-585.
 * - SoundTest unlock gating (bonus 10/10 for all 12) lives in
 *   sc1pmanager.c/sc1pbonusstage.c + mndata.c, NOT here; this TU is the
 *   screen only.
 *
 * Available in the VS shell and campaign through the source-menu pump.
 * Its normal entry remains the campaign-gated DATA menu.
 *
 * Shims vs unresolved, see handoff report:
 * - Menu enum nMNSoundTestOption* (decomp mn/mndef.h:98-108): in
 *   include/mn/mndef.h since the 2026-09-05 header widening.
 * - ID-table audio ordinals: every one of the 483 nSYAudio* names the three
 *   tables reference is in include/gm/gmsound.h (checked 2026-09-05); the
 *   FGM PACK behind them is the open half -- the sound-effect table names
 *   ids the pack does not render yet, owned by check-fgm-pack-coverage.py.
 * - syAudioSetBGMVolumeFade (decomp sys/audio.h:212, audio.c:1315): owned by
 *   the DS mixer since 2026-09-05 (src/nds/nds_audio_bgm.c, an integer ramp
 *   stepped from the per-frame BGM update); declared in include/sys/audio.h.
 * - <lb/library.h> (:4): decomp src/lb/library.h, on the include path
 *   (Makefile INCLUDES carries the decomp src dir).
 * - The ~15 ll* rows (llIFCommon*, llMNDataCommon*, llMNCommon*,
 *   llMNSoundTest*): staged in include/reloc_data.h (census: 0 unstaged
 *   symbols in this TU).
 * - Resolved port-side, no action: syAudioPlayBGM/syAudioStopBGMAll/
 *   syAudioSetBGMVolume + func_800266A0_272A0/func_800269C0_275C0 (port
 *   include/sys/audio.h) and func_80017EC0 (opening_movie_backend.c:4479).
 * - Collisions needing reported gating (not renamed away, behaviour must win):
 *   mnSoundTestStartScene (adapter below) vs
 *   src/port/title_backend.c:429 NDS_SCENE_STUB.
 */

#if NDS_P2_MENU_SHELL || NDS_P2_1P_GAME

#include <stdint.h>
#include <PR/gbi.h>
#include <PR/ultratypes.h>
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

#define mnSoundTestStartScene ndsBaseMNSoundTestStartScene
void ndsBaseMNSoundTestStartScene(void);

/* Exact source header decomp mn/mndata/mnsoundtest.h:39-40. The taskman
 * setup (:638-680) references both before their definitions (:1717+). */
extern void mnSoundTestFuncStart(void);
extern void mnSoundTestFuncLights(Gfx **dls);

/* The port headers above supply this scene's LB/GM declarations. */
#define _LIBRARY_H_
#include "../../decomp/BattleShip-main/decomp/src/mn/mndata/mnsoundtest.c"

#undef mnSoundTestStartScene

void mnSoundTestStartScene(void)
{
    ndsBaseMNSoundTestStartScene();
}

#endif /* NDS_P2_MENU_SHELL || NDS_P2_1P_GAME */
