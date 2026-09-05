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
 * Map symbols the ground TU resolves at runtime (all four via the map-head
 * base; none is in port include/reloc_data.h -- verified by grep at land
 * time, so all four are unstaged and all four ride the one reloc file):
 * - llGRBonus3MapMapHead (grbonus3.c:10,25,26) -- unstaged.
 * - llGRBonus3MapItemHead (grbonus3.c:11) -- unstaged.
 * - llGRBonus3MapBumpersDObjDesc (grbonus3.c:25) -- unstaged.
 * - llGRBonus3MapBumpersAnimJoint (grbonus3.c:26) -- unstaged.
 * Exact reloc file name for all four: GRBonus3Map
 * (decomp reloc_stages/GRBonus3Map; llGRBonus3MapFileID 0x127 in
 * decomp include/reloc_data.us.h:324; collision table mpcollision.c:43;
 * offsets in decomp reloc_data.us.h:4128-4134). The static geometry itself
 * (bonus3.py) is the other brief's; this TU deliberately defines NO ll*
 * fallback -- offsets invented here would be fabricated data (the
 * battleship_sc1pbonusstage.c precedent: ~60 ll* rows left unresolved).
 *
 * Decomp symbols the TU calls that the port does not define (nothing
 * silently stubbed -- behaviour must win, so blockers are reported):
 * - GRStruct.bonus3 member (decomp gr/grtypes.h:62-64, vars gr/grvars.h:
 *   263-270 GRBonusGroundVarsBonus3 { map_head, item_head, tarubomb_make_pos,
 *   tarubomb_make_wait }): NOT shimmed here -- struct layout can only come
 *   from include/gr/ground.h, whose GRStruct (:506-517) stops at inishie
 *   (the battleship_sc1pbonusstage.c file doc already reports this class of
 *   blocker for bonus1/bonus2). First compile fails at
 *   gGRCommonStruct.bonus3 until the header promotes the member; that edit
 *   is out of this brief's scope.
 * - nMPMapObjKind1PGameBonus3TaruBomb (decomp mp/mpdef.h:131): NOT shimmed --
 *   port include/gr/ground.h MPMapObjKind stops at Rebirth 0x20. Enum values
 *   can only come from the header (same reason as above).
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

#include "../../decomp/BattleShip-main/decomp/src/gr/grbonus/grbonus3.c"

#endif /* NDS_P2_1P_GAME */
