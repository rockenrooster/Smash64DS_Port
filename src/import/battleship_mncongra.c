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
 *
 * DS platform entry (1P build): source mnCongraStartScene :402-432 brackets
 * syVideoInit/syTaskmanStartTask with N64 framebuffer clear loops to
 * 0x80400000 (:407-409, :429-431). Those loops NEVER run here -- mapping the
 * address macro to a DS buffer and running them would overwrite RAM. The
 * wrapper below replaces only the platform start: the fighter-kind selection
 * switch (:411-424) is preserved verbatim, the three video slots alias
 * &gSYFramebufferSets[0] (like mnTitleStartScene) with the DS z-buffer extent,
 * syVideoInit runs, and the task starts on ndsTaskmanArenaStart/Size with the
 * ORIGINAL mnCongraFuncStart and mnCongraFuncDraw. Blackout needs no work
 * here: source mnCongraFuncDraw :377 latches SYVIDEO_FLAG_BLACKOUT once and
 * the shared video seam (battleship_sys_video.c) mirrors that onto the DS
 * brightness latch, while syVideoInit clears it so the Title entry after the
 * 5-frame wait (:381-392) recovers. Source gameflow (FuncStart plates,
 * FuncDraw 5-frame BLACKOUT wait to Title) is untouched; user-facing plate
 * rendering is still owed (source-derived, no invented bitmap).
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

extern void *ndsTaskmanArenaStart(void);
extern size_t ndsTaskmanArenaSize(void);

#define mnCongraStartScene ndsBaseMNCongraStartScene
void ndsBaseMNCongraStartScene(void);

/* Exact source header decomp mn/mncommon/mncongra.h:12-14. The taskman
 * setup (:125-166) references all three before their definitions
 * (:259, :369, :396). */
extern void mnCongraFuncStart(void);
extern void mnCongraFuncDraw(void);
extern void mnCongraFuncLights(Gfx **dls);

#include "../../decomp/BattleShip-main/decomp/src/mn/mncommon/mncongra.c"

#undef mnCongraStartScene

void mnCongraStartScene(void)
{
    SYTaskmanSetup setup;

    switch (gSCManagerSceneData.scene_prev)
    {
    default:
        sMNCongraFighterKind = nFTKindMario;
        break;

    case nSCKind1PGame:
        sMNCongraFighterKind = gSCManagerSceneData.fkind;
        break;

    case nSCKindDebugBattle:
        sMNCongraFighterKind = gSCManagerTransferBattleState.players[0].fkind;
        break;
    }
    dMNCongraVideoSetup.framebuffers[0] = &gSYFramebufferSets[0];
    dMNCongraVideoSetup.framebuffers[1] = &gSYFramebufferSets[0];
    dMNCongraVideoSetup.framebuffers[2] = &gSYFramebufferSets[0];
    dMNCongraVideoSetup.zbuffer = SYVIDEO_ZBUFFER_START(320, 240, 0, 10, u16);
    syVideoInit(&dMNCongraVideoSetup);

    setup = dMNCongraTaskmanSetup;
    setup.scene_setup.arena_start = ndsTaskmanArenaStart();
    setup.scene_setup.arena_size = ndsTaskmanArenaSize();
    setup.func_start = mnCongraFuncStart;
    syTaskmanStartTask(&setup);
}

#endif /* NDS_P2_1P_GAME */
