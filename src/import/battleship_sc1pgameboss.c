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

/* ALL 21 WALLPAPER ROWS ARE ARITHMETIC, NOT SYMBOL TOKENS.
 *
 * The note this replaces said the 21 llGRLastMap* rows "resolve through
 * <reloc_data.h>". They are indeed rowed there (:1686-1706) with the values
 * of the vendored upstream table (decomp/BattleShip-main/include/
 * reloc_data.us.h:4033-4055, identical symbol for symbol) -- but a row is an
 * `extern uintptr_t` whose VALUE is the offset, so `&llX` is a main-RAM
 * address near 0x020E0000. That form only works where the source hands the
 * symbol to lbRelocGetFileData, whose port resolver dereferences a
 * RAM-range token (reloc_backend_assets.c:9671-9684). This TU never does.
 * Every use here is raw arithmetic:
 *
 *   :1020 file_head = gr_desc[1].dobjdesc - (intptr_t)&llGRLastMapFileHead
 *   :31-243 the dSC1PGameBoss{Effect,Anim}Descs rows are stored as o_dobjdesc
 *           / o_mobjsub / o_anim_joint / o_matanim_joint and added straight
 *           to that base at :884-885 and :903 (`o_dobjdesc + addr`,
 *           `addr + o_mobjsub`, `addr + o_anim_joint`).
 *
 * Left as rows, file_head would land megabytes below the Final Destination
 * map and each descriptor would add another RAM address on top, so
 * sc1PGameBossMakeWallpaperEffect would build Master Hand's backdrop out of
 * unmapped memory on the first wallpaper change. All 21 are therefore
 * redefined to the port's fake-lvalue form, whose ADDRESS is the offset --
 * the same NDS_RELOC_LVALUE the ground TUs use for their own map arithmetic
 * (battleship_grpupupu_ground.c:58, battleship_grcastle_ground.c:69,
 * battleship_grinishie_ground.c:65, battleship_grjungle_ground.c:65).
 * llGRLastMapMapHeader is deliberately NOT redefined: its only use is as a
 * lbRelocGetFileData symbol in reloc_backend_compat_shims.c, which wants the
 * row form. */
#define NDS_RELOC_LVALUE(offset) (*(uintptr_t *)(uintptr_t)(offset))
#define llGRLastMapFileHead NDS_RELOC_LVALUE(0x4d48u)
#define llGRLastMapEffects0MObjSub NDS_RELOC_LVALUE(0x86d8u)
#define llGRLastMapEffects0DObjDesc NDS_RELOC_LVALUE(0x8960u)
#define llGRLastMapAnims0AnimJoint NDS_RELOC_LVALUE(0x8a40u)
#define llGRLastMapAnims0MatAnimJoint NDS_RELOC_LVALUE(0x8c50u)
#define llGRLastMapEffects1MObjSub NDS_RELOC_LVALUE(0x97b0u)
#define llGRLastMapEffects1DObjDesc NDS_RELOC_LVALUE(0xa188u)
#define llGRLastMapAnims1AnimJoint NDS_RELOC_LVALUE(0xa340u)
#define llGRLastMapAnims1MatAnimJoint NDS_RELOC_LVALUE(0xb1b0u)
#define llGRLastMapEffects2_0MObjSub NDS_RELOC_LVALUE(0xd470u)
#define llGRLastMapEffects2_0DObjDesc NDS_RELOC_LVALUE(0xdd90u)
#define llGRLastMapEffects2_1MObjSub NDS_RELOC_LVALUE(0x10788u)
#define llGRLastMapEffects2_1DObjDesc NDS_RELOC_LVALUE(0x11268u)
#define llGRLastMapAnims2_0AnimJoint NDS_RELOC_LVALUE(0xde70u)
#define llGRLastMapAnims2_0MatAnimJoint NDS_RELOC_LVALUE(0xdec0u)
#define llGRLastMapAnims2_1MatAnimJoint NDS_RELOC_LVALUE(0x11420u)
#define llGRLastMapEffects3_0MObjSub NDS_RELOC_LVALUE(0x10788u)
#define llGRLastMapEffects3_0DObjDesc NDS_RELOC_LVALUE(0x11268u)
#define llGRLastMapEffects3_1DObjDesc NDS_RELOC_LVALUE(0x12858u)
#define llGRLastMapAnims3_0MatAnimJoint NDS_RELOC_LVALUE(0x115c0u)
#define llGRLastMapAnims3_1AnimJoint NDS_RELOC_LVALUE(0x128e0u)

#include "../../decomp/BattleShip-main/decomp/src/sc/sc1pmode/sc1pgameboss.c"

#endif /* NDS_P2_1P_GAME */
