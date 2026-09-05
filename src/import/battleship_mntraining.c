/* P2-7 item 3. Training-mode character select, source import: textual
 * include of decomp/BattleShip-main/decomp/src/mn/mnplayers/
 * mnplayers1ptraining.c whole, following src/import/battleship_mnsoundtest.c
 * (P2-7 item 5 menu TU: scene entry imported as ndsBase* and re-exported
 * under its source name, so a later measured DS arena rebudget has a seam
 * and the diff stays reviewable). The adapter is a verbatim pass-through;
 * no behaviour invented here.
 *
 * Source pins (docs/p2/P2-7-modes-meta.md TRAINING rows):
 * - select writes training_man/com fkind+costume (:2911-2918, via
 *   mnPlayers1PTrainingSetSceneData, called on B-back to 1PMode (:2051),
 *   5-minute idle timeout to Title (:2947), and START-ready proceed to
 *   Maps (:2965)).
 * - MAN slot keeps scene training_man fkind/costume (:2997-3008), COM slot
 *   rolls a random unlocked fighter when training_com_fkind is Null
 *   (:3078-3098); both slots init :3058-3112; CSS idles back to Title after
 *   5 min without input (:2938-2949).
 * - FuncStart announces TrainingMode voice + BattleSelect BGM (:3204-3209);
 *   taskman setup :3216-3258 (ovl28 arena, GCCommonKindPlayerSelect proc).
 *
 * Gated on NDS_P2_1P_GAME: the Makefile defines no NDS_P2_TRAINING flag
 * (verified 2026-09-05: only NDS_P2_1P_GAME at Makefile:700), so this rides
 * the campaign flag with its scene sibling battleship_sc1ptrainingmode.c
 * until P2-7 mints its own gate.
 *
 * Shell status: same as the item-5 imports -- the native shell has no
 * training CSS module and cannot reach nSCKindPlayers1PTraining today
 * (title_backend.c:431 keeps the stub until gated); wiring is P2-7 item 9
 * (Menu completion), not this slice. Stops at the import by design.
 *
 * Shims vs unresolved, see handoff report:
 * - MNPlayersSlotTraining (decomp mn/mntypes.h:82-...): shimmed below,
 *   verbatim, guarded by NDS_MNPLAYERSISLOT_TRAINING_DEFINED. Port
 *   include/mn/mntypes.h carries only MNPlayersSlotVS; the training slot
 *   differs (no shade member, u16 unk_0xAE pad), so the VS struct cannot
 *   stand in -- layout would shift every field after costume.
 * - nSYAudioVoiceAnnounceTrainingMode (decomp gm/gmsound.h:627, ordinal
 *   530 by count under REGION_US; cross-checked: decomp AnnounceGo = 490
 *   = port value): shimmed below as a value macro. Port
 *   include/gm/gmsound.h does not carry it; every other audio ID this TU
 *   touches (BattleSelect BGM, MenuDenied, PublicCheer) is carried.
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
 *   include/sc/scene.h:561-569), lbReloc*/lbCommon*/gc*/syVideo*/syTaskman*/
 *   sys-audio/func_800266A0/func_800269C0 (same providers as the landed
 *   mnplayersvs import), dLBCommonFuncMatrixList (extern-declared below),
 *   efManagerInitEffects (extern-declared below, same as
 *   battleship_mnplayersvs.c:39).
 * - Collisions needing reported gating (not renamed away, behaviour must
 *   win): mnPlayers1PTrainingStartScene (adapter below) vs
 *   src/port/title_backend.c:431 NDS_SCENE_STUB.
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
#ifndef NDS_MNPLAYERSISLOT_TRAINING_DEFINED
#define NDS_MNPLAYERSISLOT_TRAINING_DEFINED 1
typedef struct MNPlayersSlotTraining
{
    GObj *cursor;
    GObj *puck;
    GObj *player;
    GObj *type_button;
    GObj *name_emblem_gobj;
    GObj *panel_doors;
    GObj *panel;
    GObj *team_color_button;
    GObj *handicap_cpu_level;
    GObj *arrows;
    GObj *handicap_cpu_level_value;
    GObj *flash;
    GObj *type;
    void *figatree_heap;
    u32 cpu_level;
    u32 handicap;
    s32 team;
    u32 unk_0x44;
    s32 fkind;
    u32 costume;
    s32 cursor_status;
    sb32 is_selected;
    sb32 is_recalling;
    s32 recall_end_tic;
    f32 recall_start_x;
    f32 recall_end_x;
    f32 recall_start_y;
    f32 recall_mid_y;
    f32 recall_end_y;
    s32 recall_tics;
    s32 holder_player;
    s32 held_player;
    s32 pkind;
    sb32 is_fighter_selected;
    sb32 is_status_selected;
    f32 puck_vel_x;
    f32 puck_vel_y;
    f32 cursor_pickup_x;
    f32 cursor_pickup_y;
    sb32 is_cursor_adjusting;
    s32 door_offset;
    alSoundEffect *p_sfx;
    u16 sfx_id;
    u16 unk_0xAE;
    sb32 is_hold_b;
    u32 unk_0xB4;
    s32 hold_b_tics;
} MNPlayersSlotTraining;
#endif

/* decomp gm/gmsound.h:627, ordinal 530 under REGION_US (see file header).
 * Macro, not a gameplay stub: only selects which announcer ID is requested. */
#ifndef nSYAudioVoiceAnnounceTrainingMode
#define nSYAudioVoiceAnnounceTrainingMode 530
#endif

extern sb32 (*dLBCommonFuncMatrixList[])(void);
extern void efManagerInitEffects(void);
extern s32 syUtilsRandTimeUCharRange(s32 range);

#define mnPlayers1PTrainingStartScene ndsBaseMNPlayers1PTrainingStartScene
void ndsBaseMNPlayers1PTrainingStartScene(void);

#include "../../decomp/BattleShip-main/decomp/src/mn/mnplayers/mnplayers1ptraining.c"

#undef mnPlayers1PTrainingStartScene

void mnPlayers1PTrainingStartScene(void)
{
    ndsBaseMNPlayers1PTrainingStartScene();
}

#endif /* NDS_P2_1P_GAME */
