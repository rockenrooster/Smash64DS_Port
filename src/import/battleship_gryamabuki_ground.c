/* Bounded Saffron City (Yamabuki) ground setup import, P2-4 stage 6.
 *
 * Mirrors the five stage wrappers beside it: this translation unit imports
 * decomp gr/grcommon/gryamabuki.c verbatim, so the gate's open/close cycle and
 * its Pokemon spawner are mechanically equivalent to the source, and exposes
 * ndsGRYamabukiSetupInitAll for the shared grCommonSetupInitAll gate. Only
 * built when NDS_P2_STAGE_YAMABUKI=1.
 *
 * TWO SEAMS, NEITHER OF THEM NEW. The gate is a moving yakumono --
 * mpCollisionSetYakumonoPosID (gryamabuki.c:222) with a hard-coded slot 3 --
 * and the Pokemon are ITEMS, spawned through itManagerMakeItemSetupCommon with
 * ITEM_FLAG_PARENT_GROUND (gryamabuki.c:101). Both already exist here.
 *
 * WHAT IS ABSENT AND WHY IT IS SAFE. The ground-monster item kinds have no
 * maker in the port yet, so the spawner's call returns NULL. The source guards
 * on exactly that: grYamabukiGateUpdateOpen (gryamabuki.c:177-179) treats a
 * NULL monster as the cue to close the gate again. So the gate opens, finds
 * nothing to follow, and closes -- which is the source's own behaviour for an
 * empty spawn, not a port-specific fallback. The Pokemon appear when their
 * item kinds land in P2-5.
 */
#if NDS_P2_STAGE_YAMABUKI

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

/* Forward declarations mirroring the other stage wrappers. */
void gcAddAnimJointAll(GObj *gobj, AObjEvent32 **anim_joints, f32 anim_frame);
void gcPlayAnimAll(GObj *gobj);
s32 syUtilsRandIntRange(s32 range);

/* Offsets from decomp reloc_data.us.h:3999-4011, the authoritative symbol
 * table. Note that ItemHead and MapHeader are both 0x14 there -- the item
 * attribute block starts at the map header, which is why the stage's Pokemon
 * attributes are addressed from the same base. */
#define NDS_RELOC_LVALUE(offset) (*(uintptr_t *)(uintptr_t)(offset))
#define llGRYamabukiMapItemHead NDS_RELOC_LVALUE(0x14u)
#define llGRYamabukiMapMapHead NDS_RELOC_LVALUE(0x8a0u)
#define llGRYamabukiMapGateOpenAnimJoint NDS_RELOC_LVALUE(0x9b0u)
#define llGRYamabukiMapGateCloseAnimJoint NDS_RELOC_LVALUE(0xa20u)

void ndsBaseGRCommonSetupInitAll(void);

#include "../../decomp/BattleShip-main/decomp/src/gr/grcommon/gryamabuki.c"

/* Gameplay transcription notes (all numeric behaviour is the included source,
 * cited per constant -- nothing below re-states a number):
 *
 * - Gate states dispatched by grYamabukiGateProcUpdate (gryamabuki.c:226):
 *   Sleep (:46), Wait (:145) which opens on a near fighter or on its own
 *   timer and cues nSYAudioFGMYamabukiGate, Open (:175) which tracks the
 *   monster within the source's own x clamp, and ClosedWait (:208).
 * - Near test: grYamabukiCheckNear (:56) requires a grounded fighter standing
 *   on the detect line.
 * - Spawn: grYamabukiMakeMonster (:74) reads the Monster map object for its
 *   position and refuses an immediate repeat of the last kind (:93-97).
 * - Bounds come from the source MPGroundData, not from any port constant.
 *
 * PRESENTATION ADAPTATIONS (gameplay untouched):
 * - The city, the gate and the background draw through the port's existing
 *   DObj renderer rather than a Yamabuki-specific native packet; the law 8
 *   packet for every stage is one pipeline job (P2-4n1).
 */
void ndsGRYamabukiSetupInitAll(void)
{
    ndsBaseGRCommonSetupInitAll();
}

#endif /* NDS_P2_STAGE_YAMABUKI */
