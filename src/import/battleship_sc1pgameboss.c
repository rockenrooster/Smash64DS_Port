/* P2-6 step 7 (Boss). Final Destination boss wallpaper/camera scene slice.
 *
 * Source import: textual include of
 * decomp/BattleShip-main/decomp/src/sc/sc1pmode/sc1pgameboss.c whole
 * (1024 lines: dSC1PGameBoss* tables, wallpaper/camera procs,
 * sc1PGameBossInitWallpaper :1005), following battleship_sc1pbonusstage.c
 * (scene-TU include pattern), NOT a data-only transcription.
 *
 * Unified-owner rule (stated in battleship_sc1pgame_runtime.c): the include
 * owns every symbol it defines under its source name; no renamed private
 * copies. No ndsBase* rename here: this TU has NO scene entry (no
 * StartScene), and sc1PGameBossInitWallpaper must keep its source name so it
 * beats the NDS_WEAK stub in src/port/battle_playable_compat_stubs.c:141 by
 * normal strong-over-weak link override (that file untouched). Same for the
 * sc1pgame.c camera pair already owned by battleship_sc1pgame_runtime.c
 * (sc1PGameSetCameraZoom :1892, sc1PGameBossSetCameraZoom :1910): this TU
 * defines neither; a second definition would be a link error.
 *
 * Shims vs unresolved, by reading (no compile per owner directive):
 * - SC1PGameBossPlan / Anim / Effect / Wallpaper / Main: shimmed below,
 *   verbatim from decomp sc/sctypes.h:56-103 (port include/sc/scene.h lacks
 *   them). Guarded so a later header promotion collides loudly.
 * - 21 llGRLastMap* rows (FileHead + Effects0/1/2_0/2_1/3_0/3_1 DObjDesc +
 *   MObjSub + Anims0/1/2_0 AnimJoint/MatAnimJoint + Anims2_1/3_0 MatAnimJoint
 *   + Anims3_1 AnimJoint): local externs. Real ids in
 *   tools/reloc_data_symbols.us.txt:290,4200-4221 (FileID 0x10a, FileHead
 *   0x4D48, ...); port stages none (no GRLast in reloc_backend_assets.c or
 *   manifests; Last venue rides P2-4). Left unresolved, never stubbed.
 * - nGCCommonKindBossWallpaper / nGCCommonLinkIDWallpaperEffect /
 *   camera-tag values: NOT shimmed (enum values are behaviour; invented
 *   values = fabricated data). Compile reveals first gap.
 * - Engine (gcMakeGObjSPAfter, gcAddGObjProcess, gcMakeCameraGObj, gGCCommonLinks,
 *   gMPCollisionGroundData, gSCManagerBattleState) comes from port seams.
 * - Collisions needing reported gating: sc1PGameBossInitWallpaper here vs
 *   compat_stubs.c:141 weak (resolves by override, no edit).
 */

#if NDS_P2_1P_GAME

#include <stdint.h>
#include <PR/gbi.h>
#include <PR/os.h>
#include <PR/ultratypes.h>
#include <ft/fighter.h>
#include <gm/generic.h>
#include <gm/gmsound.h>
#include <gr/ground.h>
#include <if/interface.h>
#include <it/item.h>
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

/* GRLast map rows: real ids tools/reloc_data_symbols.us.txt:290,4200-4221;
 * port stages none (P2-4 venue path). Declared so TU compiles; link stays
 * honestly open until orchestrator stages definitions. */
extern uintptr_t llGRLastMapFileHead;
extern uintptr_t llGRLastMapEffects0DObjDesc;
extern uintptr_t llGRLastMapEffects0MObjSub;
extern uintptr_t llGRLastMapAnims0AnimJoint;
extern uintptr_t llGRLastMapAnims0MatAnimJoint;
extern uintptr_t llGRLastMapEffects1DObjDesc;
extern uintptr_t llGRLastMapEffects1MObjSub;
extern uintptr_t llGRLastMapAnims1AnimJoint;
extern uintptr_t llGRLastMapAnims1MatAnimJoint;
extern uintptr_t llGRLastMapEffects2_0DObjDesc;
extern uintptr_t llGRLastMapEffects2_0MObjSub;
extern uintptr_t llGRLastMapEffects2_1DObjDesc;
extern uintptr_t llGRLastMapEffects2_1MObjSub;
extern uintptr_t llGRLastMapAnims2_0AnimJoint;
extern uintptr_t llGRLastMapAnims2_0MatAnimJoint;
extern uintptr_t llGRLastMapAnims2_1MatAnimJoint;
extern uintptr_t llGRLastMapEffects3_0DObjDesc;
extern uintptr_t llGRLastMapEffects3_0MObjSub;
extern uintptr_t llGRLastMapEffects3_1DObjDesc;
extern uintptr_t llGRLastMapAnims3_0MatAnimJoint;
extern uintptr_t llGRLastMapAnims3_1AnimJoint;

#include "../../decomp/BattleShip-main/decomp/src/sc/sc1pmode/sc1pgameboss.c"

#endif /* NDS_P2_1P_GAME */
