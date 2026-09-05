/* P2-6 step 8. Congratulations screen (per-fighter plates).
 *
 * Source import: textual include of
 * decomp/BattleShip-main/decomp/src/mn/mncommon/mncongra.c whole
 * (note: path is mn/mncommon/mncongra.c, not mn/mncongra/),
 * following src/import/battleship_mnoption.c (scene TU with the scene
 * entry imported as ndsBase* and re-exported under its source name, so a
 * later measured DS arena rebudget has a seam and the diff stays reviewable).
 * The adapter is a verbatim pass-through; no behaviour invented here.
 *
 * The include OWNS every symbol it defines under its source name
 * (unified-owner rule, see src/import/battleship_sc1pgame_runtime.c file doc).
 * No shims, no stubs in this TU.
 *
 * Reloc: no d<File>FileIDs table; dMNCongraPictures (:18-91) references 24
 * plate ids directly (12 fighters x Top/Bottom): llMNCongraMario/ Fox/
 * Donkey/ Samus/ Luigi/ Link/ Yoshi/ Captain/ Kirby/ Pikachu/ Purin/ Ness
 * Top+Bottom FileIDs. All 24 staged 2026-09-04 (reloc_data.h externs :812-
 * :1045 + NDS_MN_CONGRA_*_RELOC_SYMBOLS blocks). No unstaged file.
 *
 * Shims vs unresolved, see handoff report:
 * - MNCongraPicture struct (decomp mn/menu.h): NOT defined here; menu header
 *   owns it (link reveals).
 * - sMNCongraFighterKind / nFTKindMario default (:414) + nSCKind1PGame/
 *   nSCKindTitle (:417,:388): port ft/fighter.h + sc/scene.h carry them.
 * - nSYAudioVoiceAnnounceIncredible/Congra (:364): in include/gm/gmsound.h
 *   since the 2026-09-05 widening.
 * - func_800269C0_275C0 voice helper (:9,:362): NOT shimmed or stubbed;
 *   resolves via include/sys/audio.h like the mnmessage TU (link reveals).
 * - gc/lb/sy/reloc/ovl refs: left unresolved, no shims, no stubs.
 * - Collisions needing reported gating (not renamed away, behaviour must win):
 *   mnCongraStartScene (adapter below) vs
 *   src/port/title_backend.c:419 NDS_SCENE_STUB.
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

#define mnCongraStartScene ndsBaseMNCongraStartScene
void ndsBaseMNCongraStartScene(void);

#include "../../decomp/BattleShip-main/decomp/src/mn/mncommon/mncongra.c"

#undef mnCongraStartScene

void mnCongraStartScene(void)
{
    ndsBaseMNCongraStartScene();
}

#endif /* NDS_P2_1P_GAME */
