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
 * Gated on NDS_P2_1P_GAME: the Makefile has no NDS_P2_MODES_META flag
 * (verified 2026-09-05), so this rides the campaign flag like the P2-6 step 5
 * bonus-stage TU until P2-7 mints its own gate.
 *
 * Shell status: same as the Options import -- no native SoundTest module
 * exists and the shell cannot reach this kind today (DATA menu itself is
 * P2-7 item 4/9); wiring is P2-7 item 9 (Menu completion), not this slice.
 * Stops at the import by design.
 *
 * Shims vs unresolved, see handoff report:
 * - Menu enum nMNSoundTestOption* (decomp mn/mndef.h:98-108): NOT shimmed
 *   here. Enum members cannot be #ifndef-guarded; owning home is port
 *   include/mn/mndef.h (reported follow-up, blocks compile).
 * - ID-table audio ordinals: the port include/gm/gmsound.h carries only a
 *   subset (music table needs ~25 more: Opening/Data/TrainingMode/1PIntro/
 *   BossStage/BossEntry/Last/1PBonusStage/1PStageClear/1PGameClear/
 *   1PBonusStageClear/1PBonusStageFailure/Zako/Metal/1PChallenger/Message/
 *   Ending/1PGameEndChoice/1PGameOver/Staffroll among :40-87; the Sound
 *   :90-286 and Voice :288-585 tables are almost entirely uncarried). NOT
 *   shimmed -- ordinals belong in include/gm/gmsound.h via
 *   check-audio-ordinals (reported follow-up, blocks compile).
 * - syAudioSetBGMVolumeFade (decomp sys/audio.h:212, audio.c:1315): owned by
 *   the DS mixer since 2026-09-05 (src/nds/nds_audio_bgm.c, an integer ramp
 *   stepped from the per-frame BGM update); declared in include/sys/audio.h.
 * - <lb/library.h> (:4, decomp-only header): port mn/menu.h does not provide
 *   it; compile reveals whether the port include tree carries it.
 * - ~15 ll* rows (llIFCommon*/llMNDataCommon*/llMNCommon*/llMNSoundTest*
 *   file IDs + header/capsule/arrow/digit/button sprites; battle-proven
 *   IFCommon rows may already resolve): left unresolved, need reloc manifest
 *   staging (offsets invented here would be fabricated data).
 * - Resolved port-side, no action: syAudioPlayBGM/syAudioStopBGMAll/
 *   syAudioSetBGMVolume + func_800266A0_272A0/func_800269C0_275C0 (port
 *   include/sys/audio.h) and func_80017EC0 (opening_movie_backend.c:4479).
 * - Collisions needing reported gating (not renamed away, behaviour must win):
 *   mnSoundTestStartScene (adapter below) vs
 *   src/port/title_backend.c:429 NDS_SCENE_STUB.
 */

#if NDS_P2_1P_GAME

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

#include "../../decomp/BattleShip-main/decomp/src/mn/mndata/mnsoundtest.c"

#undef mnSoundTestStartScene

void mnSoundTestStartScene(void)
{
    ndsBaseMNSoundTestStartScene();
}

#endif /* NDS_P2_1P_GAME */
