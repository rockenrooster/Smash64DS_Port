/* P2-6 step 5. Bonus 1 (Break the Targets) + Bonus 2 (Board the Platforms).
 *
 * Source import: textual include of
 * decomp/BattleShip-main/decomp/src/sc/sc1pmode/sc1pbonusstage.c whole,
 * following battleship_scvsbattle.c (scene TU with its taskman/video setup
 * and start/update/draw functions), NOT a data-only transcription (the
 * pattern for slices of TUs whose function bodies cannot link yet; here
 * the data and the logic are inseparable -- MakeTargets indexes
 * dSC1PBonusStageTargetDescs by gkind, the platform procs index the
 * platform/bumper descs, and UpdateTargetCount drives the task HUD).
 *
 * Gated on NDS_P2_1P_GAME like the ladder tables. Provides the two symbols
 * src/import/battleship_item_target.c needs:
 * decomp sc1pbonusstage.c:318 gSC1PBonusStageItemFile (singular -- the task
 * brief's plural gSC1PBonusStageItemFiles exists nowhere in decomp or src;
 * ittarget.c:8 and battleship_item_target.c:31,50 both use the singular)
 * and decomp sc1pbonusstage.c:483 sc1PBonusStageUpdateTargetCount
 * (called by battleship_item_target.c:80).
 *
 * Rename follows battleship_ftcommon_shieldbreakfly.c / battleship_scmanager.c:
 * the scene entry is imported as ndsBase* and re-exported under its source
 * name, so a later measured DS arena rebudget (ovl6_BSS_END, DL buffers,
 * graphics/RDP sizes -- the scvsbattle precedent) has a seam and the diff
 * stays reviewable. The adapter is a verbatim pass-through; no behaviour
 * invented here.
 *
 * Shims vs unresolved, see handoff report:
 * - SCBATTLE_BONUSGAME_TASK_MAX + SC1PGAME_BONUS_MASK0_PERFECT: in
 *   include/sc/scene.h since 2026-09-05 (decomp sc/scdef.h:5,:33).
 * - GRBonusTarget and the GRStruct bonus1/bonus2/bonus3 union members: in
 *   include/gr/ground.h since 2026-09-05 (decomp gr/grvars.h:236-270,
 *   grtypes.h:62-64); the local GRBonusTarget shim was removed with them.
 * - ~60 ll* asset rows (llITBonus1ObjectHeaderFileID, 36 Bonus1 target
 *   triples, 10 Bonus2 bumper pairs, 18 Bonus2 platform rows, Bonus2Common /
 *   SC1PStageClear3 / IFCommon timer sprites): left unresolved, need reloc
 *   manifest staging like the stage-clear table's 72 rows (offsets invented
 *   here would be fabricated data).
 * - sc1PManagerCheckUnlockSoundTest (called :1235,:1247): left unresolved.
 *   It gates the SoundTest unlock message -- stubbing TRUE/FALSE would invent
 *   unlock behaviour.
 * - Collisions needing reported gating (not renamed away, behaviour must win):
 *   sc1PBonusStageInitBonus2 (:733) + sc1PBonusStageMakeBonus1Ground (:507)
 *   vs src/import/battleship_grpupupu_ground.c:711,716 stubs;
 *   sc1PBonusStageStartScene (adapter below) vs
 *   src/port/title_backend.c:439 NDS_SCENE_STUB.
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

/* AN `ll*` USED AS ARITHMETIC NEEDS ITS ADDRESS TO BE THE OFFSET.
 *
 * On N64 every `ll*` is an absolute linker symbol, so `&llX` IS the file
 * offset and the source freely mixes the two uses. The port cannot do that:
 * include/reloc_data.h declares each row as a real `extern uintptr_t` whose
 * VALUE is the offset (defined by src/port/diagnostics_mp_taskman_state.c),
 * so `&llX` is a main-RAM address near 0x020E0000. Two consumption shapes
 * appear in this source and only one of them survives that:
 *
 *  - as the `symbol` argument of lbRelocGetFileData: SAFE. The port's
 *    resolver (reloc_backend_assets.c:9671-9684 ndsRelocResolveSymbolOffset)
 *    sees a 0x02000000-range pointer and dereferences it, recovering the
 *    rowed offset. dSC1PBonusStageTargetDescs columns 1/2, every
 *    dSC1PBonusStagePlatformDescs / BoardedPlatformDescs column and the
 *    file-id arguments all travel this way and are left alone.
 *  - as a raw term in pointer arithmetic: BROKEN. sc1pbonusstage.c:443-444
 *    computes the target file's base as
 *    `gr_desc[1].dobjdesc - target->start`, and :709 the bumper file's base
 *    as `map_nodes - dSC1PBonusStageBumperDescs[...][0]`. With a RAM address
 *    as the subtrahend both bases are garbage, lbRelocGetFileData finds no
 *    loaded file for them and hands the garbage straight back
 *    (reloc_backend_assets.c:13311-13314), and the very next `dobjdesc->id`
 *    faults. Break the Targets could not build one target and Board the
 *    Platforms could not build one bumper.
 *
 * So exactly the symbols in the second shape are redefined below to the
 * port's established fake-lvalue form -- the same NDS_RELOC_LVALUE that
 * battleship_grpupupu_ground.c:58, battleship_grcastle_ground.c:69,
 * battleship_grinishie_ground.c:65 and battleship_grjungle_ground.c:65 use
 * for their own ground arithmetic -- whose ADDRESS is the offset, keeping
 * both shapes correct. Values are copied verbatim from the rows in
 * include/reloc_data.h:1928-1971, which match the vendored upstream table
 * decomp/BattleShip-main/include/reloc_data.us.h:4059-4126 symbol for
 * symbol; nothing here is invented. */
#define NDS_RELOC_LVALUE(offset) (*(uintptr_t *)(uintptr_t)(offset))

/* Bonus 1: column 0 of dSC1PBonusStageTargetDescs (sc1pbonusstage.c:443). */
#define llGRBonus1MarioMapTargetsStart NDS_RELOC_LVALUE(0x1eb0u)
#define llGRBonus1FoxMapTargetsStart NDS_RELOC_LVALUE(0x2068u)
#define llGRBonus1DonkeyMapTargetsStart NDS_RELOC_LVALUE(0x1f20u)
#define llGRBonus1SamusMapTargetsStart NDS_RELOC_LVALUE(0x1868u)
#define llGRBonus1LuigiMapTargetsStart NDS_RELOC_LVALUE(0x1ba0u)
#define llGRBonus1LinkMapTargetsStart NDS_RELOC_LVALUE(0x2378u)
#define llGRBonus1YoshiMapTargetsStart NDS_RELOC_LVALUE(0x2d68u)
#define llGRBonus1CaptainMapTargetsStart NDS_RELOC_LVALUE(0x1888u)
#define llGRBonus1KirbyMapTargetsStart NDS_RELOC_LVALUE(0x2150u)
#define llGRBonus1PikachuMapTargetsStart NDS_RELOC_LVALUE(0x2658u)
#define llGRBonus1PurinMapTargetsStart NDS_RELOC_LVALUE(0x1ff8u)
#define llGRBonus1NessMapTargetsStart NDS_RELOC_LVALUE(0x2940u)

/* Bonus 2: column 0 of dSC1PBonusStageBumperDescs (sc1pbonusstage.c:709).
 * Only the five boards whose row is a symbol; the other seven are literal
 * 0x0 in the source table. Column 1 (AnimJoint) is a symbol argument only. */
#define llGRBonus2FoxMapBumpersDObjDesc NDS_RELOC_LVALUE(0xe160u)
#define llGRBonus2SamusMapBumpersDObjDesc NDS_RELOC_LVALUE(0x2910u)
#define llGRBonus2KirbyMapBumpersDObjDesc NDS_RELOC_LVALUE(0x3920u)
#define llGRBonus2PurinMapBumpersDObjDesc NDS_RELOC_LVALUE(0x4fe0u)
#define llGRBonus2NessMapBumpersDObjDesc NDS_RELOC_LVALUE(0x3fe0u)

#define sc1PBonusStageStartScene ndsBaseSC1PBonusStageStartScene
void ndsBaseSC1PBonusStageStartScene(void);

#include "../../decomp/BattleShip-main/decomp/src/sc/sc1pmode/sc1pbonusstage.c"

#undef sc1PBonusStageStartScene

void sc1PBonusStageStartScene(void)
{
    ndsBaseSC1PBonusStageStartScene();
}

#endif /* NDS_P2_1P_GAME */
