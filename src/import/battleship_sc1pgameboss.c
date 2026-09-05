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
 * Shims vs resolved, by reading (no compile per owner directive):
 * - SC1PGameBossPlan / Anim / Effect / Wallpaper / Main: promoted in port
 *   include/sc/scene.h:334-380, verbatim from decomp sc/sctypes.h:56-103;
 *   this TU carries no shims for them.
 * - nGCCommonKindBossWallpaper (1023) and nGCCommonLinkIDWallpaperEffect
 *   (13): resolved from decomp src/sys/objdef.h:29 and :95, which is on the
 *   include path (Makefile INCLUDES carries the decomp src dir, and there
 *   is no port sys/objdef.h). No invented values here.
 * - 21 llGRLastMap* rows (FileHead + Effects0/1/2_0/2_1/3_0/3_1 DObjDesc +
 *   MObjSub + Anims0/1/2_0 AnimJoint/MatAnimJoint + Anims2_1/3_0 MatAnimJoint
 *   + Anims3_1 AnimJoint): declared by include/reloc_data.h
 *   NDS_MENU_RELOC_SYMBOLS with the real ids from
 *   tools/reloc_data_symbols.us.txt:290,4200-4221 (FileID 0x10a, FileHead
 *   0x4D48, ...), staged behind NDS_P2_1P_GAME (map plus ExternDataBank114;
 *   file 96 StageLastBackground stays with the native packet, whose header
 *   field is unused on this target).
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

/* No local llGRLastMap* externs: the 21 wallpaper rows resolve through
 * <reloc_data.h> above. */

#include "../../decomp/BattleShip-main/decomp/src/sc/sc1pmode/sc1pgameboss.c"

#endif /* NDS_P2_1P_GAME */
