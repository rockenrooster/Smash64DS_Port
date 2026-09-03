/* Bounded Yoshi's Island (Yoster) ground setup import, P2-4 first stage.
 *
 * Mirrors src/import/battleship_grpupupu_ground.c: this translation unit
 * imports decomp gr/grcommon/gryoster.c verbatim so the three cloud platforms
 * are mechanically equivalent to the source, and exposes ndsGRYosterSetupInitAll
 * for the shared grCommonSetupInitAll gate (which stays in the Pupupu wrapper;
 * this TU must NOT redefine grCommonSetupInitAll or grMainSetupMakeGround).
 * Only built when NDS_P2_STAGE_YOSTER=1; the flag-off tree never sees it.
 */
#if NDS_P2_STAGE_YOSTER

#include <PR/gbi.h>
#include <PR/os.h>
#include <PR/ultratypes.h>
#include <ef/effect.h>
#include <ft/fighter.h>
#include <gm/gmsound.h>
#include <gr/ground.h>
#include <if/interface.h>
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

void *func_800269C0_275C0(u16 fgm_id);

/* Forward declarations mirroring battleship_grpupupu_ground.c:33-42. The gc*
 * names live in decomp sys/objanim.h + sys/objdisplay.h and the lbCommon*
 * names in decomp lb/lbcommon.h; this TU does not otherwise pull those
 * headers in, and gryoster.c (below) uses every one of them. Signatures are
 * the decomp ones verbatim. */
void gcDrawDObjTreeForGObj(GObj *gobj);
void gcSetupCustomDObjs(GObj *gobj, DObjDesc *dobjdesc, DObj **dobjs,
                        u8 tk1, u8 tk2, u8 tk3);
void gcAddAnimJointAll(GObj *gobj, AObjEvent32 **anim_joints,
                       f32 anim_frame);
void gcPlayAnimAll(GObj *gobj);
void gcPlayDObjAnimJoint(DObj *dobj);
void lbCommonAddMObjForTreeDObjs(DObj *root_dobj, MObjSub ***p_mobjsubs);
void lbCommonAddTreeDObjsAnimAll(DObj *root_dobj, AObjEvent32 **anim_joints,
                                 AObjEvent32 ***p_matanim_joints,
                                 f32 anim_frame);

/* Vapor bank markers, decomp gr/grcommon/gryoster.h:9-12. Address identity
 * only, as with the Pupupu bank markers; the particle look itself is
 * presentation (see below). */
intptr_t lGRYosterParticleScriptBankLo;
intptr_t lGRYosterParticleScriptBankHi;
intptr_t lGRYosterParticleTextureBankLo;
intptr_t lGRYosterParticleTextureBankHi;

#define NDS_RELOC_LVALUE(offset) (*(uintptr_t *)(uintptr_t)(offset))
#define llGRYosterMapMapHead NDS_RELOC_LVALUE(0x100u)
#define llGRYosterMap_1E0_AnimJoint NDS_RELOC_LVALUE(0x1e0u)
#define llGRYosterMap_4B8_MObjSub NDS_RELOC_LVALUE(0x4b8u)
#define llGRYosterMapCloudDisplayList NDS_RELOC_LVALUE(0x580u)
#define llGRYosterMapCloudSolidMatAnimJoint NDS_RELOC_LVALUE(0x670u)
#define llGRYosterMapCloudEvaporateMatAnimJoint NDS_RELOC_LVALUE(0x690u)

void ndsBaseGRCommonSetupInitAll(void);

#include "../../decomp/BattleShip-main/decomp/src/gr/grcommon/gryoster.c"

/* Gameplay transcription notes (all numeric behaviour is the included source,
 * cited per constant -- nothing below re-states a number):
 *
 * - Cloud line ids {1,2,3}: dGRYosterCloudLineIDs, gryoster.c:15.
 * - Stood-on test (grounded fighter whose floor line maps to this cloud's line
 *   id): grYosterCheckFighterCloudStand, gryoster.c:50-69.
 * - Solid: pressure 0..180 by 5.0/tick up while stood on / down otherwise
 *   (gryoster.c:108-124); stood timer latched -1 -> 120 on first stood tick
 *   and decremented while > 0 (gryoster.c:104-107,125-128); timer 0 expires
 *   the cloud into Evaporate with vapor at (-750,-350) + FGM vapor cue
 *   (gryoster.c:85-99); sink y = altitude - pressure every tick
 *   (gryoster.c:131-132); line forced on while Solid (gryoster.c:79-84).
 * - Evaporate: line off (gryoster.c:140-145), 180-tick wait (gryoster.c:89,
 *   146-153), then Solid with pressure 0 / timer -1 (gryoster.c:147-152).
 * - Dispatch Solid/Evaporate + anim apply: gryoster.c:178-196,157-175.
 * - Setup: grYosterInitAll / grYosterMakeGround, gryoster.c:199-268.
 *
 * PRESENTATION ADAPTATIONS (gameplay untouched):
 * - Cloud mesh / material animation / birds / Heiho draw through the port's
 *   existing DObj renderer rather than a Yoster-specific native packet; no
 *   native-stage packet exists for Yoster yet (generate_nds_native_stage.py is
 *   Pupupu-only -- orchestrator lane).
 * - Vapor look: at NDS_R2_PARTICLE_RUNTIME=0 the particle constructor is a
 *   stub returning NULL (same standing note as the Pupupu wrapper), so the
 *   evaporate puff is currently the FGM cue alone; banking the gryoster
 *   particle scb/txb through generate_nds_particle_banks.py is orchestrator
 *   lane and changes presentation only.
 */
void ndsGRYosterSetupInitAll(void)
{
    /* Same contract as the Pupupu arm of grCommonSetupInitAll: the caller has
     * already checked gkind == nGRKindYoster, the collision ground data, and
     * the stage-ready flag. Run the original common setup, whose dispatch
     * reaches this TU's grYosterMakeGround via dGRMainSetupProcMakeList. */
    ndsBaseGRCommonSetupInitAll();
}

#endif /* NDS_P2_STAGE_YOSTER */
