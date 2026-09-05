/* P2-6 step 6, Race last. Race to the Finish ground import (nGRKindBonus3).
 *
 * Source import: textual include of
 * decomp/BattleShip-main/decomp/src/gr/grbonus/grbonus3.c whole (106 lines),
 * following battleship_grcastle_ground.c. The include owns every symbol under
 * its source name (the unified-owner rule in the file doc of
 * src/import/battleship_sc1pgame_runtime.c): grBonus3InitHeaders,
 * grBonus3MakeBumpers, grBonus3TaruBombProcUpdate, grBonus3TaruBombMakeActor,
 * grBonus3FinishProcUpdate, grBonus3FinishMakeActor, grBonus3MakeGround. No
 * rename, no wrapper, no second owner. Only built when NDS_P2_1P_GAME=1; the
 * flag-off tree never sees it, and the flag-off stub it replaces is gated out
 * in src/import/battleship_grpupupu_ground.c (the grBonus3MakeGround stub
 * there is the only port stand-in for these names; the prototype at the top
 * of that file stays, matching the P2-4 per-stage pattern where prototypes
 * stay ungated and only definitions are flag-gated).
 *
 * Dispatch already exists: grMainSetupMakeGround (decomp gr/grmainsetup.c:
 * 31-40, imported whole by battleship_grpupupu_ground.c) calls
 * grBonus3MakeGround() when gkind == nGRKindBonus3 (:37-40), outside the VS
 * table (:33-36). The strong definition here links through that arm once the
 * stub is gated; this TU must NOT redefine grCommonSetupInitAll or
 * grMainSetupMakeGround (both live in the Pupupu wrapper).
 *
 * What the TU is, line by line (106 lines, no scrolling camera in it -- the
 * brief's "scrolling camera and course logic" premise is corrected below):
 * - grBonus3InitHeaders (:8-12): resolves map_head from
 *   gMPCollisionGroundData->map_nodes minus llGRBonus3MapMapHead, and
 *   item_head from gMPCollisionGroundData minus llGRBonus3MapItemHead.
 * - grBonus3MakeBumpers (:15-40): spawns nITKindGBumper items at each bumper
 *   DObjDesc past the sentinel (DOBJ_ARRAY_MAX), with per-bumper anim joints;
 *   offsets llGRBonus3MapBumpersDObjDesc / llGRBonus3MapBumpersAnimJoint off
 *   map_head.
 * - TaruBomb spawner (:43-77): a ground process every tick; when
 *   tarubomb_make_wait hits 0 it spawns nITKindTaruBomb at tarubomb_make_pos
 *   and reloads 180, then decrements per tick (:47-55 -- so one bomb every
 *   180 ticks). MakeActor (:60-77) registers the process, asserts exactly one
 *   nMPMapObjKind1PGameBonus3TaruBomb map object (infinite-loop
 *   syDebugPrintf/scManagerRunPrintGObjStatus guard, source behaviour like
 *   the Hyrule/Inishie count guards), and seeds tarubomb_make_pos/wait=180.
 * - Finish detection (:80-95): FinishProcUpdate reads the 1P player's fighter
 *   and, while grounded (ga == nMPKineticsGround) on the detect material
 *   ((coll_data.floor_flags & MAP_VERTEX_MAT_MASK) == nMPMaterialDetect),
 *   plays the Complete announce + BonusComplete FGM. There are no finish
 *   trigger POSITIONS in this TU -- the finish is a floor-material region
 *   baked into the course collision, not coordinates.
 * - grBonus3MakeGround (:98-106): InitHeaders + MakeBumpers +
 *   TaruBombMakeActor + FinishMakeActor, returns NULL (source returns NULL;
 *   geometry comes from the common display layers, not the maker).
 *
 * Mechanical equivalence cites (honest corrections first):
 * - Scroll speed: NO scroll speed exists in grbonus3.c. The TU spawns actors
 *   and tests floor material; it never touches the camera or any scroll
 *   velocity. The per-stage camera IS source-owned but lives in sc1pgame.c,
 *   not here: sc1PGameWaitStageBonus3Update (decomp sc/sc1pmode/sc1pgame.c:
 *   1522-1540) follows the player with gmCameraSetStatusPlayerFollow(...,
 *   0.0F, -15deg, 7000.0F, 0.3F, 31.5F) (:1524-1532), holds 60 ticks, then
 *   announces Go (:1533-1539). That follow camera (already imported whole by
 *   battleship_sc1pgame_runtime.c, behind the same flag) is the closest thing
 *   to "scrolling camera" in source; there is no autoscroll constant to cite.
 * - Course bounds: NOT in this TU either. Bounds/camera bounds/blast lines
 *   are MPGroundData rows in the static course geometry, which is the P2-4
 *   descriptor another brief is writing
 *   (scripts/stages/native_stage_descriptors/bonus3.py -- absent from the
 *   tree at land time, verified). Nothing here re-states a bound.
 * - Timer: decomp sc/sc1pmode/sc1pgame.c:995-997 -- case nSC1PGameStageBonus3:
 *   gSCManagerBattleState->time_limit = 1. (Bonus3 skips entry: :1045-1047
 *   is_skip_entry = TRUE; skips arrows and swaps the timer message:
 *   sc1PGameTryInitPlayerArrows :1829-1834, sc1PGameInitTimeUpMessage
 *   :1839-1844 Failure instead of TimeUp; manager loss check exempt:
 *   sc1pmanager.c:387-393; no-damage bonus: sc1pgame.c:2875-2880. All in the
 *   already-imported sc1pgame_runtime/sc1pmanager owners, not this TU.)
 * - Finish rule: decomp gr/grbonus/grbonus3.c:84-88 verbatim -- grounded AND
 *   floor material == nMPMaterialDetect => Complete announce
 *   (ifCommonAnnounceCompleteInitInterface(nSYAudioVoiceAnnounceComplete))
 *   + BonusComplete FGM (ifCommonBattleEndAddSoundQueueID(
 *   nSYAudioFGMBonusComplete)).
 * - TaruBomb cadence: grbonus3.c:53 wait = 180, :55 wait-- per tick.
 * - TaruBomb count guard: grbonus3.c:65-72 exactly-one map object of kind
 *   nMPMapObjKind1PGameBonus3TaruBomb (mpdef.h:131, value 0x29).
 * - Ladder row: sc1pgame.c:462-475 -- nGRKindBonus3, 3 enemies, trait
 *   nFTComputerTraitBonus3; spawn shuffle :1229-1258.
 *
 * GRBonus3Map (file id 0x127) is staged. Its MapHead, ItemHead,
 * BumpersDObjDesc and BumpersAnimJoint symbols are raw pointer-arithmetic
 * offsets in grbonus3.c:10-26, so their source-pinned address forms are
 * defined below (decomp include/reloc_data.us.h:4128-4134).
 *
 * Decomp symbols the TU calls that the port does not define (nothing
 * silently stubbed -- behaviour must win, so blockers are reported):
 * - GRStruct.bonus3 (GRBonusGroundVarsBonus3, decomp gr/grvars.h:263-270)
 *   and nMPMapObjKind1PGameBonus3TaruBomb (mp/mpdef.h:131, 0x29): both in
 *   include/gr/ground.h since 2026-09-05, promoted verbatim with the rest of
 *   the 1P Game map-object kinds.
 * - llGRBonus3Map* (four above): left unresolved, need manifest staging like
 *   the stage-clear table's rows (see map-symbols note).
 * - Everything else the TU touches IS defined in port headers or linked
 *   elsewhere, so no shim needed: itManagerMakeItemSetupCommon +
 *   nITKindGBumper/nITKindTaruBomb + ITEM_FLAG_PARENT_GROUND (it/item.h:
 *   442,892,895,1009); gcAddGObjProcess/gcMakeGObjSPAfter (sys/objman.h:
 *   73,97); gcAddDObjAnimJoint/gcPlayAnimAll (decomp sys/objanim.h:16,52 --
 *   forward-declared below, the castle-TU pattern, since no port header
 *   publishes them); DObjGetStruct/DOBJ_ARRAY_MAX via the decomp sys headers
 *   the build already provides to every ground TU; mpCollisionGetMapObj* /
 *   GetMapObjPositionID (gr/ground.h:628-630); ftGetStruct + nMPKineticsGround
 *   + MAP_VERTEX_MAT_MASK + nMPMaterialDetect (ft/fighter.h:2094,2110,2127);
 *   ifCommonAnnounceCompleteInitInterface/ifCommonBattleEndAddSoundQueueID
 *   (if/interface.h:264,297) + nSYAudioVoiceAnnounceComplete/
 *   nSYAudioFGMBonusComplete (gm/gmsound.h:723,496); syDebugPrintf
 *   (sys/debug.h:5); scManagerRunPrintGObjStatus (sc/scene.h:947).
 */
#if NDS_P2_1P_GAME

#include <PR/gbi.h>
#include <PR/os.h>
#include <PR/ultratypes.h>
#include <ef/effect.h>
#include <ft/fighter.h>
#include <gm/gmsound.h>
#include <gr/ground.h>
#include <if/interface.h>
#include <it/item.h>
#include <mn/menu.h>
#include <reloc_data.h>
#include <sc/scene.h>
#include <sys/debug.h>
#include <sys/obj.h>
#include <sys/objhelper.h>
#include <sys/objman.h>
#include <sys/rdp.h>
#include <sys/taskman.h>

/* Forward declarations mirroring battleship_grcastle_ground.c:59-60. The gc*
 * names live in decomp sys/objanim.h; this TU does not otherwise pull that
 * header in, and grbonus3.c uses both. Signatures are the decomp ones
 * verbatim (objanim.h:16,52). */
void gcAddDObjAnimJoint(DObj *dobj, AObjEvent32 *anim_joint, f32 anim_frame);
void gcPlayAnimAll(GObj *gobj);

/* THE FOUR MAP SYMBOLS ARE ROWED NOW, AND ALL FOUR ARE ARITHMETIC.
 *
 * include/reloc_data.h carries all four, and their values match the
 * vendored upstream table decomp/BattleShip-main/include/reloc_data.us.h:
 * 4129-4134 exactly (MapHead 0x0, ItemHead 0x0, BumpersDObjDesc 0x0,
 * BumpersAnimJoint 0x110). A row in reloc_data.h is an `extern uintptr_t` whose VALUE is the
 * offset, so `&llX` is a main-RAM address, and every one of grbonus3.c's four
 * uses is raw pointer arithmetic on `&llX` rather than a symbol handed to
 * lbRelocGetFileData:
 *
 *   :10  map_head  = map_nodes          - (intptr_t)&llGRBonus3MapMapHead
 *   :11  item_head = gMPCollisionGroundData - (intptr_t)&llGRBonus3MapItemHead
 *   :25  dobjdesc  = map_head + (intptr_t)&llGRBonus3MapBumpersDObjDesc
 *   :26  anim_joint= map_head + (intptr_t)&llGRBonus3MapBumpersAnimJoint
 *
 * With RAM addresses those reduce to map_nodes plus the distance between two
 * globals in .data -- eight bytes, in the current row order -- so
 * grBonus3MakeBumpers would walk a misaligned DObjDesc stream whose `id`
 * never reaches DOBJ_ARRAY_MAX, and bonus3.item_head (the base
 * battleship_item_tarubomb.c:81 hands the barrel bomb) would be a wild
 * pointer. Redefined below to the port's fake-lvalue form, whose ADDRESS is
 * the offset: the same NDS_RELOC_LVALUE idiom as
 * battleship_grpupupu_ground.c:58, battleship_grcastle_ground.c:69,
 * battleship_grinishie_ground.c:65 and battleship_grjungle_ground.c:65, which
 * exist for exactly this arithmetic. */
#define NDS_RELOC_LVALUE(offset) (*(uintptr_t *)(uintptr_t)(offset))
#define llGRBonus3MapMapHead NDS_RELOC_LVALUE(0x0u)
#define llGRBonus3MapItemHead NDS_RELOC_LVALUE(0x0u)
#define llGRBonus3MapBumpersDObjDesc NDS_RELOC_LVALUE(0x0u)
#define llGRBonus3MapBumpersAnimJoint NDS_RELOC_LVALUE(0x110u)

#include "../../decomp/BattleShip-main/decomp/src/gr/grbonus/grbonus3.c"

#endif /* NDS_P2_1P_GAME */
