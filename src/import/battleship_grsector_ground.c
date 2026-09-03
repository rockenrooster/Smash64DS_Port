/* Bounded Sector Z (Sector) ground setup import, P2-4 stage 8.
 *
 * Mirrors the seven stage wrappers beside it: this translation unit imports
 * decomp gr/grcommon/grsector.c verbatim and exposes ndsGRSectorSetupInitAll
 * for the shared grCommonSetupInitAll gate. Only built when
 * NDS_P2_STAGE_SECTOR=1.
 *
 * THIS IS THE MOST EXPENSIVE STAGE IN THE GAME. grsector.c is 1,131 lines --
 * 1.9 times Mushroom Kingdom and 4.5 times Congo Jungle -- with roughly
 * fifteen hazard update functions across two independent weapon pipelines and
 * a moving collider:
 *
 *   - The Arwing's patrol and pilot state machines, which choose a flight
 *     pattern and a pilot and drive the ship along a sector descriptor.
 *   - Two laser pipelines, 2D and 3D, each reaching a fighter as an ordinary
 *     weapon through wpManagerMakeWeapon with its own attack collision.
 *   - The Arwing body itself as a moving yakumono, gated on whether its line
 *     is active and whether it is near enough in z.
 *
 * ONE VALUE HERE IS REGION-DEPENDENT and it is the only one found anywhere in
 * the eight stages: the transform kind is 0x53 under REGION_US against 0x52
 * elsewhere (grsector.c:142-146). The port builds REGION_US, so the included
 * source selects the US arm on its own.
 *
 * WHAT IS ABSENT. The Arwing's lasers are weapons, and the weapon manager
 * makes them from attributes in this stage's own map. Nothing is stubbed here:
 * if a laser cannot be made the source's own NULL handling applies, the same
 * way every other stage's absent item degrades.
 */
#if NDS_P2_STAGE_SECTOR

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

/* decomp lb/lbreloc.c:85. The by-id residency query; the port defines it
 * beside its pointer-taking twin in reloc_backend_assets.c. */
void *lbRelocGetForceStatusBufferFile(u32 id);

/* decomp wp/weapon.h. The stage tests its own laser against the map the way
 * every weapon does; three fighter weapon files already declare it. */
sb32 wpMapTestAllCheckCollEnd(GObj *weapon_gobj);

/* The laser weapon descriptors at grsector.c:170-210 name procs the same
 * file defines further down (:561-789), so they need forward declarations
 * ahead of the include. Signatures are the definitions verbatim. */
sb32 grSectorArwingWeaponLaser2DProcMap(GObj *weapon_gobj);
sb32 grSectorArwingWeaponLaser2DProcHit(GObj *weapon_gobj);
sb32 grSectorArwingWeaponLaser2DProcHop(GObj *weapon_gobj);
sb32 grSectorArwingWeaponLaser2DProcReflector(GObj *weapon_gobj);
sb32 grSectorArwingWeaponLaser3DProcMap(GObj *weapon_gobj);
sb32 grSectorArwingWeaponLaser3DProcHit(GObj *weapon_gobj);
sb32 grSectorArwingWeaponLaser3DProcAbsorb(GObj *weapon_gobj);

/* decomp lb/lbcommon.c:3008 and the libultra matrix helper the port already
 * imports (battleship_libultra_gu_mtxutil.c). */
void lbCommonCross3D(Vec3f *a, Vec3f *b, Vec3f *out);
void guMtxF2L(f32 mf[4][4], Mtx *m);

/* Offsets from decomp reloc_data.us.h:3943-3960, the authoritative symbol
 * table. Note Arwing0's two offsets are both 0x0, which is the source's own
 * value and not a placeholder: the first sector descriptor and the first
 * animation joint each sit at the start of their file. */
#define NDS_RELOC_LVALUE(offset) (*(uintptr_t *)(uintptr_t)(offset))
#define llGRSectorMapMapHead NDS_RELOC_LVALUE(0x0u)
#define llGRSectorMapArwingLaser2DWeaponAttributes NDS_RELOC_LVALUE(0xbcu)
#define llGRSectorMapArwingLaser3DWeaponAttributes NDS_RELOC_LVALUE(0xf0u)
#define llGRSectorMapArwing0SectorDesc NDS_RELOC_LVALUE(0x0000u)
#define llGRSectorMapArwing1SectorDesc NDS_RELOC_LVALUE(0x0250u)
#define llGRSectorMapArwing2SectorDesc NDS_RELOC_LVALUE(0x06d0u)
#define llGRSectorMapArwing3SectorDesc NDS_RELOC_LVALUE(0x03e0u)
#define llGRSectorMapArwing4SectorDesc NDS_RELOC_LVALUE(0x0d10u)
#define llGRSectorMapArwing5SectorDesc NDS_RELOC_LVALUE(0x0eb0u)
#define llGRSectorMapArwing6SectorDesc NDS_RELOC_LVALUE(0x1510u)
#define llGRSectorMapArwing7SectorDesc NDS_RELOC_LVALUE(0x11d0u)
#define llGRSectorMapArwing0AnimJoint NDS_RELOC_LVALUE(0x0000u)
#define llGRSectorMapArwing1AnimJoint NDS_RELOC_LVALUE(0x1d34u)
#define llGRSectorMapArwing2AnimJoint NDS_RELOC_LVALUE(0x1da4u)
#define llGRSectorMapArwing3AnimJoint NDS_RELOC_LVALUE(0x1dc4u)
#define llGRSectorMapArwing4AnimJoint NDS_RELOC_LVALUE(0x1d54u)
#define llGRSectorMapArwing5AnimJoint NDS_RELOC_LVALUE(0x1de4u)

/* The Arwing's model is Fox's Landmaster entry model, not a Sector Z asset:
 * the stage builds it from FoxSpecial3 (grsector.c:1103,1116). Offsets from
 * decomp reloc_data.us.h:3382-3383. */
#define llFoxSpecial3EntryArwingDObjDesc NDS_RELOC_LVALUE(0x2c30u)
#define llFoxSpecial3_2E74_AnimJoint NDS_RELOC_LVALUE(0x2e74u)
#define llFoxSpecial3_1B34_AnimJoint NDS_RELOC_LVALUE(0x1b34u)
#define llFoxSpecial3_1B84_AnimJoint NDS_RELOC_LVALUE(0x1b84u)
#define llFoxSpecial3_2EB4_AnimJoint NDS_RELOC_LVALUE(0x2eb4u)

void ndsBaseGRCommonSetupInitAll(void);

#include "../../decomp/BattleShip-main/decomp/src/gr/grcommon/grsector.c"

/* Gameplay transcription notes (all numeric behaviour is the included source,
 * cited per constant -- nothing below re-states a number):
 *
 * - Patrol: Sleep (grsector.c:349) and Wait (:358) roll the flight pattern and
 *   spawn; the pilot state machine is :476 and the patrol dispatch :1023.
 * - 2D lasers: the count is picked at :523, targets authorised at :533, the
 *   shot spawned at :663 through wpManagerMakeWeapon (:685, :708), and its
 *   explosion handled at :722 and :733.
 * - 3D lasers: aimed spawn at :798 (:872), fire and cue at :887, ammunition
 *   sequenced at :899.
 * - The ship as a collider: grSectorArwingUpdateCollisions (:991), driving
 *   mpCollisionSetYakumonoOnID / PosID / OffID (:1005-1014, :1038, :1118).
 * - Bounds come from the source MPGroundData, not from any port constant.
 *
 * PRESENTATION ADAPTATIONS (gameplay untouched):
 * - The Arwing, its lasers and the background draw through the port's existing
 *   DObj renderer rather than a Sector-specific native packet; the law 8
 *   packet for every stage is one pipeline job (P2-4n1).
 */
void ndsGRSectorSetupInitAll(void)
{
    ndsBaseGRCommonSetupInitAll();
}

#endif /* NDS_P2_STAGE_SECTOR */
