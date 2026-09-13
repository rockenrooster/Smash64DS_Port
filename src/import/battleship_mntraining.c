/* P2-7 item 3. Training-mode character select, source import: textual
 * include of decomp/BattleShip-main/decomp/src/mn/mnplayers/
 * mnplayers1ptraining.c whole, following src/import/battleship_mnsoundtest.c
 * (P2-7 item 5 menu TU: scene entry imported as ndsBase* and re-exported
 * under its source name, so a later measured DS arena rebudget has a seam
 * and the diff stays reviewable). The adapter is a verbatim pass-through;
 * no behaviour invented here.
 *
 * Source pins (docs/p2/P2-7-modes-meta.md TRAINING rows):
 * - select writes training_man/com fkind+costume via
 *   mnPlayers1PTrainingSetSceneData (:2911-2918), called on B-back to
 *   1PMode (:2046-2056), 5-minute idle timeout to Title and START-ready
 *   proceed to Maps (both in mnPlayers1PTrainingFuncRun, :2938-2983:
 *   Title at :2945, Maps at :2963).
 * - MAN slot keeps scene training_man fkind/costume (:2997-3008), COM slot
 *   rolls a random unlocked fighter when training_com_fkind is Null
 *   (:3078-3102); both slots init :3058-3112; CSS idles back to Title
 *   after 5 min without input (ReturnTic = total + I_MIN_TO_TICS(5),
 *   :3065, refreshed on any input :2953-2955).
 * - FuncStart announces TrainingMode voice + BattleSelect BGM (:3204-3209);
 *   taskman setup :3216-3267 (ovl28 arena, GCCommonKindPlayerSelect proc).
 *
 * Gated on NDS_P2_1P_GAME: the Makefile defines no NDS_P2_TRAINING flag
 * (verified 2026-09-05: only NDS_P2_1P_GAME at Makefile:700), so this rides
 * the campaign flag with its scene sibling battleship_sc1ptrainingmode.c
 * until P2-7 mints its own gate.
 *
 * The source 1P Mode submenu reaches nSCKindPlayers1PTraining through the
 * registered source-menu pump when NDS_P2_1P_GAME is enabled.
 *
 * Shims vs unresolved, checked against port headers (an earlier
 * revision of this comment claimed local shims; there are none -- the port
 * headers below carry everything, and duplicates here would not compile):
 * - MNPlayersSlotTraining (decomp mn/mntypes.h:82-...): port-PROVIDED,
 *   include/mn/mntypes.h:82-131 verbatim (VS-slot shape minus shade plus
 *   the u16 unk_0xAE pad).
 * - nSYAudioVoiceAnnounceTrainingMode (= 530): port-PROVIDED,
 *   include/gm/gmsound.h:742. No value macro here.
 * - ll* rows: NONE unresolved. dMNPlayers1PTrainingFileIDs (:17-27) needs
 *   MNPlayersCommon / MNPlayers1PMode / MNCommon / FTEmblemSprites /
 *   MNSelectCommon / MNPlayersGameModes / MNPlayersPortraits /
 *   MNPlayersSpotlight FileIDs; all are staged in include/reloc_data.h
 *   (same 8-file shape the landed VS CSS import links today).
 * - Resolved port-side, no action: syUtilsRandTimeUCharRange (COM random
 *   roll :3086; port-provided via existing source imports, extern-declared
 *   below like src/nds/nds_menu_shell_core.c:42), ftParamGetCostumeCommonID
 *   (port src/port/reloc_backend_compat_shims.c:16271),
 *   scSubsysFighterGetLightAngleX/Y (port include/ft/fighter.h:4813),
 *   scSubsysFighterSetLightParams + scSubsysController* (port
 *   include/sc/scene.h:561-569), lbReloc, lbCommon, gc, syVideo, syTaskman,
 *   sys-audio/func_800266A0/func_800269C0 (same providers as the landed
 *   mnplayersvs import), dLBCommonFuncMatrixList (extern-declared below),
 *   efManagerInitEffects (extern-declared below, same as
 *   battleship_mnplayersvs.c:39).
 * - Collisions needing reported gating (not renamed away, behaviour must
 *   win): mnPlayers1PTrainingStartScene (adapter below) vs
 *   src/port/title_backend.c:448 NDS_SCENE_STUB, already gated
 *   #if !NDS_P2_1P_GAME, so this TU wins exactly when the flag is on.
 */

#if NDS_P2_1P_GAME

#include <stdint.h>
#include <PR/gbi.h>
#include <PR/ultratypes.h>
#include <ft/fighter.h>
#include <gm/gmsound.h>
#include <if/interface.h>
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

/* decomp mn/mntypes.h:82-... verbatim. Port include/mn/mntypes.h carries
 * only MNPlayersSlotVS; field-for-field this is the VS slot minus shade
 * plus a u16 pad, so reusing the VS struct would mislay every trailing
 * field. When the port header gains it, delete this block. */

/* decomp gm/gmsound.h:627, ordinal 530 under REGION_US (see file header).
 * Macro, not a gameplay stub: only selects which announcer ID is requested. */

extern sb32 (*dLBCommonFuncMatrixList[])(void);
extern void efManagerInitEffects(void);
extern s32 syUtilsRandTimeUCharRange(s32 range);

/* Exact source header decomp mn/mnplayers/mnplayers1ptraining.h (each used
 * before its definition; campaign-build-1 error lines :126-:2051). */
extern sb32 mnPlayers1PTrainingCheckCostumeUsed(s32 fkind, s32 player, s32 costume);
extern void mnPlayers1PTrainingUpdateCursorPlacementPriorities(s32 player, s32 puck);
extern void mnPlayers1PTrainingUpdateCursor(GObj *gobj, s32 player, s32 cursor_status);
extern void mnPlayers1PTrainingAnnounceFighter(s32 player, s32 slot);
extern void mnPlayers1PTrainingMakePortraitFlash(s32 player);
extern void mnPlayers1PTrainingUpdateNameAndEmblem(s32 player);
extern void mnPlayers1PTrainingMakeHandicapLevel(s32 player);
extern void mnPlayers1PTrainingSetSceneData(void);

#define mnPlayers1PTrainingStartScene ndsBaseMNPlayers1PTrainingStartScene
void ndsBaseMNPlayers1PTrainingStartScene(void);

#include "../../decomp/BattleShip-main/decomp/src/mn/mnplayers/mnplayers1ptraining.c"

#undef mnPlayers1PTrainingStartScene

void mnPlayers1PTrainingStartScene(void)
{
    ndsBaseMNPlayers1PTrainingStartScene();
}

#endif /* NDS_P2_1P_GAME */
