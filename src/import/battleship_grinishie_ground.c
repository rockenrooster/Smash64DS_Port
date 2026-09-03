/* Bounded Mushroom Kingdom (Inishie) ground setup import, P2-4 stage 7.
 *
 * Mirrors the six stage wrappers beside it: this translation unit imports
 * decomp gr/grcommon/grinishie.c verbatim and exposes ndsGRInishieSetupInitAll
 * for the shared grCommonSetupInitAll gate. Only built when
 * NDS_P2_STAGE_INISHIE=1.
 *
 * THIS IS THE MOST BESPOKE STAGE IN THE GAME BAR SECTOR Z, with three
 * independent systems rather than one hazard:
 *
 *   - The two seesaw platforms, which weigh the fighters standing on each
 *     line group (grinishie.c:90) and displace a moving yakumono.
 *   - The POW block, an ITEM spawned at a cached map-object position, whose
 *     damage reaches a fighter through ftMainCheckAddGroundHazard -- the
 *     ground-HAZARD seam this port gained with Planet Zebes.
 *   - The two Piranha Plants, also items, owned by the item system once
 *     spawned.
 *
 * The port already carried part of the first: battleship_grinishie_scale.c
 * imports grInishieMakeScale behind NDS_ENABLE_INISHIE_SOURCE_SCALE_SETUP.
 * That file stays as it is; this one brings in the whole stage, and the two
 * are mutually exclusive by flag rather than by luck -- see the Makefile.
 *
 * WHAT IS ABSENT AND WHY IT IS SAFE. Neither nITKindPowerBlock nor
 * nITKindPakkun has a maker yet, so both spawns return NULL. The source
 * guards on that in both places, and itPakkunCommonSetWaitFighter is
 * NULL-guarded at source too, so the seesaws run, the POW timer runs, and
 * neither item appears until its kind lands in P2-5.
 *
 * THE HANG THIS STAGE SHIPS WITH. grInishieMakePowerBlock (grinishie.c:515-522)
 * answers a POW map-object count of zero or above ten with
 * `while (TRUE) syDebugPrintf(...)`, exactly as Hyrule Castle does for its
 * tornado. The admission arm checks the count first and refuses with a
 * counter, for the same reason.
 */
#if NDS_P2_STAGE_INISHIE

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
s32 syUtilsRandIntRange(s32 range);
void syDebugPrintf(const char *fmt, ...);
void scManagerRunPrintGObjStatus(void);

/* Offsets from decomp reloc_data.us.h:3920-3924, the authoritative symbol
 * table. */
#define NDS_RELOC_LVALUE(offset) (*(uintptr_t *)(uintptr_t)(offset))
#define llGRInishieMapScaleDObjDesc NDS_RELOC_LVALUE(0x380u)
#define llGRInishieMapMapHead NDS_RELOC_LVALUE(0x5f0u)
#define llGRInishieMapScaleRetractAnimJoint NDS_RELOC_LVALUE(0x734u)
#define llGRInishieMapPowerBlockGRAttackColl NDS_RELOC_LVALUE(0x0bcu)

void ndsBaseGRCommonSetupInitAll(void);

#include "../../decomp/BattleShip-main/decomp/src/gr/grcommon/grinishie.c"

/* Gameplay transcription notes (all numeric behaviour is the included source,
 * cited per constant -- nothing below re-states a number):
 *
 * - Seesaws: grInishieScaleUpdateWait (grinishie.c:118) alternates altitude
 *   and acceleration and falls past the source's own limit; Fall (:224),
 *   Step (:252) and Retract (:266) complete the cycle, dispatched by
 *   ScaleProcUpdate (:320) which also drives the yakumono position (:340-341).
 *   Pressure is the summed weight of every fighter whose floor line matches
 *   (:90).
 * - POW block: PBUpdateWait (:432) arms on battle start, PBSetWait (:442),
 *   PBUpdateMake (:449) spawns at a cached position, PBUpdateDamage (:477)
 *   clears the hazard after two tics, and CheckGetDamageKind (:545) yields
 *   nGMHitEnvironmentPowerBlock with the map's own GRAttackColl.
 * - Piranhas: PakkunSetWait (:402) and MakePakkun (:413), two of them at the
 *   PakkunL and PakkunR map objects.
 * - Bounds come from the source MPGroundData, not from any port constant.
 *
 * PRESENTATION ADAPTATIONS (gameplay untouched):
 * - The kingdom, its pipes and the background draw through the port's existing
 *   DObj renderer rather than an Inishie-specific native packet; the law 8
 *   packet for every stage is one pipeline job (P2-4n1).
 */
void ndsGRInishieSetupInitAll(void)
{
    ndsBaseGRCommonSetupInitAll();
}

#endif /* NDS_P2_STAGE_INISHIE */
