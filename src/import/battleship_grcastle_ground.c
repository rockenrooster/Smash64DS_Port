/* Bounded Peach's Castle (Castle) ground setup import, P2-4 stage 2.
 *
 * Mirrors src/import/battleship_gryoster_ground.c: this translation unit
 * imports decomp gr/grcommon/grcastle.c verbatim, so the bumper follower and
 * the sliding platform are mechanically equivalent to the source, and exposes
 * ndsGRCastleSetupInitAll for the shared grCommonSetupInitAll gate (which
 * stays in the Pupupu wrapper; this TU must NOT redefine grCommonSetupInitAll
 * or grMainSetupMakeGround). Only built when NDS_P2_STAGE_CASTLE=1; the
 * flag-off tree never sees it.
 *
 * grcastle.c is the smallest stage translation unit in the game -- 66 lines,
 * three functions -- because Castle's two moving parts are not code:
 *
 *   - The sliding platform is animation data. grCastleInitAll:45 calls
 *     gcAddAnimJointAll on gMPCollisionGroundData->map_nodes, which is
 *     dStageCastleFile3_AnimJointRoot (259_GRCastleMap.c:59), and the sweep is
 *     a looping TraX SetValBlock ramp in 156_StageCastleFile3.c:28-40:
 *     0 -> -1050 over 599 frames -> 1050 over 1200 -> 0 over 600, then 2400
 *     frames of hold. There is no per-frame platform code to port.
 *   - The bumper is an ITEM. grCastleInitAll:57 spawns nITKindGBumper through
 *     itManagerMakeItemSetupCommon with ITEM_FLAG_PARENT_GROUND, and the only
 *     stage-side code is grCastleBumperProcUpdate (grcastle.c:12-22), which
 *     copies the ground's x into the bumper's x each tick.
 *
 * That second point is the dependency this file is honest about: the maker
 * returns NULL until the port's item core can make a stage-kind item (board
 * row P2-5i1), and the source's own guard at grcastle.c:17 tests
 * bumper_gobj != NULL before touching it. So with no item core the stage is
 * fully playable and the bumper is simply absent, rather than crashing -- and
 * the bumper starts working the moment the item core lands, with no change
 * here.
 */
#if NDS_P2_STAGE_CASTLE

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
#include <nds/nds_renderer.h>
#include <nds/nds_startup.h>
#include <reloc_data.h>
#include <sc/scene.h>
#include <sys/obj.h>
#include <sys/objhelper.h>
#include <sys/objman.h>
#include <sys/rdp.h>
#include <sys/taskman.h>

/* Forward declarations mirroring battleship_gryoster_ground.c:37-43. The gc*
 * names live in decomp sys/objanim.h + sys/objdisplay.h; this TU does not
 * otherwise pull those headers in, and grcastle.c uses both. Signatures are
 * the decomp ones verbatim. */
void gcAddAnimJointAll(GObj *gobj, AObjEvent32 **anim_joints, f32 anim_frame);
void gcPlayAnimAll(GObj *gobj);

/* decomp it/itmanager.h. The item manager's kind-indexed maker plus the
 * common spawn setup, itmanager.c:464-477 and :717-720. Declared rather than
 * included for the same reason as the gc* names above. The definition is the
 * port's item core; see the header comment on what happens before it exists. */
GObj *itManagerMakeItemSetupCommon(GObj *parent_gobj, s32 kind, Vec3f *pos,
                                   Vec3f *vel, u32 flags);

#define NDS_RELOC_LVALUE(offset) (*(uintptr_t *)(uintptr_t)(offset))

/* dStageCastleFile3_AnimJointRoot sits at offset 0 of relocData file 156, and
 * file 259's reference to it resolves with target offset 0
 * (156_StageCastleFile3.c:1-11). So Castle's map-head base is 0 and
 * grcastle.c:34's `map_nodes - &llGRCastleMapMapHead` stores map_nodes
 * unchanged -- unlike Yoster (0x100), Inishie (0x5f0) or Pupupu (0x10f0),
 * whose joint roots are interior to much larger node files. */
#define llGRCastleMapMapHead NDS_RELOC_LVALUE(0x0u)

void ndsBaseGRCommonSetupInitAll(void);

#include "../../decomp/BattleShip-main/decomp/src/gr/grcommon/grcastle.c"

/* Gameplay transcription notes (all numeric behaviour is the included source,
 * cited per constant -- nothing below re-states a number):
 *
 * - Bumper follow: x only, ground x plus the stored bumper offset, every tick
 *   at process priority 4 (grcastle.c:12-22, registered :38).
 * - Bumper spawn position: the nMPMapObjKindBumper map object, read through
 *   mpCollisionGetMapObjIDsKind / GetMapObjPositionID (grcastle.c:48-51), with
 *   zero initial velocity (:53-55).
 * - Platform sweep: joint animation only (grcastle.c:42-46); see the header.
 * - Bounds come from the source MPGroundData, not from any port constant:
 *   camera 4800/-1300/4000/-4000, blast 9500/-4000/9000/-9000, alt_warning
 *   -1900, with separate team rows (259_GRCastleMap.c:50-69).
 *
 * PRESENTATION ADAPTATIONS (gameplay untouched):
 * - Castle's mesh, its layer material animation and its background draw
 *   through the port's existing DObj renderer rather than a Castle-specific
 *   native packet; generate_nds_native_stage.py is Pupupu-only today, and the
 *   law 8 native packet for this stage is its own board item.
 */
void ndsGRCastleSetupInitAll(void)
{
    /* Same contract as the Pupupu and Yoster arms of grCommonSetupInitAll: the
     * caller has already checked gkind == nGRKindCastle, the collision ground
     * data, and the stage-ready flag. Run the original common setup, whose
     * dispatch reaches this TU's grCastleMakeGround via
     * dGRMainSetupProcMakeList. */
    ndsBaseGRCommonSetupInitAll();
}

#endif /* NDS_P2_STAGE_CASTLE */
