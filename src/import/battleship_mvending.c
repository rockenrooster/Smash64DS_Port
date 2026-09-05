/* P2-6 step 8 tail. 1P ending movie, source import: textual include of
 * decomp/BattleShip-main/decomp/src/mv/mvending/mvending.c whole (560 lines:
 * dMVEndingFileIDs :87, lights, room/fighter/camera builders, mvEndingFuncRun
 * :468, mvEndingFuncStart :508, mvEndingStartScene :551), following
 * src/import/battleship_sc1pbonusstage.c / battleship_mnoption.c (scene TU
 * with the scene entry imported as ndsBase* and re-exported under its source
 * name, so a later measured DS arena rebudget has a seam and the diff stays
 * reviewable). The adapter is a verbatim pass-through; no behaviour invented
 * here.
 *
 * Unified-owner rule (stated in battleship_sc1pgame_runtime.c, followed
 * here): the include OWNS every symbol it defines under its source name --
 * no renamed private copies. The only rename is the scene entry, imported as
 * ndsBase* and re-exported under its source name. Gated on NDS_P2_1P_GAME
 * like the rest of the P2-6 step 8 tail.
 *
 * Reloc files (dMVEndingFileIDs mvending.c:87): MVCommon + MVEnding 0x4c.
 * Both staged 2026-09-04 by scripts/menus/stage_reloc_file.py; manifests in
 * include/reloc_data.h (NDS_MV_ENDING_RELOC_SYMBOLS: one camera AnimJoint
 * llMVEndingOperatorCamAnimJoint); definitions in
 * src/port/diagnostics_mp_taskman_state.c (llMVCommonFileID :64,
 * llMVEndingFileID = 0x4c :444). No file this TU loads directly is unstaged.
 * Per-fighter payload is indirect: mvEndingFuncStart :530 calls
 * ftManagerSetupFilesAllKind(fkind), which pulls that fighter's FTData
 * closure from decomp ft/ftdata.c (dFTMarioData :348 pattern): Main,
 * MainMotion, Model, ShieldPose, Special1/2/3 (+Special4 where the kind has
 * one), plus that fighter's ~100 llFT<Name>Anim*FileID motion rows. Those
 * model/animation files are NOT sprite records, so the sprite tool cannot
 * normalize them; the orchestrator stages them per fighter with
 * python scripts/menus/stage_reloc_file.py --file NAME --list
 * NDS_1P_RELOC_FILES (12 playable closures: Mario, Fox, Donkey, Samus,
 * Luigi, Link, Yoshi, Captain, Kirby, Pikachu, Purin, Ness).
 *
 * Shims vs unresolved, by reading (no compile per owner directive):
 * - No local shims. BGM/FGM the TU needs already exist in port
 *   include/gm/gmsound.h (nSYAudioBGMEnding = 38, nSYAudioFGMDoorClose = 20);
 *   nSCKindStaffroll exists in include/sc/scene.h; ovl54_BSS_END + ovl1_VRAM
 *   are covered by DECLARE_OVL in include/sc/scene.h; func_80017EC0 is in
 *   include/sys/objhelper.h:92; nFTDemoStatusFigureDropped is in
 *   include/ft/fighter.h:176.
 * - Left unresolved at link (never stubbed): func_800269C0_275C0
 *   (called :495; port include/sys/audio.h:92 declares it per the
 *   stage-clear precedent, definition lives in the audio seam), and the
 *   per-fighter model/animation file definitions above until the
 *   orchestrator stages them. Everything else the TU calls is
 *   port-provided: gc*/lbReloc*/sy*/ef*/ftManager*/scSubsys*/syAudioPlayBGM.
 * - Collisions needing reported gating (not renamed away, behaviour must
 *   win): mvEndingStartScene (adapter below) vs
 *   src/port/title_backend.c:463 NDS_SCENE_STUB.
 */

#if NDS_P2_1P_GAME

#include <stdint.h>
#include <PR/gbi.h>
#include <PR/os.h>
#include <PR/ultratypes.h>
#include <ft/fighter.h>
#include <gm/gmsound.h>
#include <mv/movie.h>
#include <reloc_data.h>
#include <sc/scene.h>
#include <sys/rdp.h>
#include <sys/taskman.h>
#include <sys/video.h>

#define mvEndingStartScene ndsBaseMVEndingStartScene
void ndsBaseMVEndingStartScene(void);

#include "../../decomp/BattleShip-main/decomp/src/mv/mvending/mvending.c"

#undef mvEndingStartScene

void mvEndingStartScene(void)
{
    ndsBaseMVEndingStartScene();
}

#endif /* NDS_P2_1P_GAME */
