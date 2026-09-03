/* Bounded Congo Jungle (Jungle) ground setup import, P2-4 stage 3.
 *
 * Mirrors src/import/battleship_grcastle_ground.c and
 * battleship_gryoster_ground.c: this translation unit imports decomp
 * gr/grcommon/grjungle.c verbatim, so the barrel cannon's rotation sweep and
 * its capture geometry are mechanically equivalent to the source, and exposes
 * ndsGRJungleSetupInitAll for the shared grCommonSetupInitAll gate (which
 * stays in the Pupupu wrapper; this TU must NOT redefine grCommonSetupInitAll
 * or grMainSetupMakeGround). Only built when NDS_P2_STAGE_JUNGLE=1.
 *
 * WHAT MAKES THIS STAGE DIFFERENT FROM THE FIRST TWO. Dream Land's wind
 * pushes a fighter's velocity, Yoshi's Island's clouds are collision, and
 * Peach's Castle's bumper is an item. Congo Jungle's cannon CAPTURES the
 * fighter: grJungleTaruCannCheckGetDamageKind (grjungle.c:142) hands the
 * fighter to ftMainCheckAddGroundObstacle (ft/ftmain.c:1592), which puts it in
 * nFTCommonStatusTaruCann and hands control to ft/ftcommon/ftcommontarucann.c
 * until the cannon fires. So the stage half here is only half the work; the
 * fighter half lives beside the other common statuses in
 * src/port/reloc_backend_compat_shims.c, and the two are one slice because
 * this file's grJungleTaruCannAddAnimShoot is what the fighter's ProcUpdate
 * and ProcInterrupt call when the shot is armed.
 *
 * The same seam serves exactly one other stage, Hyrule Castle's tornado
 * (grhyrule.c:172); nothing else in the game uses it. Planet Zebes and
 * Mushroom Kingdom use ftMainCheckAddGroundHazard, which is a different entry
 * point.
 */
#if NDS_P2_STAGE_JUNGLE

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

/* Forward declarations mirroring battleship_gryoster_ground.c:37-43: the gc*
 * names live in decomp sys/objanim.h and sys/objdisplay.h and the sy* ones in
 * sys/utils.h, and this TU does not otherwise pull those headers in.
 * Signatures are the decomp ones verbatim. */
void gcAddAnimJointAll(GObj *gobj, AObjEvent32 **anim_joints, f32 anim_frame);
void gcPlayAnimAll(GObj *gobj);
s32 syUtilsRandIntRange(s32 range);

/* Offsets are the source's own, from decomp reloc_data.us.h:3932-3937 -- the
 * authoritative symbol table, not derived from the file layout by hand:
 *   MapHead 0xA98, ThrowHitDesc 0xBC, and the cannon's three joint roots,
 *   Default 0xB20, Fill 0xB68, Shoot 0xBF8.
 * The throw descriptor is read by the FIGHTER half rather than by this file
 * (ftcommontarucann.c:100), which is why it is defined here beside the others
 * rather than where it is used. */
#define NDS_RELOC_LVALUE(offset) (*(uintptr_t *)(uintptr_t)(offset))
#define llGRJungleMapMapHead NDS_RELOC_LVALUE(0xa98u)
#define llGRJungleMapTaruCannDefaultAnimJoint NDS_RELOC_LVALUE(0xb20u)
#define llGRJungleMapTaruCannFillAnimJoint NDS_RELOC_LVALUE(0xb68u)
#define llGRJungleMapTaruCannShootAnimJoint NDS_RELOC_LVALUE(0xbf8u)

void ndsBaseGRCommonSetupInitAll(void);

#include "../../decomp/BattleShip-main/decomp/src/gr/grcommon/grjungle.c"

/* Gameplay transcription notes (all numeric behaviour is the included source,
 * cited per constant -- nothing below re-states a number):
 *
 * - Cannon states, two of them: Move and Rotate
 *   (grjungle.c:24-27), dispatched by grJungleTaruCannProcUpdate
 *   (grjungle.c:92) at process priority 4 (registered grjungle.c:119-130).
 * - Move counts its wait down and then picks a new rotation step at random
 *   (grjungle.c:59-72); Rotate accumulates that step into the DObj's z
 *   rotation (grjungle.c:74-90).
 * - Capture test: a box centred on the cannon (grjungle.c:142-190), giving
 *   nGMHitEnvironmentTaruCann; the fighter goes into the common TaruCann
 *   status through ftMainCheckAddGroundObstacle.
 * - Pose exports for the fighter half and for the CPU:
 *   grJungleTaruCannGetPosition (grjungle.c:193) and GetRotate (:199).
 * - Bounds come from the source MPGroundData, not from any port constant:
 *   camera 4000/-2000/3700/-3700, blast 8000/-4700/8100/-8100, alt_warning
 *   -1900, with separate team rows (261_GRJungleMap.c:54-76).
 *
 * PRESENTATION ADAPTATIONS (gameplay untouched):
 * - The cannon mesh, the birds and the background draw through the port's
 *   existing DObj renderer rather than a Jungle-specific native packet. The
 *   law 8 packet for every stage is one pipeline job, sized in
 *   docs/p2/P2-4-stage-production.md, not eight per-stage bakes.
 */
void ndsGRJungleSetupInitAll(void)
{
    /* Same contract as the Pupupu, Yoster and Castle arms of
     * grCommonSetupInitAll: the caller has already checked
     * gkind == nGRKindJungle, the collision ground data, and the stage-ready
     * flag. Run the original common setup, whose dispatch reaches this TU's
     * grJungleMakeGround via dGRMainSetupProcMakeList. */
    ndsBaseGRCommonSetupInitAll();
}

#endif /* NDS_P2_STAGE_JUNGLE */
